OBJS = relaxation.o
SRCS = relaxation.c
CC = gcc

all: re

re:    ${OBJS}
	${CC} -g -Wall -o re ${OBJS} -lm

.c.o:
	${CC} -g -Wall -c $*.c

clean:
	rm *.o;rm re
