#!/bin/sh
# run_scripted.sh - Plays every tests/scripts/*.in file through the game
# and checks the result.
#
#   NAME.in        input typed by the "players", one line per move
#   NAME.expected  text that MUST appear in the output (e.g. final score)
#   NAME.args      (optional) extra options, e.g. --rows 2 --cols 2
#
# Usage: sh tests/run_scripted.sh ./dotsandboxes

GAME=${1:-./dotsandboxes}
DIR=$(dirname "$0")/scripts
pass=0
fail=0

for input in "$DIR"/*.in; do
  name=$(basename "$input" .in)
  expected="$DIR/$name.expected"
  args=$(cat "$DIR/$name.args" 2>/dev/null)
  output=$("$GAME" --plain $args < "$input")
  status=$?
  want=$(cat "$expected")
  if [ "$status" -eq 0 ] && printf '%s\n' "$output" | grep -qF "$want"; then
    echo "  PASS $name ($want)"
    pass=$((pass + 1))
  else
    echo "  FAIL $name: expected '$want' (exit status $status)"
    fail=$((fail + 1))
  fi
done

echo "scripted games: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
