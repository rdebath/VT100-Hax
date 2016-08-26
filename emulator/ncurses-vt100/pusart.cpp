#include "pusart.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

PUSART::PUSART() : 
  mode_select_mode(true),
  has_xmit_ready(true),
  mode(0),
  command(0),
  pty_fd(-1),
  has_rx_rdy(false),
  child_pid(0)
{
}

void PUSART::start_shell() {
  if (pty_fd != -1) close(pty_fd);

  pty_fd = posix_openpt( O_RDWR | O_NOCTTY );
  grantpt(pty_fd);
  unlockpt(pty_fd);
  int flags = fcntl(pty_fd, F_GETFL, 0);
  fcntl(pty_fd, F_SETFL, flags | O_NONBLOCK);

  struct termios config;
  struct termios orig_settings;

  if(!isatty(pty_fd)) {}

  int fds = open(ptsname(pty_fd), O_RDWR);
  if(tcgetattr(fds, &orig_settings) < 0) {}

  if(tcgetattr(pty_fd, &config) < 0) {}
  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
		      INLCR | PARMRK | INPCK | ISTRIP | IXON);
  config.c_oflag = 0;
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 0;
  if(tcsetattr(pty_fd, TCSAFLUSH, &config) < 0) {}

  struct winsize ws = {0};
  ws.ws_row = 24; ws.ws_col = 80;
  ioctl(pty_fd, TIOCSWINSZ, &ws);

  child_pid = fork();

  if (child_pid == 0) {
    // Child process.
    close(pty_fd);  // Close master

    config = orig_settings;
    config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
			INLCR | PARMRK | INPCK | ISTRIP | IXON);
    config.c_oflag = 0;
    config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    config.c_cflag &= ~(CSIZE | PARENB);
    config.c_cflag |= CS8;
    config.c_cc[VMIN]  = 1;
    config.c_cc[VTIME] = 0;
    if(tcsetattr(fds, TCSANOW, &config) < 0) {}

    // Reopen stdio to slave pty.
    close(0); close(1); close(2);
    dup(fds); dup(fds); dup(fds);

    setsid();
    ioctl(0, TIOCSCTTY, 1);
    if(tcsetattr(fds, TCSANOW, &orig_settings) < 0) {}
    close(fds);

    /* The VT100 is not multi language */
    unsetenv("LANG");
    setenv("TERM", "vt100", 1);

    char * shell = getenv("SHELL");
    if (shell && *shell)
      execl(shell, shell, 0);
    execl("/bin/sh", "/bin/sh", 0);

    exit(128);
  }

  // Don't need the slave here
  close(fds);
}

bool PUSART::xmit_ready() { return has_xmit_ready; }

void PUSART::write_command(uint8_t cmd) {
  /*
   * At reset the device is in 'mode select' mode and the next command byte
   * is a 'mode' byte that describes the character format.
   *
   * After that the device is it's normal mode with the following flags:
   * 0: Transmit Enable
   * 1: DTR Line
   * 2: Receive Enable
   * 3: Send Break
   * 4: Error reset (clear serial error indicators)
   * 5: RTS Line
   * 6: RESET -> Next command must be a mode as at reset.
   * 7: "Hunt Mode", for Sync communications not normal RS-232 async
   *
   * The VT100 asserts RTS and TE at all times.
   * It asserts DTR when the 'Online' flag is active.
   * It asserts the RE at all times except during a 'hang-up'
   *
   * Command 0x2f is BREAK  (cmd & 0x08)
   * Command 0x2d is hangup (cmd & 0x02) == 0
   * Command 0x25 is Offline
   * Command 0x27 is Online
   * Command 0x40 is RESET
   *
   * The two LSB of the 'mode' byte are a divisor for the input clock.
   * The VT100 only ever uses XXXXXX10, or x16.
   */

#if 0
  // Decode command bytes ...
  if (mode_select_mode) {
    fprintf(stderr, "PSUART MODE 0x%02x"
	    ", STOP %s" ", PARITY %s" ", %s" ", CHAR %s" ", CLK %s" "\n",
      cmd,
      (cmd&0xC0) == 0x00 ? "invalid":
      (cmd&0xC0) == 0x40 ? "1 bit":
      (cmd&0xC0) == 0x80 ? "1.5 bit":
      (cmd&0xC0) == 0xC0 ? "2 bit": "",
      (cmd&0x20) == 0 ? "odd": "even",
      (cmd&0x10) == 0 ? "disabled": "enabled",
      (cmd&0x0C) == 0x00 ? "5 bits":
      (cmd&0x0C) == 0x04 ? "6 bits":
      (cmd&0x0C) == 0x08 ? "7 bits":
      (cmd&0x0C) == 0x0C ? "8 bits": "",
      (cmd&0x03) == 0x00 ? "SYNC":
      (cmd&0x03) == 0x01 ? "x1":
      (cmd&0x03) == 0x02 ? "x16":
      (cmd&0x03) == 0x03 ? "x64": "");
  } else {
    fprintf(stderr, "PSUART CMD 0x%x%s%s%s%s%s%s%s%s\n",
      cmd,
      (cmd&1)?"":", !TE",
      (cmd&2)?", DTR":", !DTR",
      (cmd&4)?"":", !RE",
      (cmd&8)?", BREAK":"",
      (cmd&0x10)?", CLR":"",
      (cmd&0x20)?"":", !RTS",
      (cmd&0x40)?", RESET":"",
      (cmd&0x80)?", SYNC":"");
  }
#endif

  if (mode_select_mode) {
    mode_select_mode = false;
    mode = cmd; // like we give a wet turd
  } else {
    command = cmd;
    if (cmd & 1<<6) { // INTERNAL RESET
      mode_select_mode = true;
    }
    // Start the shell when the VT100 first goes online.
    if ((cmd & 3) == 3 && pty_fd == -1)
      start_shell();

    if (cmd == 0x2d && pty_fd != -1) {
	close(pty_fd);
	pty_fd = -1;
    }
  }
}

void PUSART::write_data(uint8_t dat) {
  if ((dat>0 && dat<' ') || dat == '\177') {
    struct termios config;
    if(tcgetattr(pty_fd, &config) >= 0) {
      if (dat == config.c_cc[VINTR])
	tcflush(pty_fd, TCIOFLUSH);
    }
    if (dat == '\023' || dat == '\021') {
	xoff = (dat == '\023');
	// fprintf(stderr, "PSUART FLOW %s\n", xoff?"xoff":"xon");
	return;
    }
  }
  if (write(pty_fd,&dat,1) < 0) {
    close(pty_fd);
    pty_fd = -1;
  }
}

void PUSART::write_cols(bool cols132) {
  if (pty_fd == -1) return;
  if (cols132 == iscols132) return;
  iscols132 = cols132;

  struct winsize ws = {0};
  ws.ws_row = 24; ws.ws_col = cols132?132:80;
  ioctl(pty_fd, TIOCSWINSZ, &ws);
}

uint8_t PUSART::read_command() {
  /// always indicate data set ready
  bool tx_empty = false;
  bool sync_det = true;
  bool tx_rdy = true;
  bool rx_rdy = has_rx_rdy;
  return 0x80 | sync_det?0x40:0 | tx_empty?0x04:0 | rx_rdy?0x02:0 | tx_rdy?0x01:0;
}

bool PUSART::clock() {
  char c;
  if (has_rx_rdy || xoff || pty_fd == -1) return false;
  int i = read(pty_fd,&c,1);
  if (i != -1) {
    data = c;
    has_rx_rdy = true;
    return true;
  }
  int stat, r = waitpid(child_pid, &stat, WNOHANG);
  if (r != 0) {
    if (r<0 || r == child_pid) {
	close(pty_fd);
	child_pid = pty_fd = -1;
	data = 0;
	return true;
    }
  }
  return false;
}

uint8_t PUSART::read_data() {
  has_rx_rdy = false;
  return data;
}

char* PUSART::pty_name() {
  if (pty_fd == -1)
    return (char*)"<NONE>";
  return ptsname(pty_fd);
}

int PUSART::shell_pid() {
    if (child_pid<0) { child_pid=0; return -1; }
    return child_pid;
}
