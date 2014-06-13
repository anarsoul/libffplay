.PHONY: all

all: libffplay.so

PREFIX:=/usr/local
LIBDIR:=${PREFIX}/lib
CFLAGS+=$(shell pkg-config --cflags libavformat libavcodec libswscale libavutil libavfilter libswresample libavdevice)
LDFLAGS+=$(shell pkg-config --libs libavformat libavcodec libswscale libavutil libavfilter libswresample libavdevice) -lm -pthread
LIBFFPLAY_CFLAGS+=-fpic -shared -g
CC:=c99

LIBFFPLAY_SRC=libffplay.c
LIBFFPLAY_OBJ=${LIBFFPLAY_SRC:.c=.o}

libffplay.so: ${LIBFFPLAY_OBJ}
	${CC} -Wall -o $@ ${LIBFFPLAY_OBJ} ${LDFLAGS} ${CFLAGS} ${LIBFFPLAY_CFLAGS}

%.o : %.c
	${CC} -Wall ${CFLAGS} ${LIBFFPLAY_CFLAGS} -c -o $@ $<

clean:
	${RM} ${LIBFFPLAY_OBJ} libffplay.so

install: libffplay.so
	install -d ${DESTDIR}${LIBDIR}
	install -m755 libffplay.so ${DESTDIR}${LIBDIR}/

