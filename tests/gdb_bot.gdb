# gdb_bot.gdb - Watch the bot's search, one line per finished depth.
# Run with: gdb -q -x tests/gdb_bot.gdb ./dotsandboxes

set pagination off
break bot_choose_move
dprintf bot.c:223,"depth %2d finished: best value %d, positions so far %ld\n", depth_done, best_value, s.nodes
break bot.c:235
run --plain < tests/scripts/bot_first_move.txt > /dev/null

print me
print board_lines_left(board)
continue

print best
print s
continue
