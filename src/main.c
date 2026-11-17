/*
 * main.c - Entry point for Dots and Boxes (CMPS 241 project, Sprint 1).
 *
 * Usage: dotsandboxes [--plain] [--rows N] [--cols N] [--help]
 *   --plain    no colours / no screen clearing (used by tests and Valgrind)
 *   --rows N   number of box rows    (default 4, range 1..9)
 *   --cols N   number of box columns (default 5, range 1..9)
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "display.h"
#include "game.h"
#include "input.h"

static void print_usage(const char *prog) {
  printf("Usage: %s [--plain] [--rows N] [--cols N]\n", prog);
  printf("  --plain    no colours or screen clearing\n");
  printf("  --rows N   box rows    (%d..%d, default %d)\n", MIN_SIZE, MAX_SIZE,
         DEFAULT_ROWS);
  printf("  --cols N   box columns (%d..%d, default %d)\n", MIN_SIZE, MAX_SIZE,
         DEFAULT_COLS);
}

/* Parses a board dimension. Returns -1 if text is not a valid size. */
static int parse_size(const char *text) {
  char *end;
  long value = strtol(text, &end, 10);
  if (end == text || *end != '\0' || value < MIN_SIZE || value > MAX_SIZE) {
    return -1;
  }
  return (int)value;
}

int main(int argc, char *argv[]) {
  bool fancy = true;
  int rows = DEFAULT_ROWS;
  int cols = DEFAULT_COLS;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--plain") == 0) {
      fancy = false;
    } else if ((strcmp(argv[i], "--rows") == 0 || strcmp(argv[i], "--cols") == 0) &&
               i + 1 < argc) {
      int value = parse_size(argv[i + 1]);
      if (value < 0) {
        fprintf(stderr, "Invalid size '%s' (must be %d..%d).\n", argv[i + 1],
                MIN_SIZE, MAX_SIZE);
        return EXIT_FAILURE;
      }
      if (argv[i][2] == 'r') {
        rows = value;
      } else {
        cols = value;
      }
      i++; /* skip the number we just consumed */
    } else if (strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    } else {
      fprintf(stderr, "Unknown option '%s'.\n", argv[i]);
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  display_init(fancy, rows, cols);

  for (;;) {
    GameResult result = game_play(rows, cols);
    if (result == GAME_ERROR) {
      return EXIT_FAILURE;
    }
    if (result == GAME_ABORTED) {
      return EXIT_SUCCESS;
    }
    display_play_again();
    bool again;
    if (input_read_yes_no(&again) == INPUT_EOF || !again) {
      printf("\nThanks for playing!\n");
      return EXIT_SUCCESS;
    }
  }
}
