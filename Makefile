# Makefile для csvstat
#
#   make                 отладочная сборка с ASan и UBSan -> build/debug/csvstat
#   make MODE=release    сборка -O2 без санитайзеров      -> build/release/csvstat
#   make release         то же, что MODE=release
#   make test            запустить tests/*.sh
#   make clean           удалить build/

CC     ?= cc
TARGET := csvstat
MODE   ?= debug

SRC := main.c fault.c $(wildcard function/*.c)

STD      := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Werror
DEPFLAGS := -MMD -MP

ifeq ($(MODE),release)
BUILD_DIR  := build/release
MODE_FLAGS := -O2 -DNDEBUG
else ifeq ($(MODE),debug)
BUILD_DIR  := build/debug
MODE_FLAGS := -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
else
$(error MODE должен быть debug или release, а не '$(MODE)')
endif

CFLAGS  := $(STD) $(WARNINGS) $(MODE_FLAGS) $(DEPFLAGS)
LDFLAGS := $(MODE_FLAGS)

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
DEPS := $(OBJS:.o=.d)

.PHONY: all release test clean

all: $(BUILD_DIR)/$(TARGET)

release:
	$(MAKE) MODE=release

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	@for t in tests/*.sh; do \
		[ -f "$$t" ] || continue; \
		sh "$$t" || exit 1; \
	done

clean:
	rm -rf build

-include $(DEPS)