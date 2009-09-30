CFLAGS := -Wall -g -D_GNU_SOURCE

CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CFLAGS += `pkg-config --cflags gobject-2.0`
LDFLAGS += `pkg-config --libs gobject-2.0`

CFLAGS += `pkg-config --cflags gnet-2.0`
LDFLAGS += `pkg-config --libs gnet-2.0`

LDFLAGS += -lelf

fetproxy: fetproxy.o fet-module.o crc.o fet-commands.o elf-access.o serial.o \
	gdb-remote.o gdb-client.o

.PHONY: clean

clean:
	-rm -f fetproxy *.o

