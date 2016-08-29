
#include "vt100sim.h"
#include "8080/sim.h"
#include "8080/simglb.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <signal.h>
#include <map>
#include <ctype.h>
#include "default_rom.h"

extern "C" {
extern void int_on(void), int_off(void);
extern void init_io(void), exit_io(void);

extern void cpu_z80(void), cpu_8080(void);
extern void disass(unsigned char **, int);
extern int exatoi(char *);
extern int getkey(void);
extern void int_on(void), int_off(void);
extern int load_file(char *);

}

Vt100Sim* sim;

WINDOW* regWin;
WINDOW* memWin;
WINDOW* vidWin;
WINDOW* msgWin;
WINDOW* statusBar;
WINDOW* bpWin;

Vt100Sim::Vt100Sim(const char* romPath, bool running, bool avo_on) :
	running(running), inputMode(false),
	dc12(true), controlMode(!running),
	enable_avo(avo_on)
{
  this->romPath = romPath;
  base_attr = 0;
  screen_rev = 0;
  blink_ff = 0;
  cols132 = 0;
  refresh50 = 0;
  interlaced = 0;
  bright = 0xF0;
  invalidated = true;

  //breakpoints.insert(8);
  //breakpoints.insert(0xb);
  initscr();
  int std_y,std_x;
  getmaxyx(stdscr,std_y,std_x);
  start_color();
  raw();
  nonl();
  noecho();
  keypad(stdscr,1);
  nodelay(stdscr,1);
  curs_set(0);

  /* Stdscr starts as dirty spaces, make sure they're cleaned. */
  clearok(curscr, 1);
  wrefresh(stdscr);

  /* The colour parts of the attributes we want to use */
  init_pair(1,COLOR_RED,COLOR_BLACK);
  init_pair(2,COLOR_BLUE,COLOR_BLACK);
  init_pair(3,COLOR_YELLOW,COLOR_BLACK);
  init_pair(4,COLOR_GREEN,COLOR_BLACK);

  // All the windows we need.
  makeScr();
}

Vt100Sim::~Vt100Sim() {
  curs_set(1);
  endwin();
}

class Signal {
private:
    bool value;
    uint32_t period_half;
    uint32_t ticks;
public:
    Signal(uint32_t period);
    bool add_ticks(uint32_t delta);
    bool get_value() { return value; }
    void change_period(uint32_t period) { period_half = period/2; }
};

Signal::Signal(uint32_t period) : value(false),period_half(period/2),ticks(0) {
}

bool Signal::add_ticks(uint32_t delta) {
    ticks += delta;
    if (ticks >= period_half) {
        ticks -= period_half;
        value = !value;
        return true;
    }
    return false;
}

// In terms of processor cycles:
// LBA4 : period of 22 cycles
// LBA7 : period of 182 cycles
// Vertical interrupt: period of 46084 cycles
Signal lba4(22);
Signal lba7(182);
Signal vertical(46084);
Signal uartclk(2880);   // uart clock is 9600 baud
			// The VT100 CPU can process data at a bit over
			// 4800 baud, so this will be sufficient.

void Vt100Sim::init() {
    i_flag = 1;
    f_flag = 0;
    m_flag = 0;
    tmax = f_flag*10000;
    cpu = I8080;
    wprintw(msgWin,"\nRelease %s, %s\n", RELEASE, COPYR);
#ifdef	USR_COM
    wprintw(msgWin,"\n%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
#endif

    //printf("Prep ram\n");
    //fflush(stdout);
    wrk_ram	= PC = ram;
    STACK = ram + 0xffff;
    if (cpu == I8080)	/* the unused flag bits are documented for */
        F = 2;		/* the 8080, so start with bit 1 set */
    memset((char *)	ram, m_flag, 65536);
    memset((char *)	touched, m_flag, 65536);
    if (romPath)
    {
	// load binary
	wprintw(msgWin,"Loading rom %s...\n",romPath);
	wrefresh(msgWin);
	FILE* romFile = fopen(romPath,"rb");
	if (!romFile) {
	  wprintw(msgWin,"Failed to read rom file\n");
	  wrefresh(msgWin);
	    return;
	}
	uint32_t count = fread((char*)ram,1,2048*4,romFile);
	//printf("Read ROM file; %u bytes\n",count);
	fclose(romFile);
    }
#ifdef DEFAULT_ROM
    else
	memcpy((char*)ram,default_rom, sizeof(default_rom));
#else
    else {
	wprintw(msgWin,"No default ROM included (CPU STOPPED)\n");
	running = false;
	controlMode = true;
    }
#endif
    int_on();
    // add local io hooks
    
    i_flag = 0;

    // We are always running the CPU in single-step mode so we can do the clock toggles when necessary.
    cpu_state = SINGLE_STEP;

    wprintw(msgWin,"Function Key map:\n");
    wprintw(msgWin,"F1..F4 -> PF1..PF4\n");
    wprintw(msgWin,"F5 -> Break\n");
    wprintw(msgWin,"F9 -> Setup\n");
    wprintw(msgWin,"F10 -> Cmd Mode\n");
    wprintw(msgWin,"F11 -> Escape\n");
    wprintw(msgWin,"F12 -> Backspace\n");
    wprintw(msgWin,"F13 -> Linefeed\n");
    wprintw(msgWin,"F14 -> Keycodes\n");
    wprintw(msgWin,"F15 -> Hangup\n");

}

BYTE Vt100Sim::ioIn(BYTE addr) {
  if (addr == 0x00) {
    uint8_t r = uart.read_data();
    //wprintw(msgWin,"PUSART RD DAT: %x\n", r);
    return r;
  } else if (addr == 0x01) {
    uint8_t r = uart.read_command();
    //wprintw(msgWin,"PUSART RD CMD: %x\n", r);
    return r;
  } else if (addr == 0x42) {
        // Read buffer flag
	/*
	    flags:
		AVO	on=0x00, off=0x02
		GPO	on=0x00, off=0x04
		STP	on=0x08, off=0x00
	 */
        uint8_t flags = 0x06;	/* STP off, GPO off, AVO off */
	if (enable_avo)
	    flags = 0x04;

        if (lba7.get_value()) {
            flags |= 0x40;
        }
        if (nvr.data()) {
            flags |= 0x20;
        }
	if (uart.xmit_ready()) {
	  flags |= 0x01;
	}
        if (t_ticks % (46084*2) < 46084) flags |= 0x10;
        if (kbd.get_tx_buf_empty()) {
            flags |= 0x80; // kbd ready?
        }
        //printf(" IN PORT %02x -- %02x\n",addr,flags);fflush(stdout);
        return flags;
    } else if (addr == 0x82) {
      return kbd.get_latch();
    } else {
      //printf(" IN PORT %02x at %04lx\n",addr,PC-ram);fflush(stdout);
    }
    return 0;
}

void Vt100Sim::ioOut(BYTE addr, BYTE data) {
    switch(addr) {
    case 0x00:
      //wprintw(msgWin,"PUSART DAT: %x\n", data);wrefresh(msgWin);
      uart.write_data(data);
      break;
    case 0x01:
      //wprintw(msgWin,"PUSART CMD: %x\n", data);wrefresh(msgWin);
      uart.write_command(data);
      break;
    case 0x02:
      {
	/*
	  BAUD rates 0xTTTTRRRR
	  Transmit/receive rates 0x0..0xF
	*/
	static int baud_clock_divisors[16] = {
	3456,   // 50
	2304,   // 75
	1571,   // 110
	1285,   // 134.5
	1152,   // 150
	864,    // 200
	576,    // 300
	288,    // 600
	144,    // 1200
	96,     // 1800
	86,     // 2000
	72,     // 2400
	48,     // 3600
	36,     // 4800
	18,     // 9600
	9,      // 19200
	};

	static int lastbaud = 0;
	if (lastbaud != data) {
	  /* Clock divider is always set to 16. We are sending 1+8+1 bits */
	  uartclk.change_period(160* baud_clock_divisors[data&0xF]);
	  lastbaud = data;
	}
      }
      break;
    case 0x82:
        kbd.set_status(data);
        break;
    case 0x62:
        nvr.set_latch(data);
	break;
    case 0x42:
      bright = data;
      break;

    case 0xa2:
      //wprintw(msgWin,"DC12 %02x\n",data);
      //wrefresh(msgWin);
      dc12 = true;
      switch (data & 0xF) {
      case 0: case 1: case 2: case 3:
         scroll_latch = ((scroll_latch & 0x0C) | (data & 0x3));
	 break;
      case 4: case 5: case 6: case 7:
         scroll_latch = ((scroll_latch & 0x03) | ((data & 0x3)<<2));
	 break;
      case 8: /* Toggle blink FF */
	 blink_ff = ~blink_ff;
         break;
      case 9: /* Vertical retrace clear. */
         break;
      case 10: screen_rev = 0x80; break;
      case 11: screen_rev = 0; break;

      case 12: base_attr = 1; blink_ff = 0; break;	/* Underline */
      case 13: base_attr = 0; blink_ff = 0; break;	/* Reverse */
      case 14: case 15: blink_ff = 0; break;
      }
      break;

    case 0xc2:
      if (data & 0x20) {
         interlaced = 0;
	 refresh50 = ((data & 0x10) != 0);
	 if (refresh50) vertical.change_period(55296);
	 else		vertical.change_period(46084);
      } else {
         interlaced = 1;
	 cols132 = ((data & 0x10) != 0);
	 uart.write_cols(cols132);
      }
      break;

    default:
	wprintw(msgWin,"OUT PORT %02x <- %02x\n",addr,data);
	wrefresh(msgWin);
	break;
    }
}

std::map<int,uint8_t> make_code_map() {
  std::map<int,uint8_t> m;
  // 0x01, 0x02 (both) -> del
  m[KEY_DC] = 0x03;
  // ??? m[KEY_ENTER] = 0x04;
  // 0x04 -> nul
  m['p'] = 0x05;
  m['o'] = 0x06;
  m['y'] = 0x07;
  m['t'] = 0x08;
  m['w'] = 0x09;
  m['q'] = 0x0a;

  // 0x0b, 0x0c, 0x0d, 0x0e, 0x0f (Mirror of next 5)

  m[KEY_RIGHT] = 0x10;
  // 0x11 -> nul
  // 0x12 -> nul
  // 0x13 -> nul
  m[']'] = 0x14; m['}'] = 0x94;
  m['['] = 0x15; m['{'] = 0x95;
  m['i'] = 0x16;
  m['u'] = 0x17;
  m['r'] = 0x18;
  m['e'] = 0x19;
  m['1'] = 0x1a; m['!'] = 0x9a;

  // 0x1b, 0x1c, 0x1d, 0x1e, 0x1f (Mirror of next 5)

  m[KEY_LEFT] = 0x20;
  // 0x21 -> nul
  m[KEY_DOWN] = 0x22;
  m[KEY_BREAK] = 0x23; m[KEY_F(5)] = 0x23; m[KEY_F(15)] = 0xA3;

  m['`'] = 0x24; m['~'] = 0xa4;
  m['-'] = 0x25; m['_'] = 0xa5;
  m['9'] = 0x26; m['('] = 0xa6;
  m['7'] = 0x27; m['&'] = 0xa7;
  m['4'] = 0x28; m['$'] = 0xa8;
  m['3'] = 0x29; m['#'] = 0xa9;
  m[KEY_CANCEL] = 0x2a; m[KEY_F(11)] = 0x2a; m[0x1b] = 0x2a; // Escape Key

  // 0x2b, 0x2c, 0x2d, 0x2e, 0x2f (Mirror of next 5)

  m[KEY_UP] = 0x30;
  m[KEY_F(3)] = 0x31;
  m[KEY_F(1)] = 0x32;
  m[KEY_BACKSPACE] = 0x33; m[KEY_F(12)] = 0x33;
  m['='] = 0x34; m['+'] = 0xb4;
  m['0'] = 0x35; m[')'] = 0xb5;
  m['8'] = 0x36; m['*'] = 0xb6;
  m['6'] = 0x37; m['^'] = 0xb7;
  m['5'] = 0x38; m['%'] = 0xb8;
  m['2'] = 0x39; m['@'] = 0xb9;
  m['\t'] = 0x3a;

  // 0x3b, 0x3c, 0x3d, 0x3e, 0x3f (Mirror of next 5)

  m[KEY_F(27)] = 0x40; m[KEY_A1] = 0x40; m[KEY_HOME] = 0x40; // 0x40 -> Keypad-'7'
  m[KEY_F(4)] = 0x41;
  m[KEY_F(2)] = 0x42;
  m[KEY_F(20)] = 0x43; m[KEY_IC] = 0x43; // 0x43 -> Keypad-'0'
  m['\n'] = 0x44; m[KEY_F(13)] = 0x44; // Linefeed key:
  m['\\'] = 0x45; m['|'] = 0xc5;
  m['l'] = 0x46;
  m['k'] = 0x47;
  m['g'] = 0x48;
  m['f'] = 0x49;
  m['a'] = 0x4a;

  // 0x4b, 0x4c, 0x4d, 0x4e, 0x2f (Mirror of next 5)

  m[KEY_F(28)] = 0x50; // 0x50 -> Keypad-'8'    (Keypad ^[Ox)
  m[KEY_F(32)] = 0x51; // 0x51 -> ^M	    (Keypad Enter)
  m[KEY_F(22)] = 0x52; // 0x52 -> Keypad-'2'
  m[KEY_F(21)] = 0x53; m[KEY_C1] = 0x53; m[KEY_END] = 0x53; // 0x53 -> '1'
  // 0x54 -> nul
  m['\''] = 0x55; m['"'] = 0xd5;
  m[';'] = 0x56; m[':'] = 0xd6;
  m['j'] = 0x57;
  m['h'] = 0x58;
  m['d'] = 0x59;
  m['s'] = 0x5a;

  // 0x5b, 0x5c, 0x5d, 0x5e, 0x5f

  m[KEY_F(33)] = 0x60; // 0x60 -> Keypad-'.'
  m[KEY_F(31)] = 0x61; // 0x61 -> Keypad-','
  m[KEY_F(25)] = 0x62; m[KEY_B2] = 0x62; // 0x62 -> Keypad-'5'
  m[KEY_F(24)] = 0x63; // 0x63 -> Keypad-'4'
  // 0x64 -> ^M	    (Return Key)
  m['\r'] = 0x64;
  m['.'] = 0x65; m['>'] = 0xe5;
  m[','] = 0x66; m['<'] = 0xe6;
  m['n'] = 0x67;
  m['b'] = 0x68;
  m['x'] = 0x69;
  m[KEY_F(6)] = 0x6a; // 0x6a -> NoSCROLL

  // 0x6b, 0x6c, 0x6d, 0x6e, 0x6f (Mirror of next 5)

  m[KEY_F(29)] = 0x70; m[KEY_A3] = 0x70; m[KEY_PPAGE] = 0x70; // 0x70 -> Keypad-'9'
  m[KEY_F(23)] = 0x71; m[KEY_C3] = 0x71; m[KEY_NPAGE] = 0x71; // 0x71 -> Keypad-'3'
  m[KEY_F(26)] = 0x72; // 0x72 -> Keypad-'6'
  m[KEY_F(30)] = 0x73; // 0x73 -> Keypad-'-'
  // 0x74 -> nul
  m['/'] = 0x75; m['?'] = 0xf5;
  m['m'] = 0x76;
  m[' '] = 0x77;
  m['v'] = 0x78;
  m['c'] = 0x79;
  m['z'] = 0x7a;

  // setup
  m[KEY_F(9)] = 0x7b;

  // 0x7c   Control Key
  // 0x7d   Shift Key
  // 0x7e
  // 0x7f

  for (int i = 0; i < 26; i++) {
    m['A'+i] = m['a'+i] | 0x80;
  }
  return m;
}

std::map<int,uint8_t> code = make_code_map();

bool hexParse(char* buf, int n, uint16_t& d) {
  d = 0;
  for (int i = 0; i < n; i++) {
    char c = buf[i];
    if (c == 0) return true;
    d = d << 4;
    if (c >= '0' && c <= '9') { d |= (c-'0'); }
    else if (c >= 'a' && c <= 'f') { d |= ((c-'a')+10); }
    else if (c >= 'A' && c <= 'F') { d |= ((c-'A')+10); }
    else {
      return false;
    }
  }
  return true;
}

void Vt100Sim::run() {
  const int CPUHZ = 2764800;
  int steps = 0;
  needsUpdate = true;
  gettimeofday(&last_sync, 0);
  has_breakpoints = (breakpoints.size() != 0);
  while(1) {
    if (running) {
      step();
      if (steps > 0) {
	if (--steps == 0) { running = false; }
      }
      uint16_t pc = (uint16_t)(PC-ram);
      //wprintw(msgWin,"BP %d PC %d\n",breakpoints.size(),pc);wrefresh(msgWin);
      if (has_breakpoints && breakpoints.find(pc) != breakpoints.end()) {
	wprintw(msgWin,"Breakpoint trace for %04x:\n",pc);
	for (int i = 10; i > 1; i--) {
	  uint16_t laddr = his[(HISIZE+h_next-i)%HISIZE].h_adr;
	  wprintw(msgWin,"  PC %04x\n",laddr);
	}
	wrefresh(msgWin);
	controlMode = true;
	running = false;
	invalidated = true;
      }
      if (rt_ticks > CPUHZ/100) {
        struct timeval now;
	gettimeofday(&now, 0);
	long long clock_usec =
	    (now.tv_sec-last_sync.tv_sec) * 1000000 +
	    (now.tv_usec-last_sync.tv_usec) ;
	long long cpu_usec = rt_ticks * 1000000 / CPUHZ;

	if (cpu_usec > clock_usec+10000) {
	  usleep(cpu_usec - clock_usec);
	  last_sync = now;
	  rt_ticks -= clock_usec * CPUHZ / 1000000;
	} else {
	  /* EMU is too slow ? */
	}
      }
    } else {
      usleep(50000);
      gettimeofday(&last_sync, 0);
      rt_ticks = 0;
    }
    int ch = ERR;
    if (vscan_tick>0) {
      vscan_tick--;
      refresh_clock++;
      if (needsUpdate && refresh_clock>3) { update(); refresh_clock=0; }

      last_latch = scroll_latch;
      if (controlMode || !kbd.busy_scanning()) {
	  ch = getch();
	  key_stuck = 0;
      } else if (key_stuck < 5) {
	  key_stuck++;
      } else {
	  wprintw(msgWin, "Keyboard STUCK\n");
	  controlMode = true;
      }

      has_breakpoints = (breakpoints.size() != 0);
    } else if (!running) {
      if (needsUpdate) update();
      ch = getch();
      has_breakpoints = (breakpoints.size() != 0);
      controlMode = true;
    }
    if (ch != ERR) {
      if (ch == KEY_F(10)) { // Control Mode key
	controlMode = !controlMode;
	needsUpdate = invalidated = true;
      } else if (controlMode &&
	    ch != KEY_F(5) && ch != KEY_F(15) && ch != KEY_F(9)) {
	if (ch == 'q' || ch == 4 || ch == 3) {
	  werase(statusBar);
	  wrefresh(statusBar);
	  return;
	}
	else if (ch == '\f') {
	  needsUpdate = invalidated = true;
	}
	else if (ch == ' ') {
	  running = !running;
	}
	else if (ch == 'n') {
	  running = true; steps = 1;
	}
	else if (ch == 'm') {
	  snapMemory(); dispMemory();
	}
	else if (ch == 'b') {
	  char bpbuf[10];
	  getString("Addr. of breakpoint: ",bpbuf,4);
	  invalidated = true;
	  uint16_t bp;
	  if (hexParse(bpbuf,4,bp)) {
	    addBP(bp);
	    dispBPs();
	    mvwprintw(statusBar,0,0,"Breakpoint added at %s\n",bpbuf); 
	  } else {
	    mvwprintw(statusBar,0,0,"Bad breakpoint %s\n",bpbuf); 
	  }
	  // set up breakpoints
	  gettimeofday(&last_sync, 0);	// CPU was frozen
	}
	else if (ch == 'd') {
	  char bpbuf[10];
	  getString("Addr. of bp to remove: ",bpbuf,4);
	  invalidated = true;
	  uint16_t bp;
	  if (hexParse(bpbuf,4,bp)) {
	    if (breakpoints.count(bp) == 0) {
	      mvwprintw(statusBar,0,0,"No breakpoint %s\n",bpbuf); 
	    } else {
	      clearBP(bp);
	      dispBPs();
	      mvwprintw(statusBar,0,0,"Breakpoint removed at %s\n",bpbuf); 
	    }
	  } else {
	    mvwprintw(statusBar,0,0,"Bad breakpoint %s\n",bpbuf); 
	  }
	  // set up breakpoints
	  gettimeofday(&last_sync, 0);	// CPU was frozen
	}
	else if (ch == 's') {
	  char path[128];
	  getString("Save to: ",path,sizeof path);
	  invalidated = true;

	  FILE* f = fopen(path,"w");
	  int i;
	  if (!f) {
	      wprintw(msgWin,"Cannot save mem file %s\n", path);
	  } else {
	    for(i=0; i<8192; i++) {
		unsigned char c = ram[0x2000+i];
		fprintf(f, "%02x", c);
		if (i % 16 == 15) fprintf(f, "\n"); else fprintf(f, " ");
	    }
	    fclose(f);
	    wprintw(msgWin,"File saved\n");
	  }
	  gettimeofday(&last_sync, 0);	// CPU was frozen
	}
	needsUpdate = true;
      }
      else {
        static int kstat = 0, ksum = 0;
	if (controlMode) {
	    controlMode = !controlMode;
	    invalidated = true;
	}

	if (kstat || ch == KEY_F(14)) {
	    if (ch == KEY_F(14)) {
		kstat = 1; ksum = 0;
	    } else if (ch == '\n' || ch == '\r') {
		ksum |= (kstat & 0xF80);
		if (ksum & 0x80) keypress(0x7d);	// Shift Key
		if (ksum & 0x100) keypress(0x7c);	// Ctrl Key
		ksum &= 0x7f;
		if (ksum)
		    keypress(ksum);
		ksum = kstat = 0;
	    } else if (ch >='0' && ch <='9') {
		ksum = ksum * 10 + ch - '0';
	    } else if (ch >='s') {
		kstat |= 0x80;
	    } else if (ch >='c') {
		kstat |= 0x100;
	    } else
		kstat = 0;
	} else {
	    uint8_t kc = code[ch];
	    if (kc == 0 && ch >= 0 && ch < 32) {
		kc = code[ch+'@'] & 0x7F;
		if (ch == 0 ) kc = 0x77;        // Ctrl-Space -> ^@
		if (ch == '\036') kc = 0x24;    // Ctrl-` -> ^^
		if (ch == '\037') kc = 0x75;    // Ctrl-/ -> ^_
		if (kc) {
	          //wprintw(msgWin,"KC=7c (Control)\n");
		  keypress(0x7c);	// Control Key
		}
	    }
	    if (kc == 0 && ch == KEY_F(16)) {
	      keypress(0x7c);	// Control break, send answerback.
	      kc = 0x23;
	    }
	    if (kc & 0x80) {
	      //wprintw(msgWin,"KC=7d (Shift)\n");
	      keypress(0x7d);	// Shift Key
	      kc &= 0x7f;
	    }
	    // If the key we've generated is on the number pad we're after an
	    // application keypad key. Poke the memory to force it.
	    // PS: You didn't see me do that!
	    if ((kc&0xF) < 4 && (kc&0x70) >= 0x40 && ram[0x2178] == 0)
	      ram[0x2178] = 1;
	    if (kc)
	      keypress(kc);
	}
      }
    }
  }
}

void Vt100Sim::getString(const char* prompt, char* buf, uint8_t sz) {
  uint8_t l = strlen(prompt);
  werase(statusBar);
  mvwprintw(statusBar,0,0,prompt);
  echo();
  curs_set(1);
  leaveok(statusBar, false);
  wgetnstr(statusBar,buf,sz);
  noecho();
  curs_set(0);
  leaveok(statusBar, true);
  invalidated = true;
}

void Vt100Sim::step()
{
  const uint32_t start = t_ticks;
  cpu_error = NONE;
  cpu_8080();
  if (int_int == 0) { int_data = 0xc7; }
  const uint16_t t = t_ticks - start;
  if (bright != 0xF0) rt_ticks += t;
  if (uartclk.add_ticks(t) && uartclk.get_value()) {
    if (uart.clock()) {
      int_data |= 0xd7;
      int_int = 1;
      //wprintw(msgWin,"UART interrupt\n");wrefresh(msgWin);
      if (uart.shell_pid() < 0)
      {
	controlMode = true;
	invalidated = true;
      }
    }
  }
  if (dc12 && lba4.add_ticks(t)) {
    if (kbd.clock(lba4.get_value())) {
      int_data |= 0xcf;
      int_int = 1;
      //wprintw(msgWin,"KBD interrupt\n");wrefresh(msgWin);
    }
  }
  if (dc12 && lba7.add_ticks(t)) {
    nvr.clock(lba7.get_value());
  }
  if (dc12 && vertical.add_ticks(t)) {
    if (vertical.get_value()) {
      int_data |= 0xe7;
      int_int = 1;
      vscan_tick++;
    }
  }
  needsUpdate = true;
}

void Vt100Sim::makeScr() {
  int std_x, std_y;
  int vid_x = (cols132?132:80);

  getmaxyx(stdscr,std_y,std_x);

  if (!invalidated && stdscr_x == std_x && stdscr_y == std_y) {
    int my,mx = 0;
    if (vidWin) getmaxyx(vidWin,my,mx);
    if (std_x < vid_x || mx == vid_x || mx == vid_x+2)
      return;
  }

  stdscr_x = std_x; stdscr_y = std_y;
  invalidated = false;

  // Status bar: bottom line of the screen
  if (statusBar) delwin(statusBar);
  statusBar = newwin(1,std_x,std_y-1,0);

  // Register window is fixed size.
  const int regw = 11;
  const int regh = 8;
  if (!regWin) regWin = newwin(regh,regw,0,0);

  // The video window is 24..26 lines by 80..134 columns
  if (vidWin) delwin(vidWin);
  const int vidpos = (std_y<=27?0:std_y-27);
  const int vidwt = std::min(vid_x+2,std_x);
  int vidht = std::min(26,std_y); // video area height (max 26 rows)
  if (std_y == 25 || std_y == 26) vidht--;

  vidWin = newwin(vidht,vidwt,vidpos,0);

  // Guess a nice division between the memory window and the message window.
  int memw = 7 + 32*3; // memory area width: big enough for 32B across
  if (memw > std_x -regw - 18) memw = 7 + 16*3; // Okay, make that 16
  if (memw > std_x -regw - 18) memw = 7 + 8*3; // Okay, make that 8
  const int msgw = std_x - (regw+memw); // message area: std_x - memory area - register area (12)

  int memht = std::max(12,std_y-vidht-1);
  if (std_y < 39) memht = std::max(12,std_y-1);

  if (bpWin) delwin(bpWin);
  if (memWin) delwin(memWin);
  bpWin = newwin(memht-regh,regw,regh,0);
  memWin = newwin(memht,memw,0,regw);

#ifdef NCURSES_VERSION
  if (!msgWin)
    msgWin = newwin(memht,msgw,0,regw+memw);
  else {
    wresize(msgWin, memht,msgw);
    mvwin(msgWin, 0,regw+memw);
  }
#else
  if (msgWin) delwin(msgWin);
  msgWin = newwin(memht,msgw,0,regw+memw);
#endif

  scrollok(msgWin,1);

  leaveok(statusBar, true);
  leaveok(regWin, true);
  leaveok(vidWin, true);
  leaveok(bpWin, true);
  leaveok(memWin, true);
  leaveok(msgWin, true);

  wattroff(regWin,COLOR_PAIR(1));
  box(regWin,0,0);
  mvwprintw(regWin,0,1,"Registers");
  wattron(regWin,COLOR_PAIR(1));

  box(memWin,0,0);
  mvwprintw(memWin,0,1,"Mem");
  mvwprintw(vidWin,0,1,"Video (init)");
  box(bpWin,0,0);
  mvwprintw(bpWin,0,1,"Brkpts");

  touchwin(regWin);
  touchwin(memWin);
  touchwin(vidWin);
  touchwin(msgWin);
  touchwin(statusBar);
  touchwin(bpWin);
  wclear(newscr);
  clearok(curscr,1);
}

void Vt100Sim::update() {

  needsUpdate = false;

  makeScr();
  if (stdscr_y >= 39) {
    wrefresh(msgWin);
    dispRegisters();
    dispMemory();
    dispBPs();
    dispVideo();
    dispStatus();
  } else if (controlMode) {
    wrefresh(msgWin);
    dispRegisters();
    dispMemory();
    dispBPs();
    dispStatus();
  } else {
    dispVideo();
    if (stdscr_y > 24)
      dispStatus();
  }
}

void Vt100Sim::keypress(uint8_t keycode)
{
    kbd.keypress(keycode);
}

void Vt100Sim::clearBP(uint16_t bp)
{
  breakpoints.erase(bp);
}

void Vt100Sim::addBP(uint16_t bp)
{
  breakpoints.insert(bp);
}

void Vt100Sim::clearAllBPs()
{
  breakpoints.clear();
}

void Vt100Sim::dispRegisters() {
  mvwprintw(regWin,1,1,"A %02x",A);
  mvwprintw(regWin,2,1,"B %02x C %02x",B,C);
  mvwprintw(regWin,3,1,"D %02x E %02x",D,E);
  mvwprintw(regWin,4,1,"H %02x L %02x",H,L);
  mvwprintw(regWin,5,1,"PC %04x",(PC-ram));
  mvwprintw(regWin,6,1,"SP %04x",(STACK-ram));
  wrefresh(regWin);
}

void Vt100Sim::dispVideo() {
  uint16_t start = 0x2000;
  int my,mx;
  int lattr = 3;
  int inscroll = 0;

  int have_border = 0, have_title = 0;
  int want_x = (cols132?132:80);

  getmaxyx(vidWin,my,mx);
  werase(vidWin);

  if (bright == 0xF0) {
      mvwprintw(vidWin,0,1,"Video [disabled]");
      wrefresh(vidWin);
      return;
  }

  if (mx >= want_x+2 && my >= 26) have_border = 1;
  if (my >= 25) have_title = 1;

  // BTW: The VT100 is white on black, not green on black.
  wattron(vidWin,COLOR_PAIR(4));
  int y = -2, x = 0;
  bool scroll_fix = (last_latch!=0);
  bool is_setup = false, is_setupB = false;
  if (refresh50) y -= 3;
  for (uint8_t i = 1; i < 31 && y < 25; i++) {
        char* p = (char*)ram + start;
        char* maxp = p + 133;
	//if (*p != 0x7f) y++;
	y++; x=0;
	if (y>0) {
	    wmove(vidWin,y+have_title-1,have_border);
	    if (scroll_latch || last_latch) {
		if (inscroll)
		    wattron(vidWin,COLOR_PAIR(1));
		else
		    wattron(vidWin,COLOR_PAIR(4));
	    }
	}

	if (y == 1) {
	  is_setup = (scroll_latch==0 && memcmp(p, "SET-UP A\177", 9) == 0);
	  is_setupB = (scroll_latch==0 && memcmp(p, "SET-UP B\177", 9) == 0);
	  is_setup = is_setup || is_setupB;
	}

        while (*p != 0x7f && p != maxp) {
            unsigned char c = *p;
	    int attrs = enable_avo?p[0x1000]:0xF;
	    bool lattr_done = false;
	    p++;
	    if (y > 0 && x<mx && !(scroll_fix && inscroll)) {
	      bool inverse = !!(c & 128);
	      bool blink = !(attrs & 0x1);
	      bool uline = !(attrs & 0x2);
	      bool bold = !(attrs & 0x4);
	      bool altchar = !(attrs & 0x8);
	      c &= 0x7F;
	      x++;

	      if (!enable_avo && base_attr) { uline = inverse; inverse = 0; }

	      if (screen_rev) inverse = !inverse;

	      if (inverse) wattron(vidWin,A_REVERSE);
	      if (uline) wattron(vidWin,A_UNDERLINE);
	      if (blink) wattron(vidWin,A_BLINK);
	      if (bold) wattron(vidWin,A_BOLD);

	      if (c == 0 || c == 127) {
		waddch(vidWin,' ');
	      } else  {
#ifdef _XOPEN_CURSES
extern int utf8_term;
static int vt100_chars[] = {
	0x0020, 0x2666, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0,
	0x00b1, 0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c,
	0x23ba, 0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534,
	0x252c, 0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3, 0x00b7
	};

static int dec_mcs[] = {
	0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
	0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
	0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
	0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x2426, 0x00a5, 0x2426, 0x00a7,
	0x00a4, 0x00a9, 0x00aa, 0x00ab, 0x2426, 0x2426, 0x2426, 0x2426,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x2426, 0x00b5, 0x00b6, 0x00b7,
	0x2426, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x2426, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x2426, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x0152,
	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0178, 0x2426, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x2426, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x0153,
	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00ff, 0x2426, 0xFFFD
	};

		if (lattr!=3 && utf8_term && (c == 30 || (c>' ' && c<='~'))) {
		    wchar_t ubuf[2] = { c - ' ' + 0xFF00, '\0' };
		    if (c == 30) ubuf[0] = 0xFFE1;
		    waddwstr(vidWin,ubuf);
		    lattr_done = true;
		} else
		if ((c>=3 && c<=6) || (c<32 && utf8_term)) {
		    wchar_t ubuf[2] = { vt100_chars[c&0x1F], '\0' };
		    waddwstr(vidWin,ubuf);
		} else if (c < 32) { waddch(vidWin,NCURSES_ACS(0x5F+c));
		} else if (altchar) {

		    wchar_t ubuf[2] = { dec_mcs[c], '\0' };
		    waddwstr(vidWin,ubuf);
		}
#else
		if (c < 32) { waddch(vidWin,NCURSES_ACS(0x5F+c)); }
#endif
		else { waddch(vidWin,c); }
	      }

	      if (lattr!=3 && !lattr_done) waddch(vidWin,' ');
	      if (inverse) wattroff(vidWin,A_REVERSE);
	      if (uline) wattroff(vidWin,A_UNDERLINE);
	      if (bold) wattroff(vidWin,A_BOLD);
	      if (blink) wattroff(vidWin,A_BLINK);
            }
        }
        if (p == maxp) {
	  //wprintw(msgWin,"Overflow line %d\n",i); wrefresh(msgWin);
	  break;
	}
        // at terminator
	if (scroll_fix && inscroll) { y--; scroll_fix = false; }
	if (y>=4 && y<=22) is_setup = (is_setup && 0 == p-(char*)ram-start);
        p++;
        unsigned char a1 = *(p++);
        unsigned char a2 = *(p++);
        //printf("Next: %02x %02x\n",a1,a2);fflush(stdout);
        uint16_t next = (((a1&0x10)!=0)?0x2000:0x4000) | ((a1&0x0f)<<8) | a2;
	lattr = ((a1 >> 5) & 0x3);
	inscroll = ((a1 >> 7) & 0x1);
        if (start == next) break;
        start = next;
    }

  if (is_setup) {
    y = have_title+4;
    wmove(vidWin,y++,have_border);
    y++;
    wprintw(vidWin,"Use the arrow keys, return and space to move cursor position.");
    if(!is_setupB) {
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"The up and down arrows control the brightness.   0 => Reset");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"(on a real VT100!)                               1 => ");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 2 => Toggle TAB");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 3 => Clear all TABs");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 4 => Online/Local");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 5 => Setup B");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 6 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 7 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 8 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 9 => Toggle 80/132");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 Shift-S => Save");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"                                                 Shift-R => Recall");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"");
    } else {
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  X     Scroll 1-Smooth                          0 => Reset");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .X    Repeat 1-On                              1 => ");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  . X   Screen 1-LightBG                         2 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  X  Cursor 1-Block                           3 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    X     Margin Bell                      4 => Online/Local");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .X    Keyclick 1-On                    5 => Setup A");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    . X   1=Ansi/0=VT52                    6 => Toggle this");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  X  FlowCtrl 1-On                    7 => Next Xmit Speed");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    X     UK-Ascii 1-On            8 => Next Rcv Speed");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .X    LineWrap 1-On            9 =>");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    . X   1-Crlf, 0-Cr");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .  X  1-Interlace              Shift-S => Save");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .  .    X     Parity 1-Even    Shift-R => Recall");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .  .    .X    Parity Sense     Shift-A => Answerback");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .  .    . X   BPC 1=8Bit");
      wmove(vidWin,y++,have_border);
      wprintw(vidWin,"  .  .    .  .    .  .    .  X  Refresh 1=50Hz");
    }

  }

  wattroff(vidWin,COLOR_PAIR(4));
  if (have_title) {
    if (have_border) box(vidWin,0,0);
    mvwprintw(vidWin,0,1,"Video [bright %d%%]",(32-bright)*100/32);
    if (scroll_latch) wprintw(vidWin,"[Scroll %d]",scroll_latch);
    // if (blink_ff) wprintw(vidWin,"[BLINK]");
  }
  wrefresh(vidWin);
}

void displayFlag(const char* text, int on) {
  if (on) {
    if (on == 1)
      wattron(statusBar,COLOR_PAIR(3));
    else
      wattron(statusBar,COLOR_PAIR(1));
    wattron(statusBar,A_BOLD);
  } else {
    wattron(statusBar,COLOR_PAIR(2));
    wattroff(statusBar,A_BOLD);
  }
  wprintw(statusBar,text);
  waddch(statusBar,' ');
  waddch(statusBar,' ');
}

void Vt100Sim::dispStatus() {
  // ONLINE LOCAL KBD LOCK L1 L2 L3 L4
  const static char* ledNames[] = { 
    "ONLINE", "LOCAL", "KBD LOCK", "L1", "L2", "L3", "L4", "BEEP " };
  const int lwidth = 8 + 7 + 10 + 4 + 4 + 4 + 4;
  const int awidth = 2 + 7 + 3 + 7 + 5;
  int mx, my;
  werase(statusBar);
  getmaxyx(statusBar,my,mx);
  wmove(statusBar,0,mx-lwidth);
  uint8_t flags = kbd.get_status();
  displayFlag(ledNames[0], (flags & (1<<5)) == 0 );
  if ((flags & (1<<7)) != 0) {
    displayFlag(ledNames[7], 2);
  } else {
    displayFlag(ledNames[1], (flags & (1<<5)) != 0 );
  }
  displayFlag(ledNames[2], (flags & (1<<4)) != 0 );
  displayFlag(ledNames[3], (flags & (1<<3)) != 0 );
  displayFlag(ledNames[4], (flags & (1<<2)) != 0 );
  displayFlag(ledNames[5], (flags & (1<<1)) != 0 );
  displayFlag(ledNames[6], (flags & (1<<0)) != 0 );

  // Mode information
  char * ptyn = uart.pty_name();
  int posn = strlen(ptyn);
  posn = (mx-lwidth-awidth-posn)/2;
  if (posn<0) posn = 0;
  wmove(statusBar,0,posn);
  wattrset(statusBar,A_BOLD);
  wprintw(statusBar,"| ");
  if (controlMode) {
    wprintw(statusBar,"CONTROL");
  } else {
    wattron(statusBar,A_REVERSE);
    wprintw(statusBar,"TYPING");
    wattroff(statusBar,A_REVERSE);
    wprintw(statusBar," ");
  }
  wprintw(statusBar," | ");
  if (running) {
    wprintw(statusBar,"RUNNING");
  } else {
    wattron(statusBar,A_REVERSE);
    wprintw(statusBar,"STOPPED");
    wattroff(statusBar,A_REVERSE);
  }
  wprintw(statusBar," | %s |",ptyn);
  wrefresh(statusBar);
}

void Vt100Sim::dispBPs() {
  int y = 1;
  werase(bpWin);
  box(bpWin,0,0);
  mvwprintw(bpWin,0,1,"Brkpts");
  for (std::set<uint16_t>::iterator i = breakpoints.begin();
       i != breakpoints.end();
       i++) {
    mvwprintw(bpWin,y++,2,"%04x",*i);
  }
  wrefresh(bpWin);
}

void Vt100Sim::snapMemory() {
  memset(touched+0x2000,0,0x2000);
}

void Vt100Sim::dispMemory() {
  int my,mx;
  getmaxyx(memWin,my,mx);
  int bavail = (mx - 7)/3;
  int bdisp = 8;
  while (bdisp*2 <= bavail) bdisp*=2;
  uint16_t start = 0x2000;
  
  wattrset(memWin,A_NORMAL);
  for (int b = 0; b<bdisp;b++) {
    mvwprintw(memWin,0,7+3*b,"%02x",b);
  }
  wattrset(memWin,COLOR_PAIR(1));

  for (int y = 1; y < my - 1; y++) {
    wattrset(memWin,COLOR_PAIR(1));
    mvwprintw(memWin,y,1,"%04x:",start);
    for (int b = 0; b<bdisp;b++) {
      if (!touched[start]) 
	wattron(memWin,A_STANDOUT);
      if (ram[start] != 00) {
	wattron(memWin,COLOR_PAIR(2));
	wprintw(memWin," %02x",ram[start++]);
	wattron(memWin,COLOR_PAIR(1));
      } else {
	wprintw(memWin," %02x",ram[start++]);
      }
      wattroff(memWin,A_STANDOUT);
    }
  }
  wrefresh(memWin);
}

extern "C" {
BYTE io_in(BYTE addr);
void io_out(BYTE addr, BYTE data);
void exit_io();
}

void exit_io() {}

BYTE io_in(BYTE addr)
{
    return sim->ioIn(addr);
}

void io_out(BYTE addr, BYTE data)
{
    sim->ioOut(addr,data);
}
