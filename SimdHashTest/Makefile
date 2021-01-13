TARGET ?= simdhashtest

BUILD_DIR ?= build
SRC_DIRS ?= .
CC ?= gcc

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)  # one dependency file for each source

INC_DIRS := ../SimdHash/
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS ?= $(INC_FLAGS) -Wall -mavx2 -O2
LDFLAGS = 
LIBA = ../SimdHash/simdhash.a

.PHONY: simdhashtest
simdhashtest: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBA) $(LDFLAGS)

-include $(DEPS)   # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)

.PHONY: all
all: $(TARGET)