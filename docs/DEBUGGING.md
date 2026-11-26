# Debugging Strategy (Sprint 1)

We follow the **scientific debugging** method from Lecture 7:
*failure → hypothesis → prediction → experiment → diagnosis → fix → confirm*.
Our time budget per bug: 0 min `-Wall -Werror` + Valgrind; 1–10 min informal;
10–60 min scientific; >60 min take a break / ask.

## Our routine on every change

| Step | Command | Why |
|---|---|---|
| 1. Compile with all warnings as errors | `make` (uses `-Wall -Wextra -Werror -pedantic -g -O0`) | A warning never gets ignored; `-g -O0` keeps line numbers exact for GDB/Valgrind. |
| 2. Unit tests | `./tests/test_units` | 177 checks on the rules and the input parser. |
| 3. Full scripted games | `sh tests/run_scripted.sh` | Plays 4 complete games + 1 aborted game from files and checks each final score. |
| 4. Valgrind on every game | `make valgrind` | Even when output looks right (see Case 1). |
| 5. CI | GitHub Actions (`.github/workflows/ci.yml`) | Runs steps 1–4 on Ubuntu on every push. |

Current status: `make valgrind` → **0 errors, 21 allocs / 21 frees, no leaks possible**.

---

## Case 1 — Heap buffer overflow that does NOT show up in normal runs (Valgrind)

*Fault-injection demo: we wrote the bug on purpose to test our process.*

- **Failure:** none visible! The game printed the right result (`It's a tie! Final score: A 10 - 10 B`).
- **Experiment:** we run Valgrind on every build anyway:
  `valgrind --leak-check=full ./dotsandboxes --plain < tests/scripts/tie_game.in`
- **Observation:** `Invalid read of size 1 ... Address is 0 bytes after a block of size 20 alloc'd at board_create (board.c:47)`. 240 errors in total, including an `Invalid write of size 1`.
- **Hypothesis:** the vertical-lines array is too small. For R×C boxes there are R×(C+1) vertical lines = 4×6 = **24**, but the block is **20** = 4×5.
- **Diagnosis:** `calloc(rows * cols)` instead of `calloc(rows * (cols + 1))` — an off-by-one in the *size*, so the rightmost column of vertical lines wrote past the end of the block.
- **Fix + confirm:** corrected the size; Valgrind is clean again.
- **Lesson:** the program "worked" only because malloc happened to leave spare bytes after the block. This is exactly Lecture 7's point: *run Valgrind even if your program appears to be working.*

Log: [`logs/case1_valgrind_offbyone.txt`](logs/case1_valgrind_offbyone.txt)

## Case 2 — Memory leak on the "input closed" path (Valgrind)

*Fault-injection demo.*

- **Failure:** none visible; only shows up when the game is aborted (Ctrl+D) mid-game.
- **Experiment:** `valgrind --leak-check=full ./dotsandboxes --plain < tests/scripts/abort_midgame.in`
- **Observation:** `185 (56 direct, 129 indirect) bytes in 1 blocks are definitely lost ... by board_create`.
- **Reading the numbers:** 56 bytes = the `Board` struct itself (direct). 129 bytes = its 3 arrays (indirect): horizontal 5×5 = 25 + vertical 4×6 = 24 + owners 20×4 = 80.
- **Diagnosis:** the early `return` skipped `board_destroy()`.
- **Fix + confirm:** single exit point — `break` out of the loop, so `board_destroy()` always runs. Valgrind: no leaks.

Log: [`logs/case2_valgrind_leak.txt`](logs/case2_valgrind_leak.txt)

## Case 3 — Bonus turn not given after completing a box (GDB)

*Fault-injection demo. Valgrind cannot find this: it is a logic bug, not a memory bug.*

- **Failure:** scripted tests `a_wins` and `b_wins` fail — final scores differ from the reference.
- **Hypothesis:** the turn passes to the other player even when a box was completed.
- **Prediction:** if we stop right after a move that completed a box, `current` will change anyway.
- **Experiment (conditional breakpoint):**
  ```
  break game.c:92 if last_completed > 0
  run --plain --rows 1 --cols 2 < tests/scripts/case3_1x2.txt
  print last_completed      # 1  -> a box was just completed
  print current             # PLAYER_B (B completed it)
  next
  next
  print current             # PLAYER_A  <-- turn passed: hypothesis confirmed
  ```
- **Diagnosis:** the condition guarding `current = other_player(current)` was wrong.
- **Fix + confirm:** `if (last_completed == 0)`; all scripted games pass again.

Log: [`logs/case3_gdb_bonus_turn.txt`](logs/case3_gdb_bonus_turn.txt)

## GDB demo for the presentation

`gdb -q -x tests/gdb_demo.gdb ./dotsandboxes` plays a 1×2 game and shows:
breakpoints on `board_draw_line`, printing a struct (`print *b`), printing arrays
(`print/d b->h[0]@n`), `finish` to see a return value, `ignore` to skip hits, and a
**watchpoint** on `b->score[1]` that fires twice (0→1→2) when one line completes two
boxes, with `backtrace` showing `main → game_play → board_draw_line → try_claim`.

Log: [`logs/gdb_demo_session.txt`](logs/gdb_demo_session.txt)

---

## Real bugs we hit during development

> Add every real bug here in the same format (failure, hypothesis, experiment, diagnosis, fix). Graders value a genuine log much more than demos.

1. **GDB `continue 6` did nothing.** While writing the GDB demo we used `continue 6` to skip to the 7th move, but GDB stopped at the next call. Hypothesis: `continue N` only sets an ignore count when stopped *at a breakpoint*; after `finish` we were not. Fix: use the explicit `ignore 1 5` command. Confirmed: stops at the 7th move.
2. **Possible `-Werror` failure on 32-bit Linux.** `value < INT_MIN` on a `long` is "always false" where `long` is 32-bit, and GCC can warn about that; with `-Werror` the build would fail (= zero grade). Fix: guard the check with `#if LONG_MAX > INT_MAX`.
