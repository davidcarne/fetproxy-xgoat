CFLAGS := -Wall -g

CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CFLAGS += `pkg-config --cflags gobject-2.0`
LDFLAGS += `pkg-config --libs gobject-2.0`

LDFLAGS += -lelf

fetproxy: fetproxy.o fet-module.o xb-fd-source.o crc.o fet-commands.o elf-access.o

.PHONY: clean

clean:
	-rm -f fetproxy *.o

