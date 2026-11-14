/*
 * input.c - Line-based input parsing (see input.h).
 */
/* isatty() is POSIX, not ISO C, so ask for POSIX declarations explicitly
 * (needed because we compile with -std=c17 -pedantic). */
#define _POSIX_C_SOURCE 200809L

#include "input.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX_LEN 128

/* Reads one line into buf (without the '\n').
 * Returns INPUT_EOF at end of input, INPUT_BAD_FORMAT if the line was too
 * long (the rest of it is thrown away), INPUT_OK otherwise. */
static InputStatus read_line(char *buf, size_t size) {
  if (fgets(buf, (int)size, stdin) == NULL) {
    return INPUT_EOF;
  }
  /* When input comes from a file/pipe (tests, Valgrind runs) the terminal
   * doesn't echo it, so print it ourselves to keep transcripts readable. */
  if (!isatty(STDIN_FILENO)) {
    fputs(buf, stdout);
    if (strchr(buf, '\n') == NULL) {
      putchar('\n');
    }
  }
  char *newline = strchr(buf, '\n');
  if (newline != NULL) {
    *newline = '\0';
    return INPUT_OK;
  }
  if (feof(stdin)) {
    return INPUT_OK; /* last line of a file with no trailing newline */
  }
  /* Line longer than the buffer: discard the rest so it can't be
   * mistaken for the next move. */
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF) {
  }
  return INPUT_BAD_FORMAT;
}

bool input_parse_move(const char *line, int out[4]) {
  const char *p = line;
  for (int i = 0; i < 4; i++) {
    while (isspace((unsigned char)*p) || *p == ',') {
      p++;
    }
    char *end;
    errno = 0;
    long value = strtol(p, &end, 10);
    if (end == p || errno == ERANGE) {
      return false; /* not a number, or does not fit in a long */
    }
#if LONG_MAX > INT_MAX
    /* Only needed where long is wider than int (64-bit). Guarded so 32-bit
     * builds don't warn "comparison is always false" under -Werror. */
    if (value < INT_MIN || value > INT_MAX) {
      return false;
    }
#endif
    out[i] = (int)value;
    p = end;
  }
  /* Only whitespace may follow the 4th number. */
  while (isspace((unsigned char)*p)) {
    p++;
  }
  return *p == '\0';
}

InputStatus input_read_move(int *r1, int *c1, int *r2, int *c2) {
  char buf[LINE_MAX_LEN];
  InputStatus status = read_line(buf, sizeof(buf));
  if (status != INPUT_OK) {
    return status;
  }
  int v[4];
  if (!input_parse_move(buf, v)) {
    return INPUT_BAD_FORMAT;
  }
  *r1 = v[0];
  *c1 = v[1];
  *r2 = v[2];
  *c2 = v[3];
  return INPUT_OK;
}

InputStatus input_read_yes_no(bool *yes) {
  char buf[LINE_MAX_LEN];
  InputStatus status = read_line(buf, sizeof(buf));
  if (status == INPUT_EOF) {
    return status;
  }
  const char *p = buf;
  while (isspace((unsigned char)*p)) {
    p++;
  }
  *yes = (*p == '\0' || *p == 'y' || *p == 'Y');
  return INPUT_OK;
}
