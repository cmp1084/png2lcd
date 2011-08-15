
PROGRAM = png2lcd

ifndef $(INSTALLDIR)
	INSTALLDIR = /usr/local/sbin
#endif

SRCDIR = src
INCDIR = include

CC = gcc

CFLAGS = -Os
CFLAGS += -lpng
CFLAGS += -ggdb

LDFLAGS =

SOURCES = $(SRCDIR)/main.c
OBJ = ${SOURCES:%.c=%.o}


all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o$(PROGRAM)

install:
	cp $(PROGRAM) $(INSTALLDIR)

clean:
	rm -rf *.o $(PROGRAM)
