OBJS = main.o relaxation.o util.o messaging.o
SRCS = main.c relaxation.c util.c messaging.c
CC = mpicc

all: re

re:    ${OBJS}
	${CC} -o re ${OBJS} -lm -lmpi

.c.o:
	${CC} -g -Wall -c $*.c

clean:
	rm *.o re

