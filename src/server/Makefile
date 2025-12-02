include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard states/*.c) $(wildcard pam/*.c)
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
	rm -f $(EXEC) $(OBJS) states/*.o

.PHONY: clean all
