/*
 * input.h - Reading what the players type, one whole line at a time.
 */
#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>

typedef enum { INPUT_OK, INPUT_BAD_FORMAT, INPUT_EOF } InputStatus;

InputStatus input_read_move(int *r1, int *c1, int *r2, int *c2);
InputStatus input_read_yes_no(bool *yes);

InputStatus input_read_number(int *value);

/* Returns true if the line holds exactly 4 whole numbers. */
bool input_parse_move(const char *line, int out[4]);

#endif
