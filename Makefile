OBJS = hellompi.o relaxation.o util.o
SRCS = hellompi.c relaxation.c util.c
CC = mpicc

all: re

re:    ${OBJS}
	${CC} -o re ${OBJS} -lm -lmpi

.c.o:
	${CC} -g -Wall -c $*.c

clean:
	rm *.o re

