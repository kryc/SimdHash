TARGET_LIB ?= simdhash.a

BUILD_DIR ?= build
SRC_DIRS ?= .
C ?= gcc

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)  # one dependency file for each source

INC_DIRS := ./
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS ?= $(INC_FLAGS) -Wall -Werror -mavx2 -O3 -funroll-loops
LDFLAGS = -mavx2

.PHONY: simdhash.a
simdhash.a: $(OBJS)
	ar rvs $(TARGET_LIB) $(OBJS)

-include $(DEPS)   # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	$(C) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET_LIB) $(DEPS)

.PHONY: all
all: $(TARGET_LIB)