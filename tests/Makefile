include ../Makefile.inc

SHARED_PATH = ../src/shared
SHARED_SRCS = $(filter-out %_test.c, $(wildcard $(SHARED_PATH)/*.c))
SHARED_OBJS = $(SHARED_SRCS:.c=.o)

TEST_SRCS = $(wildcard *_test.c)
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_EXEC = $(BIN_PATH)/run_tests

INCLUDES = -I$(SHARED_PATH)/include
CFLAGS += $(shell pkg-config --cflags check)
LDFLAGS = $(shell pkg-config --libs check)

all: $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJS) $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TEST_OBJS) $(TEST_EXEC)

.PHONY: clean all
