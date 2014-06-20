.PHONY: all

all: libffplay.so

PREFIX:=/usr/local
LIBDIR:=${PREFIX}/lib
INCDIR:=${PREFIX}/include
LIBRARY_NAME=libffplay.so
SOVERSION:=0.1
FULL_LIBRARY_NAME=${LIBRARY_NAME}.${SOVERSION}
CFLAGS+=$(shell pkg-config --cflags libavformat libavcodec libswscale libavutil libavfilter libswresample libavdevice)
LDFLAGS+=$(shell pkg-config --libs libavformat libavcodec libswscale libavutil libavfilter libswresample libavdevice) -lm -pthread -Wl,-soname,${FULL_LIBRARY_NAME}
LIBFFPLAY_CFLAGS+=-fpic -shared -g -O2
CC:=c99
LN:=ln

LIBFFPLAY_SRC=libffplay.c
LIBFFPLAY_OBJ=${LIBFFPLAY_SRC:.c=.o}

${FULL_LIBRARY_NAME}: ${LIBFFPLAY_OBJ}
	${CC} -Wall -o $@ ${LIBFFPLAY_OBJ} ${LDFLAGS} ${CFLAGS} ${LIBFFPLAY_CFLAGS}

${LIBRARY_NAME}: ${FULL_LIBRARY_NAME}
	${LN} -fs ${FULL_LIBRARY_NAME} ${LIBRARY_NAME}


%.o : %.c
	${CC} -Wall ${CFLAGS} ${LIBFFPLAY_CFLAGS} -c -o $@ $<

clean:
	${RM} ${LIBFFPLAY_OBJ} libffplay.so

install: ${FULL_LIBRARY_NAME}
	install -d ${DESTDIR}${LIBDIR}
	install -d ${DESTDIR}${INCDIR}
	install -m755 ${FULL_LIBRARY_NAME} ${DESTDIR}${LIBDIR}/
	${LN} -fs ${FULL_LIBRARY_NAME} ${DESTDIR}${LIBDIR}/${LIBRARY_NAME}
	install -m644 libffplay.h ${DESTDIR}${INCDIR}/
