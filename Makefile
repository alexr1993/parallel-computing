OBJS = hellompi.o
SRCS = hellompi.c
CC = mpicc

all: re

re:    ${OBJS}
	${CC} -o re ${OBJS} -lm -lmpi

.c.o:
	${CC} -g -Wall -c $*.c

clean:
	rm *.o re

