.PHONY: ffplay-sdl all

all: libffplay.so

PREFIX:=/usr/local
LIBDIR:=${PREFIX}/lib
CFLAGS+=$(shell pkg-config --cflags libavformat libavcodec libswscale libavutil libavfilter libswresample)
LDFLAGS+=$(shell pkg-config --libs libavformat libavcodec libswscale libavutil libavfilter libswresample) -lm -pthread
LIBFFPLAY_CFLAGS+=-fpic -shared
FFPLAY_SDL_CFLAGS+=$(shell pkg-config --cflags sdl)
FFPLAY_SDL_LDFLAGS+=$(shell pkg-config --libs sdl) -lffplay
CC:=c99

LIBFFPLAY_SRC=libffplay.c
LIBFFPLAY_OBJ=${LIBFFPLAY_SRC:.c=.o}

FFPLAY_SDL_SRC=examples/ffplay-sdl.c
FFPLAY_OBJ=${FFPLAY_SDL_SRC:.c=.o}

libffplay.so: ${LIBFFPLAY_OBJ}
	${CC} -pedantic -Wall -o $@ ${LIBFFPLAY_OBJ} ${LDFLAGS} ${CFLAGS} ${LIBFFPLAY_CFLAGS}

ffplay-sdl: examples/ffplay-sdl

examples/ffplay-sdl: ${FFPLAY_SDL_OBJ}
	${CC} -pedantic -Wall -o $@ ${FFPLAY_SDL_OBJ} ${FFPLAY_SDL_CFLAGS} ${FFPLAY_SDL_LDFLAGS}

%.o : %.c
	${CC} -pedantic -Wall ${CFLAGS} ${LIBFFPLAY_CFLAGS} -c -o $@ $<

clean:
	${RM} ${LIBFFPLAY_OBJ} libffplay.so

install: mouse-emul
	install -d ${DESTDIR}${LIBDIR}
	install -m755 libffplay.so ${DESTDIR}${LIBDIR}/

