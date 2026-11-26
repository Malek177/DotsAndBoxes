/*
 * input.c - Reading and checking what the players type.
 */
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

/* Reads one whole line. Lines that are too long are thrown away. */
static InputStatus read_line(char *buf, size_t size) {
  if (fgets(buf, (int)size, stdin) == NULL) {
    return INPUT_EOF;
  }
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
    return INPUT_OK;
  }
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF) {
  }
  return INPUT_BAD_FORMAT;
}

/* Reads exactly 4 numbers; anything else is rejected. */
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
      return false;
    }
#if LONG_MAX > INT_MAX
    if (value < INT_MIN || value > INT_MAX) {
      return false;
    }
#endif
    out[i] = (int)value;
    p = end;
  }
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

InputStatus input_read_number(int *value) {
  char buf[LINE_MAX_LEN];
  InputStatus status = read_line(buf, sizeof(buf));
  if (status != INPUT_OK) {
    return status;
  }
  char *end;
  errno = 0;
  long v = strtol(buf, &end, 10);
  if (end == buf || errno == ERANGE) {
    return INPUT_BAD_FORMAT;
  }
#if LONG_MAX > INT_MAX
  if (v < INT_MIN || v > INT_MAX) {
    return INPUT_BAD_FORMAT;
  }
#endif
  while (isspace((unsigned char)*end)) {
    end++;
  }
  if (*end != '\0') {
    return INPUT_BAD_FORMAT;
  }
  *value = (int)v;
  return INPUT_OK;
}
