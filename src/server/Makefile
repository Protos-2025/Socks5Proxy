include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard http/*.c)
SHARED_SRCS = $(wildcard $(SHARED_PATH)/*.c)
OBJS = $(SRCS:.c=.o)
SHARED_OBJS = $(SHARED_SRCS:.c=.o)
EXEC = $(BIN_PATH)/server

# Override INCLUDES for server-specific needs
INCLUDES = -Iinclude -I$(SHARED_PATH)/include

all: $(EXEC)

$(EXEC): $(OBJS) $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS) http/*.o

.PHONY: clean all
