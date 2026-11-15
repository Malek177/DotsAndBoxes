# gdb_demo.gdb - A short GDB tour of the game, good for the live demo.
#
# Run:  gdb -q -x tests/gdb_demo.gdb ./dotsandboxes
#
# It plays a tiny 1x2 game from a file and stops inside the rules code
# so you can show how a move changes the board's memory.

set pagination off

# 1) Stop every time a line is drawn.
break board_draw_line
run --plain --rows 1 --cols 2 < tests/scripts/demo_1x2.txt > /dev/null

# 2) Look at the arguments: which line, which player?
print line
print p

# 3) Look inside the opaque Board struct (only visible in board.c).
print *b
print/d b->h[0]@(b->rows + 1) * b->cols
print/d b->v[0]@b->rows * (b->cols + 1)

# 4) Run to the end of the function and see how many boxes it completed.
finish

# 5) Skip ahead to the 7th move (player A), which completes TWO boxes at
#    once. "ignore 1 5" = skip breakpoint 1 the next 5 times it is hit.
ignore 1 5
continue
print line
print p
# Stop whenever A's score changes (A = 1 in the Player enum).
watch b->score[1]
continue
backtrace
continue
print b->score
print/d b->owner[0]@2
delete
continue
quit
