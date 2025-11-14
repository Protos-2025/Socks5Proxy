include ../../Makefile.inc

SRCS = $(wildcard *.c)
SHARED_SRCS = $(wildcard $(SHARED_PATH)/*.c)
OBJS = $(SRCS:.c=.o)
SHARED_OBJS = $(SHARED_SRCS:.c=.o)
EXEC = $(BIN_PATH)/client

# Override INCLUDES for client-specific needs
INCLUDES = -Iinclude -I$(SHARED_PATH)/include

all: $(EXEC)

$(EXEC): $(OBJS) $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)

.PHONY: clean all
