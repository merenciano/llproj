SHELL = /bin/sh

SRC = ${wildcard ./src/*.c}
OBJS = ${SRC:.c=.o} 
CFLAGS = -Wall
CC = gcc
INCLUDES = 
LIBS = -lSDL2

emu:${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

.PHONY: clean
clean:
	rm -f ${OBJS} emu
