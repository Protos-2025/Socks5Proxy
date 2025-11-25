CC = gcc
CFLAGS = -g -fsanitize=address -std=c11 -Wall -D_XOPEN_SOURCE=600
INCLUDES = -I../../src/client/include -I../../src/server/include -I../../src/shared/include
LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit
BIN_PATH = ../../bin
SHARED_PATH = ../../src/shared

ifneq ($(DEBUG), 0)
  CFLAGS += -g -DDEBUG
else
  CFLAGS += -O2
endif
