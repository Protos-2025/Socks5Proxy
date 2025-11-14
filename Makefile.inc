CC = gcc
CFLAGS = -g -fsanitize=address -std=c99 -Wall
INCLUDES = -I../../src/client/include -I../../src/server/include -I../../src/shared/include
LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit
BIN_PATH = ../../bin
SHARED_PATH = ../../src/shared

ifneq ($(DEBUG), 0)
  CFLAGS += -g -DDEBUG
else
  CFLAGS += -O2
endif
