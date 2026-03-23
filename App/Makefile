CC=gcc
CFLAGS+=$(shell pkg-config --cflags gtk+-3.0 --cflags gtkdatabox --cflags freetype2)
LDFLAGS+=$(shell pkg-config --libs gtk+-3.0 --libs gtkdatabox --libs freetype2)
UNAME:=$(shell uname)

ifeq ($(UNAME),Linux)
LDFLAGS+=-lm -Wl,-rpath=/lib64
endif 

all: app

%.o: %.c
	${CC} ${CFLAGS} -c $? -o $@

app:    gtkapp.o \
	signals.o \
	stack.o \
	plot.o \
	loop.o \
	widgets.o \
	speed_widget.o \
	weather_widget.o \
	throttle.o \
	socket.o
	${CC} ${CFLAGS} $? ${LDFLAGS} -rdynamic -o $@

clean:
	rm -f *.o
	rm -f app
