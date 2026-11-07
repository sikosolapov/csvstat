CC     ?= cc
TARGET := csvstat
MODE   ?= debug
V      ?= 0

# --- исходники --------------------------------------------------------------
# main.c и fault.c лежат в корне, остальное - в function/ (и подпапках).
SRC := main.c fault.c $(wildcard function/*.c function/*/*.c)

# --- флаги ------------------------------------------------------------------
STD      := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Werror
DEPFLAGS := -MMD -MP
INCLUDES := -I. -Ifunction
LDLIBS   := -lm

ifeq ($(MODE),release)
BUILD_DIR  := build/release
MODE_FLAGS := -O2 -DNDEBUG
else ifeq ($(MODE),debug)
BUILD_DIR  := build/debug
# -fno-sanitize-recover: UB не "просто печатается", а завершает программу
# с ненулевым кодом, поэтому тест реально падает.
MODE_FLAGS := -g -O0 -fsanitize=address,undefined -fno-sanitize-recover=all \
              -fno-omit-frame-pointer
else
$(error MODE должен быть debug или release, а не '$(MODE)')
endif

EXTRA_CFLAGS ?=
CFLAGS  := $(STD) $(WARNINGS) $(MODE_FLAGS) $(DEPFLAGS) $(INCLUDES) $(EXTRA_CFLAGS)
LDFLAGS := $(MODE_FLAGS)

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
DEPS := $(OBJS:.o=.d)
BIN  := $(BUILD_DIR)/$(TARGET)

TESTS := $(sort $(wildcard tests/*.sh))

# --- тише/громче ------------------------------------------------------------
ifeq ($(V),1)
Q :=
else
Q := @
endif

# --- общие настройки make ---------------------------------------------------
MAKEFLAGS += --no-builtin-rules
.DELETE_ON_ERROR:
.PHONY: all release test test-all bench clean help

# --- сборка -----------------------------------------------------------------
all: $(BIN)
	$(Q)ln -sf $(BIN) $(TARGET)

release:
	$(Q)$(MAKE) --no-print-directory MODE=release all

$(BIN): $(OBJS)
	@echo "LD  $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# --- тесты ------------------------------------------------------------------
# Каждому скрипту через окружение передаётся путь к бинарнику: BIN.
test: all
	@if [ -z "$(TESTS)" ]; then echo "tests/*.sh не найдены"; exit 0; fi
	@for t in $(TESTS); do \
		echo "== $$t ($(MODE))"; \
		BIN=$(BIN) sh "$$t" || exit 1; \
	done

test-all:
	$(Q)$(MAKE) --no-print-directory MODE=debug test
	$(Q)$(MAKE) --no-print-directory MODE=release test

# --- производительность (ТЗ 10.6) -------------------------------------------
# Всегда release: санитайзеры искажают время. tests/perf.sh сравнивает
# `stats table.csv` и `bin-stats table.bin`.
bench:
	$(Q)$(MAKE) --no-print-directory MODE=release all
	@if [ -f tests/perf.sh ]; then \
		BIN=build/release/$(TARGET) sh tests/perf.sh; \
	else \
		echo "tests/perf.sh не найден: создайте скрипт замера"; \
	fi

# --- служебное --------------------------------------------------------------
clean:
	rm -rf build $(TARGET)

help:
	@sed -n '2,20p' Makefile | sed 's/^# \{0,1\}//'

-include $(DEPS)
