# CMPE322 Operating Systems Project: Shooter
# Author: Mehmet Saraçoğlu
# Designed as course material for use in the CMPE322 Operating Systems lectures.

# Targets:
#   make / make fifo   student input source
#   make keyboard      keyboard input source
#   make run ID=<id>   build, then play
#   make clean         remove build outputs

CC     := gcc
CFLAGS ?= -O2 -Wall -Wextra
IMPL   := implementation
BUILD  := build

SRCS  := ship.c planet.c bullet.c station.c entity.c space.c level.c renderer.c realtime.c window.c \
         control.c hud.c main.c
OBJS  := $(SRCS:%.c=$(BUILD)/%.o)
# Student outputs stay in implementation/.
PARTS := $(IMPL)/libfifo_management.so $(IMPL)/shield $(IMPL)/gather

# Required student sources.
STUDENT_SRCS := $(IMPL)/input_student.c $(IMPL)/fifo_management.c $(IMPL)/shield.c $(IMPL)/gather.c
MISSING      := $(filter-out $(wildcard $(STUDENT_SRCS)),$(STUDENT_SRCS))

fifo: check $(OBJS) $(IMPL)/input_student.o $(PARTS)
	$(CC) $(CFLAGS) -o game $(filter %.o,$^) -lncurses -ldl

keyboard: $(OBJS) $(BUILD)/input_keyboard.o
	$(CC) $(CFLAGS) -o game $^ -lncurses

check:
ifneq ($(MISSING),)
	$(error missing in $(IMPL)/: $(notdir $(MISSING)). Use 'make keyboard' to build without them)
endif

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(IMPL)/input_student.o: $(IMPL)/input_student.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(IMPL)/libfifo_management.so: $(IMPL)/fifo_management.c $(IMPL)/fifo_management.h input.h
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

$(IMPL)/shield $(IMPL)/gather: %: %.c $(IMPL)/fifo_management.h input.h
	$(CC) $(CFLAGS) -o $@ $<

run: fifo
	@[ -n "$(ID)" ] || { echo "usage: make run ID=<student_id>" >&2; exit 1; }
	./game "$(ID)" levels/level01.txt

clean:
	rm -rf game $(BUILD) $(PARTS) $(IMPL)/input_student.o $(IMPL)/input_student.d $(IMPL)/game.fifo

.PHONY: fifo keyboard check run clean

-include $(OBJS:.o=.d) $(BUILD)/input_keyboard.d $(IMPL)/input_student.d
