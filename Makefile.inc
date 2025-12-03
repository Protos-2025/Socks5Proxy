CC = gcc
CFLAGS = -g -std=c11 -Wall -D_XOPEN_SOURCE=600
INCLUDES = -I../../src/client/include -I../../src/server/include -I../../src/shared/include
LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit
BIN_PATH = ../../bin
SHARED_PATH = ../../src/shared

ifneq ($(DEBUG), 0)
  CFLAGS += -g -DDEBUG -fsanitize=address
  LDFLAGS += -fsanitize=address
else
  CFLAGS += -O3 -DNO_COLOR_LOGS
endif

# Override constants w/ env vars:
ifdef BUFFER_SIZE
  CFLAGS += -DBUFFER_SIZE=$(BUFFER_SIZE)
endif

ifdef LOGGER_MIN_LEVEL
  CFLAGS += -DLOGGER_MIN_LEVEL=$(LOGGER_MIN_LEVEL)
endif

ifdef MAX_LOG_QUEUE_SIZE
  CFLAGS += -DMAX_LOG_QUEUE_SIZE=$(MAX_LOG_QUEUE_SIZE)
endif
