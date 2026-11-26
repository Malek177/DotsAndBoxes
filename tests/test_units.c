/*
 * test_units.c - Unit tests for the board rules and the input parser.
 * Run with: make test
 */
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "input.h"

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    checks_run++;                                                      \
    if (!(cond)) {                                                     \
      checks_failed++;                                                 \
      printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
    }                                                                  \
  } while (0)

static int play(Board *b, int r1, int c1, int r2, int c2, Player p) {
  Line line;
  if (board_validate_move(b, r1, c1, r2, c2, &line) != MOVE_OK) {
    return -1;
  }
  return board_draw_line(b, line, p);
}

static void test_create(void) {
  printf("test_create\n");
  CHECK(board_create(0, 5) == NULL);
  CHECK(board_create(4, 0) == NULL);
  CHECK(board_create(MAX_SIZE + 1, 5) == NULL);
  CHECK(board_create(-3, 5) == NULL);

  Board *b = board_create(DEFAULT_ROWS, DEFAULT_COLS);
  CHECK(b != NULL);
  CHECK(board_rows(b) == 4);
  CHECK(board_cols(b) == 5);
  CHECK(!board_is_full(b));
  CHECK(board_score(b, PLAYER_A) == 0);
  CHECK(board_score(b, PLAYER_B) == 0);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 5; c++) {
      CHECK(board_box_owner(b, r, c) == PLAYER_NONE);
      CHECK(board_box_sides(b, r, c) == 0);
    }
  }
  board_destroy(b);
  board_destroy(NULL);
}

static void test_validate(void) {
  printf("test_validate\n");
  Board *b = board_create(4, 5);
  Line line;

  CHECK(board_validate_move(b, -1, 0, 0, 0, &line) == MOVE_OUT_OF_RANGE);
  CHECK(board_validate_move(b, 4, 5, 5, 5, &line) == MOVE_OUT_OF_RANGE);
  CHECK(board_validate_move(b, 0, 6, 0, 5, &line) == MOVE_OUT_OF_RANGE);
  CHECK(board_validate_move(b, 2, 2, 2, 2, &line) == MOVE_SAME_DOT);
  CHECK(board_validate_move(b, 0, 0, 1, 1, &line) == MOVE_NOT_ADJACENT);
  CHECK(board_validate_move(b, 0, 0, 0, 2, &line) == MOVE_NOT_ADJACENT);
  CHECK(board_validate_move(b, 0, 0, 2, 0, &line) == MOVE_NOT_ADJACENT);

  CHECK(board_validate_move(b, 4, 4, 4, 5, &line) == MOVE_OK);
  CHECK(board_validate_move(b, 3, 5, 4, 5, &line) == MOVE_OK);

  CHECK(board_validate_move(b, 0, 4, 0, 3, &line) == MOVE_OK);
  CHECK(line.orient == LINE_HORIZONTAL && line.r == 0 && line.c == 3);
  CHECK(board_validate_move(b, 0, 3, 0, 4, &line) == MOVE_OK);
  CHECK(line.orient == LINE_HORIZONTAL && line.r == 0 && line.c == 3);

  CHECK(board_validate_move(b, 2, 1, 1, 1, &line) == MOVE_OK);
  CHECK(line.orient == LINE_VERTICAL && line.r == 1 && line.c == 1);

  CHECK(play(b, 0, 3, 0, 4, PLAYER_A) == 0);
  CHECK(board_validate_move(b, 0, 4, 0, 3, &line) == MOVE_ALREADY_DRAWN);
  board_destroy(b);
}

static void test_single_box(void) {
  printf("test_single_box\n");
  Board *b = board_create(1, 1);
  CHECK(play(b, 0, 0, 0, 1, PLAYER_A) == 0);
  CHECK(board_box_sides(b, 0, 0) == 1);
  CHECK(play(b, 0, 0, 1, 0, PLAYER_B) == 0);
  CHECK(play(b, 1, 0, 1, 1, PLAYER_A) == 0);
  CHECK(board_box_sides(b, 0, 0) == 3);
  CHECK(!board_is_full(b));
  CHECK(play(b, 0, 1, 1, 1, PLAYER_B) == 1);
  CHECK(board_box_owner(b, 0, 0) == PLAYER_B);
  CHECK(board_score(b, PLAYER_B) == 1);
  CHECK(board_score(b, PLAYER_A) == 0);
  CHECK(board_is_full(b));
  board_destroy(b);
}

static void test_double_box(void) {
  printf("test_double_box\n");
  /* One line in the middle of a 1x2 board completes both boxes. */
  Board *b = board_create(1, 2);
  CHECK(play(b, 0, 0, 0, 1, PLAYER_A) == 0);
  CHECK(play(b, 0, 1, 0, 2, PLAYER_A) == 0);
  CHECK(play(b, 1, 0, 1, 1, PLAYER_A) == 0);
  CHECK(play(b, 1, 1, 1, 2, PLAYER_A) == 0);
  CHECK(play(b, 0, 0, 1, 0, PLAYER_A) == 0);
  CHECK(play(b, 0, 2, 1, 2, PLAYER_A) == 0);
  CHECK(play(b, 0, 1, 1, 1, PLAYER_B) == 2);
  CHECK(board_score(b, PLAYER_B) == 2);
  CHECK(board_box_owner(b, 0, 0) == PLAYER_B);
  CHECK(board_box_owner(b, 0, 1) == PLAYER_B);
  CHECK(board_is_full(b));
  board_destroy(b);
}

static void test_full_default_board(void) {
  printf("test_full_default_board\n");
  /* Drawing all 49 lines must give out all 20 boxes. */
  Board *b = board_create(4, 5);
  int lines = 0;
  int boxes = 0;
  Player p = PLAYER_A;
  for (int r = 0; r <= 4; r++) {
    for (int c = 0; c < 5; c++) {
      CHECK(!board_is_full(b));
      int got = play(b, r, c, r, c + 1, p);
      CHECK(got >= 0);
      boxes += got;
      lines++;
    }
    if (r < 4) {
      for (int c = 0; c <= 5; c++) {
        int got = play(b, r, c, r + 1, c, p);
        CHECK(got >= 0);
        boxes += got;
        lines++;
        p = (p == PLAYER_A) ? PLAYER_B : PLAYER_A;
      }
    }
  }
  CHECK(lines == 49);
  CHECK(boxes == 20);
  CHECK(board_is_full(b));
  CHECK(board_score(b, PLAYER_A) + board_score(b, PLAYER_B) == 20);
  board_destroy(b);
}

static void test_copy_and_undo(void) {
  printf("test_copy_and_undo\n");
  /* 1x2 board: the middle line is shared by both boxes. */
  Board *b = board_create(1, 2);
  CHECK(play(b, 0, 0, 0, 1, PLAYER_A) == 0);
  CHECK(play(b, 1, 0, 1, 1, PLAYER_A) == 0);
  CHECK(play(b, 0, 0, 1, 0, PLAYER_A) == 0);
  CHECK(play(b, 0, 1, 0, 2, PLAYER_A) == 0);
  CHECK(play(b, 1, 1, 1, 2, PLAYER_A) == 0);
  CHECK(play(b, 0, 2, 1, 2, PLAYER_A) == 0);
  CHECK(board_lines_left(b) == 1);

  Board *copy = board_copy(b);
  CHECK(copy != NULL);
  CHECK(board_lines_left(copy) == 1);
  Line middle = {LINE_VERTICAL, 0, 1};
  CHECK(board_draw_line(copy, middle, PLAYER_B) == 2);
  CHECK(board_score(copy, PLAYER_B) == 2);
  CHECK(board_score(b, PLAYER_B) == 0);
  CHECK(!board_has_line(b, middle));

  /* Undo takes back the line and both boxes it completed. */
  board_undo_line(copy, middle);
  CHECK(!board_has_line(copy, middle));
  CHECK(board_score(copy, PLAYER_B) == 0);
  CHECK(board_box_owner(copy, 0, 0) == PLAYER_NONE);
  CHECK(board_box_owner(copy, 0, 1) == PLAYER_NONE);
  CHECK(board_box_sides(copy, 0, 0) == 3);
  CHECK(board_lines_left(copy) == 1);
  board_destroy(copy);
  board_destroy(b);

  /* Undoing all 49 lines must give back an empty board. */
  b = board_create(4, 5);
  Line drawn[49];
  int n = 0;
  Player p = PLAYER_A;
  for (int r = 0; r <= 4; r++) {
    for (int c = 0; c < 5; c++) {
      Line h = {LINE_HORIZONTAL, r, c};
      drawn[n++] = h;
      board_draw_line(b, h, p);
      p = (p == PLAYER_A) ? PLAYER_B : PLAYER_A;
    }
  }
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c <= 5; c++) {
      Line v = {LINE_VERTICAL, r, c};
      drawn[n++] = v;
      board_draw_line(b, v, p);
      p = (p == PLAYER_A) ? PLAYER_B : PLAYER_A;
    }
  }
  CHECK(n == 49);
  CHECK(board_is_full(b));
  CHECK(board_score(b, PLAYER_A) + board_score(b, PLAYER_B) == 20);
  while (n > 0) {
    board_undo_line(b, drawn[--n]);
  }
  CHECK(board_lines_left(b) == 49);
  CHECK(board_score(b, PLAYER_A) == 0);
  CHECK(board_score(b, PLAYER_B) == 0);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 5; c++) {
      CHECK(board_box_sides(b, r, c) == 0);
      CHECK(board_box_owner(b, r, c) == PLAYER_NONE);
    }
  }
  board_destroy(b);
}

static void test_parse_move(void) {
  printf("test_parse_move\n");
  int v[4];
  CHECK(input_parse_move("0 0 0 1", v) && v[0] == 0 && v[3] == 1);
  CHECK(input_parse_move("  1   2 3 4  ", v) && v[0] == 1 && v[1] == 2 &&
        v[2] == 3 && v[3] == 4);
  CHECK(input_parse_move("1,2,3,4", v) && v[2] == 3);
  CHECK(input_parse_move("-1 0 0 0", v) && v[0] == -1);
  CHECK(!input_parse_move("", v));
  CHECK(!input_parse_move("1 2 3", v));
  CHECK(!input_parse_move("1 2 3 4 5", v));
  CHECK(!input_parse_move("a b c d", v));
  CHECK(!input_parse_move("1 2 3 4x", v));
  CHECK(!input_parse_move("99999999999999999999999 0 0 0", v));
}

int main(void) {
  test_create();
  test_validate();
  test_single_box();
  test_double_box();
  test_full_default_board();
  test_copy_and_undo();
  test_parse_move();
  printf("%d checks, %d failed\n", checks_run, checks_failed);
  return checks_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
