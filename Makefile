# Makefile - Dots and Boxes (CMPS 241 project)
#
#   make            build the game  -> ./dotsandboxes
#   make test       build + run unit tests and scripted full-game tests
#   make valgrind   run full scripted games under Valgrind (must be clean)
#   make gdb        start the game inside GDB
#   make install    copy the game to /usr/local/bin (used on the VM)
#   make clean      remove everything that was built

CC = gcc
# -g -O0: debug info for GDB/Valgrind, no optimisation (Lecture 7).
# -Werror: any warning stops the build, so warnings are never ignored.
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c17 -g -O0
OUTPUT = dotsandboxes
PREFIX = /usr/local

OBJECTS = src/main.o src/game.o src/board.o src/bot.o src/display.o src/input.o
TEST_OBJECTS = tests/test_units.o src/board.o src/input.o

$(OUTPUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Each object depends on its .c file and every header it includes, so
# editing a header rebuilds exactly the files that use it.
src/main.o: src/main.c src/board.h src/display.h src/game.h src/input.h
	$(CC) $(CFLAGS) -c $< -o $@

src/game.o: src/game.c src/game.h src/board.h src/display.h src/input.h
	$(CC) $(CFLAGS) -c $< -o $@

src/board.o: src/board.c src/board.h
	$(CC) $(CFLAGS) -c $< -o $@

src/bot.o: src/bot.c src/bot.h src/board.h
	$(CC) $(CFLAGS) -c $< -o $@

src/display.o: src/display.c src/display.h src/board.h
	$(CC) $(CFLAGS) -c $< -o $@

src/input.o: src/input.c src/input.h
	$(CC) $(CFLAGS) -c $< -o $@

tests/test_units.o: tests/test_units.c src/board.h src/input.h
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

tests/test_units: $(TEST_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: all test valgrind gdb install clean

all: $(OUTPUT)

test: $(OUTPUT) tests/test_units
	./tests/test_units
	sh tests/run_scripted.sh ./$(OUTPUT)

valgrind: $(OUTPUT) tests/test_units
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
		./tests/test_units
	for f in tests/scripts/*.in; do \
		echo "== valgrind: $$f"; \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
			-q ./$(OUTPUT) --plain < $$f > /dev/null || exit 1; \
	done
	@echo "Valgrind: no errors, no leaks."

gdb: $(OUTPUT)
	gdb ./$(OUTPUT)

install: $(OUTPUT)
	install -m 755 $(OUTPUT) $(PREFIX)/bin/$(OUTPUT)

clean:
	@rm -f src/*.o tests/*.o tests/test_units $(OUTPUT)
