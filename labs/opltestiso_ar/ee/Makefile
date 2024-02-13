EE_BIN = opltester.elf
EE_OBJS = main.o
EE_LIBS = -ldebug -lpatches -lxcdvd
EE_CFLAGS += -Wall -Werror

all: $(EE_BIN)

run: all
	ps2client -h 192.168.1.10 execee host:$(EE_BIN)

reset: clean
	ps2client -h 192.168.1.10 reset

clean:
	rm -f -r $(EE_OBJS) $(EE_BIN) *_irx.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
