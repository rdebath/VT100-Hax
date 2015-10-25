TARGET=vt100sim

LIBS=-lncurses

OBJS=main.o nvr.o keyboard.o vt100sim.o \
	8080/simglb.o \
	8080/sim1a.o \
	8080/simint.o \
	pusart.o

ROMPATH=../../ROMs

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ -o $(TARGET) $^ $(LIBS)

$(OBJS): keyboard.h nvr.h optionparser.h pusart.h vt100sim.h

vt100sim.o: default_rom.h

clean:
	@-rm -f $(OBJS) $(TARGET) default_rom.h

wide:
	$(MAKE) all CXXFLAGS=-I/usr/include/ncursesw LIBS=-lncursesw

default_rom.h:  $(ROMPATH)/23-061E2.bin $(ROMPATH)/23-032E2.bin \
		$(ROMPATH)/23-033E2.bin $(ROMPATH)/23-034E2.bin
	echo 'char default_rom[] = {' > _romtmp.h
	xxd -i < $(ROMPATH)/23-061E2.bin >> _romtmp.h
	echo ',' >> _romtmp.h
	xxd -i < $(ROMPATH)/23-032E2.bin >> _romtmp.h
	echo ',' >> _romtmp.h
	xxd -i < $(ROMPATH)/23-033E2.bin >> _romtmp.h
	echo ',' >> _romtmp.h
	xxd -i < $(ROMPATH)/23-034E2.bin >> _romtmp.h
	echo '};' >> _romtmp.h
	echo '#define DEFAULT_ROM default_rom' >> _romtmp.h
	mv _romtmp.h default_rom.h
