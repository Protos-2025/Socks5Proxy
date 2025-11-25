include ../Makefile.inc

SHARED_PATH = ../src/shared
SHARED_SRCS = $(filter-out %_test.c, $(wildcard $(SHARED_PATH)/*.c))
SHARED_OBJS = $(SHARED_SRCS:.c=.o)

TEST_SRCS = $(wildcard *_test.c)
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_EXECS = $(TEST_SRCS:.c=)

TEST_STANDALONE = $(wildcard *_test_standalone.c)
TEST_STANDALONE_OBJS = $(TEST_STANDALONE:.c=.o)
TEST_STANDALONE_EXECS = $(TEST_STANDALONE:.c=)

CFLAGS += $(shell pkg-config --cflags check)
LDFLAGS += $(shell pkg-config --libs check) -g -DDEBUG

BIN_PATH = ../bin/tests

INCLUDES = -I../src/client/include -I../src/server/include -I../src/shared/include -fsanitize=address

all: tests_bin $(TEST_EXECS) $(TEST_STANDALONE_EXECS)

tests_bin: 
	mkdir -p $(BIN_PATH)

%_test: %_test.o $(SHARED_OBJS)
	$(CC) $(CFLAGS) $^ -o $(BIN_PATH)/$@ $(LDFLAGS)

%_test_standalone: %_test_standalone.o
	$(CC) $(CFLAGS) $^ -o $(BIN_PATH)/$@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TEST_OBJS) $(TEST_EXECS)

.PHONY: clean all
