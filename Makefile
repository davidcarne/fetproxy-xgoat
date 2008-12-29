CFLAGS := -Wall

CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CFLAGS += `pkg-config --cflags gobject-2.0`
LDFLAGS += `pkg-config --libs gobject-2.0`

fetproxy: fetproxy.o fet-module.o xb-fd-source.o crc.o

.PHONY: clean

clean:
	-rm -f fetproxy *.o

