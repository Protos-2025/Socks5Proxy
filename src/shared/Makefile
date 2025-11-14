include ../../Makefile.inc

# Only compile source files, not tests
SRCS = $(filter-out %_test.c, $(wildcard *.c))
OBJS = $(SRCS:.c=.o)

INCLUDES = -Iinclude

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

.PHONY: clean all
