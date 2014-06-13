all: ffplay-sdl

PREFIX:=/usr/local
BINDIR:=${PREFIX}/bin
CC:=c99

FFPLAY_SDL_SRC=ffplay-sdl.c
FFPLAY_SDL_OBJ=${FFPLAY_SDL_SRC:.c=.o}

CFLAGS+=$(shell pkg-config --cflags sdl) -I.. -g
LDFLAGS+=$(shell pkg-config --libs sdl) -L.. -lffplay

ffplay-sdl: ${FFPLAY_SDL_OBJ}
	${CC} -Wall -o $@ ${FFPLAY_SDL_OBJ} ${LDFLAGS}

%.o : %.c
	${CC} -Wall -D_XOPEN_SOURCE ${CFLAGS} -c -o $@ $<

clean:
	${RM} ${FFPLAY_SDL_OBJ} ffplay-sdl

install: ffplay-sdl
	install -d ${DESTDIR}${BINDIR}
	install -m755 ffplay-sdl ${DESTDIR}${BINDIR}/
