# The Hard Bot: Minimax with Alpha-Beta Pruning

The bot (`src/bot.c`) plays as Player B. It does not use a list of if/else
rules: it **searches the game tree**. It tries every move, then every reply,
then every reply to that, and picks the move that leads to the best final
score, assuming the human also plays as well as possible.

## 1. The idea: minimax

Think of the game as a tree. The root is the current board, each branch is one
line someone could draw, and each leaf is a finished game.

* The bot wants to **maximise** `(bot score − human score)`.
* The human is assumed to **minimise** the same number.
* So at the bot's turns we take the **max** of the children, and at the human's
  turns the **min**.

```
                bot to move  (max)
              /       |       \
          +1          -2        0       <- value of each move
         (min)       (min)    (min)     human to move
```

The bot picks the move with value +1.

**The Dots and Boxes twist:** completing a box gives the same player another
turn. So after a move, the turn changes **only if no box was completed**:

```c
int completed = board_draw_line(b, moves[i], turn);
Player next = (completed > 0) ? turn : other(turn);
```

This means two "max" levels can follow each other in the tree, which is right:
a player taking a chain of boxes really does make several moves in a row.

## 2. Making it fast enough

A 4×5 board has 49 lines, so the full tree has up to 49! ≈ 6 × 10⁶² paths. We
cannot search all of it, so the bot uses five techniques.

| Technique | What it does | Where |
|---|---|---|
| **Alpha-beta pruning** | Stops searching a branch as soon as it is clear the other player would never allow it. Gives the same answer as plain minimax, with far less work. | `minimax()`: `if (alpha >= beta) break;` |
| **Move ordering** | Tries moves that take a box first, then safe moves, then moves that give a box away. Good moves first means more branches are cut. | `list_moves()` |
| **Iterative deepening** | Searches 1 move ahead, then 2, then 3, … and keeps the answer of the deepest search that finished. The best move so far is tried first at the next depth. | `bot_choose_move()` |
| **Work budget** | Each move may examine at most `12,250,000 / E` positions (250,000 on 4×5). This keeps every move under about half a second, on every board size, and the bot always plays the same way in the same position (useful for tests). | `WORK_BUDGET` |
| **Settling at the leaves** | When the search stops before the end of the game, the player to move first takes every box that is free to take, and only then is the board scored. Without this the bot misjudges positions in the middle of a chain. | `settle_and_evaluate()` |

The bot tries moves on its **own copy** of the board (`board_copy`) and takes
each move back with `board_undo_line`, so the real board is never changed and
no memory is allocated during the search.

## 3. Complexity

Notation: **E** = number of lines (49 on 4×5), **b** = moves available at a
position (at most E), **d** = search depth, **N** = node budget per move.

### 3.1 Cost of one position (one node)

| Step | Cost | Why |
|---|---|---|
| Draw a line / undo a line | **O(1)** | only the ≤ 2 boxes next to the line can change |
| Classify a move (takes a box? gives a box?) | **O(1)** | looks at ≤ 2 boxes, 4 sides each |
| List and order all moves | **O(E)** | one pass over the lines, sorted into 3 groups (no real sort needed) |
| Settle at a leaf | **O(R·C)** per box taken | scan the boxes for one with 3 sides |

So one node costs **O(E)**.

### 3.2 Cost of the search

| Search | Positions examined | Cost |
|---|---|---|
| Plain minimax to depth d | b · b · … · b = **b^d** | O(b^d · E) |
| Alpha-beta, worst case (bad ordering) | still b^d | O(b^d · E) |
| Alpha-beta, best case (best move always first) | about **b^(d/2)** | O(b^(d/2) · E) |
| Iterative deepening 1, 2, …, d | b + b² + … + b^d < 2·b^d for b ≥ 2 | same order as the last depth |
| **Our bot, per move** | at most **N** (the budget) | **O(N · E)** |
| Full game tree (solving 4×5 exactly) | up to E! = 49! ≈ 6 × 10⁶² | impossible |

Best-case alpha-beta means the bot can look **about twice as deep** as plain
minimax for the same work.

**Memory:** the search is depth-first, so it only stores the current path:
**O(d · E)** for the move lists on the stack plus **O(R·C)** for the board
copy. On a 9×9 board that is at most a few hundred kilobytes.

### 3.3 Measured: how much alpha-beta and ordering save

Same 5 positions from the 4×5 board (40 lines left), searched to a fixed depth.
All three versions returned exactly the same values: pruning never changes the answer.

| Depth | Plain minimax | Alpha-beta, no ordering | Alpha-beta + our ordering |
|---|---|---|---|
| 3 | 60,880 | 6,002 | 1,402 |
| 4 | 2,254,240 | 112,472 | 4,025 |
| 5 | 81,215,200 | 2,701,765 | **39,269** |

Plain minimax grows like 40⁵ ≈ 10⁸, as predicted by b^d. With ordering,
alpha-beta at depth 5 examines about **2,000× fewer** positions, close to the
best case b^(d/2) = 40^2.5 ≈ 10⁴.

### 3.4 Measured: the real bot

`make arena` plays 10 games against each opponent on 4×5 (the opponent moves
first in half the games). Built with `-O0`; times depend on the computer, so run `make arena` on the VM to compare:

| Opponent | Won | Tied | Lost | Average time per move | Slowest move |
|---|---|---|---|---|---|
| Random player | 10 | 0 | 0 | 0.152 s | 0.594 s |
| Greedy player (takes boxes, avoids giving any) | 10 | 0 | 0 | 0.144 s | 0.591 s |

How deep the bot looks during a game against the greedy player:

| Stage of the game | Average depth | Average positions examined |
|---|---|---|
| Opening (49–34 lines left) | 5.6 | 179,614 |
| Middle (33–17 lines left) | 11.4 | 186,311 |
| End (16–1 lines left) | 7.9 (usually to the end of the game) | 16,885 |

With fewer lines left, b is smaller, so the same budget reaches much deeper:
in the end game the bot usually searches to the very last move, which means
it plays the end game perfectly.

## 4. How we test the bot

`tests/test_bot.c` (run by `make test` and `make valgrind`):

* takes the last box of a 1×1 board, and takes a free box when it is the best move;
* on 30 random positions (2×2, 1×3, 2×3 boards, 7 lines left) the bot's value is
  compared with a **plain minimax with no pruning**, written separately in the test;
  they must be equal, and the move the bot picks must actually reach that value;
* in full games against a random player, every bot move is a free line and the real
  board is never changed by `bot_choose_move` (checked line by line, box by box).

`tests/scripts/bot_2x2.in` plays a full game against the bot through the real
program, including a human typing lines the bot has already drawn.
