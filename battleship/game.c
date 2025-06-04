/*
 * game.c
 *
 * Functionality related to the game state and features.
 *
 * Author: Jarrod Bennett, Cody Burnett
 *
 * Modified by: Andrew Wilson
 */

#include "game.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "display.h"
#include "ledmatrix.h"
#include "string.h"
#include "terminalio.h"

uint8_t human_grid[GRID_NUM_ROWS][GRID_NUM_COLUMNS];
uint8_t computer_grid[GRID_NUM_ROWS][GRID_NUM_COLUMNS];
int8_t cursor_x, cursor_y;
uint8_t cursor_on;
uint8_t invalidMoves = 0;
uint8_t humanConsolePrinter = 2;
uint8_t computerConsolePrinter = 2;
uint8_t count = 0;

// Initialise the game by resetting the grid and beat
void initialise_game(void) {
  // clear the splash screen art
  ledmatrix_clear();

  // see "Human Turn" feature for how ships are encoded
  // fill in the grid with the ships
  uint8_t initial_human_grid[GRID_NUM_ROWS][GRID_NUM_COLUMNS] = {
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA},
      {SEA, CARRIER | HORIZONTAL | SHIP_END, CARRIER | HORIZONTAL,
       CARRIER | HORIZONTAL, CARRIER | HORIZONTAL, CARRIER | HORIZONTAL,
       CARRIER | HORIZONTAL | SHIP_END, SEA},
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA},
      {SEA, SEA, CORVETTE | SHIP_END, SEA, SEA, SUBMARINE | SHIP_END, SEA, SEA},
      {DESTROYER | SHIP_END, SEA, CORVETTE | SHIP_END, SEA, SEA,
       SUBMARINE | SHIP_END, SEA, FRIGATE | SHIP_END},
      {DESTROYER, SEA, SEA, SEA, SEA, SEA, SEA, FRIGATE},
      {DESTROYER | SHIP_END, SEA, CRUISER | HORIZONTAL | SHIP_END,
       CRUISER | HORIZONTAL, CRUISER | HORIZONTAL,
       CRUISER | HORIZONTAL | SHIP_END, SEA, FRIGATE | SHIP_END},
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA}};
  uint8_t initial_computer_grid[GRID_NUM_ROWS][GRID_NUM_COLUMNS] = {
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA},
      {DESTROYER | SHIP_END, SEA, CRUISER | HORIZONTAL | SHIP_END,
       CRUISER | HORIZONTAL, CRUISER | HORIZONTAL,
       CRUISER | HORIZONTAL | SHIP_END, SEA, FRIGATE | SHIP_END},
      {DESTROYER, SEA, SEA, SEA, SEA, SEA, SEA, FRIGATE},
      {DESTROYER | SHIP_END, SEA, CORVETTE | SHIP_END, SEA, SEA,
       SUBMARINE | SHIP_END, SEA, FRIGATE | SHIP_END},
      {SEA, SEA, CORVETTE | SHIP_END, SEA, SEA, SUBMARINE | SHIP_END, SEA, SEA},
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA},
      {SEA, CARRIER | HORIZONTAL | SHIP_END, CARRIER | HORIZONTAL,
       CARRIER | HORIZONTAL, CARRIER | HORIZONTAL, CARRIER | HORIZONTAL,
       CARRIER | HORIZONTAL | SHIP_END, SEA},
      {SEA, SEA, SEA, SEA, SEA, SEA, SEA, SEA}};
  for (uint8_t i = 0; i < GRID_NUM_COLUMNS; i++) {
    for (uint8_t j = 0; j < GRID_NUM_COLUMNS; j++) {
      human_grid[j][i] = initial_human_grid[j][i];
      computer_grid[j][i] = initial_computer_grid[j][i];
      if (human_grid[j][i] & SHIP_MASK) {
        ledmatrix_draw_pixel_in_human_grid(i, j, COLOUR_ORANGE);
      }
    }
  }
  cursor_x = 3;
  cursor_y = 3;
  cursor_on = 1;
}

void flash_cursor(void) {
  cursor_on = 1 - cursor_on;

  if (cursor_on && (computer_grid[7 - cursor_y][cursor_x] & HIT)) {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y,
                                          COLOUR_DARK_YELLOW);
  } else if (cursor_on) {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_YELLOW);
  }

  else if ((computer_grid[7 - cursor_y][cursor_x] & HIT) &&
           (computer_grid[7 - cursor_y][cursor_x] &
            SHIP_MASK))  // test for hit ship here, then duplicate and modify to
                         // test for sunken ship
  {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_RED);
  }

  else if ((computer_grid[7 - cursor_y - 7][cursor_x] & HIT) &&
           !(computer_grid[7 - cursor_y][cursor_x] & SHIP_MASK)) {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_GREEN);
  } else {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_BLACK);
  }
}

// moves the position of the cursor by (dx, dy) such that if the cursor
// started at (cursor_x, cursor_y) then after this function is called,
// it should end at ( (cursor_x + dx) % WIDTH, (cursor_y + dy) % HEIGHT)
// the cursor should be displayed after it is moved as well <- TODO need to
// flash it
void move_cursor(int8_t dx, int8_t dy) {
  // update board as cursor moves
  if ((computer_grid[7 - cursor_y][cursor_x] & HIT) &&
      (computer_grid[7 - cursor_y][cursor_x] & SHIP_MASK)) {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_RED);
  } else if ((computer_grid[7 - cursor_y][cursor_x] & HIT) &&
             !(computer_grid[7 - cursor_y][cursor_x] & SHIP_MASK)) {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_GREEN);
  } else {
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_BLACK);
  }

  // move cursor to new position
  cursor_x += dx;
  cursor_y += dy;
  if (cursor_x >= 8) {
    cursor_x = 0;
  }
  if (cursor_x < 0) {
    cursor_x = 7;
  }
  if (cursor_y >= 8) {
    cursor_y = 0;
  }
  if (cursor_y < 0) {
    cursor_y = 7;
  }
}

void print_sunken_ship(uint8_t player, uint8_t ship) {
  // set up ship types and message variable
  char ship_type[20];
  char message[30];

  if ((ship & CARRIER) && !(ship & CRUISER) && !(ship & FRIGATE)) {
    strcpy(ship_type, "Carrier");
  } else if ((ship & CRUISER) && !(ship & CARRIER) && !(ship & FRIGATE)) {
    strcpy(ship_type, "Cruiser");
  } else if ((ship & DESTROYER) && !(ship & FRIGATE)) {
    strcpy(ship_type, "Destroyer");
  } else if ((ship & FRIGATE) && !(ship & CARRIER) && !(ship & CRUISER)) {
    strcpy(ship_type, "Frigate");
  } else if ((ship & CORVETTE) && !(ship & CRUISER)) {
    strcpy(ship_type, "Corvette");
  } else if (ship & SUBMARINE) {
    strcpy(ship_type, "Submarine");
  }

  // char human[] = "You Sunk My %s", ship_type;
  // char computer[] = "I Sunk Your %s", ship_type;

  // print to the terminal after a ship has been sunk
  if (player == 1) {
	// TODO: delete, print statements for debugging
    // move_terminal_cursor(60 - strlen(human), humanConsolePrinter);
    // printf(human);
    // humanConsolePrinter++;
    sprintf(message, "You Sunk My %s", ship_type);
    move_terminal_cursor(80 - strlen(message), humanConsolePrinter);
    printf("%s\n", message);
    humanConsolePrinter++;  // Assuming this is a variable managed elsewhere to
                            // track output position
    return;
  } else {
	// TODO: delete, print statements for debugging
    // move_terminal_cursor(20, computerConsolePrinter);
    // printf(computer);
    sprintf(message, "I Sunk Your %s", ship_type);
    move_terminal_cursor(20, computerConsolePrinter);
    printf("%s\n", message);
    computerConsolePrinter++;
    return;
  }
}

// sink a ship, starting at x and y, progressing along the length in the
// direction of travel
void sink_ship(uint8_t player, int8_t col, int8_t row, char direction,
               uint8_t grid[8][8]) {
  // TODO: delete, print statements for debugging
  // move_terminal_cursor(50, count);
  // count++;
  // printf("%c\n", direction);

  uint8_t miss = 0;
  int8_t length = 0;

  if (direction == 'w') {
    for (row; row >= 0; row--) {
      if ((grid[row][col] & SHIP_MASK) && !(grid[row][col] & HIT)) {
        miss++;
      } else if ((grid[row][col] & ~HIT) == SEA) {
        break;
      }
      length++;
    }

  } else if (direction == 's') {
    for (row; row < 8; row++) {
      if ((grid[row][col] & SHIP_MASK) && !(grid[row][col] & HIT)) {
        miss++;
      } else if ((grid[row][col] & ~HIT) == SEA) {
        break;
      }
      length++;
    }

  } else if (direction == 'a') {
    for (col; col > 0; col--) {
      if ((grid[row][col] & SHIP_MASK) && !(grid[row][col] & HIT)) {
        miss++;
      } else if ((grid[row][col] & ~HIT) == SEA) {
        break;
      }
      length++;
    }

  } else if (direction == 'd') {
    for (col; col < 8; col++) {
      if ((grid[row][col] & SHIP_MASK) && !(grid[row][col] & HIT)) {
        miss++;
      } else if ((grid[row][col] & ~HIT) == SEA) {
        break;
      }
      length++;
    }
  }

  // use length and direction, start coord, and player to mark appropriate ship
  // as sunk
  if (miss == 0) {
    uint8_t original_length = length;

    // TODO: add a while loop which incrementally lower the length integer and
    // puts a SUNK bit into each value based on direction be careful of
    // behaviour from notes TODO, need to adjust row and col values have an if
    // statement at the end of each nested loop inside while loop that returns
    // to print ships when the count gets to 0

    if (direction == 'w') {
      // solving weird problem with row and length values
      row++;
      length--;
      while (length >= 0) {
        // TODO: delete, print statements for debugging
        // move print cursor -> DELETE debugging
        // move_terminal_cursor(30, count);
        // count++;
        // printf("Sinking ship at (Column %d, Row %d), length %d direction
        // %c.\n \n", col, row, length, direction);

        // sink the ship part
        grid[row][col] |= SUNK;
        row++;
        length--;
        if (length < 0) {
          // TODO: delete, print statements for debugging
          // this will be sent to print_ship
          // move_terminal_cursor(30, count);
          // count++;
          // printf("Finished sinking ship of original length %d",
          // original_length);
          return print_sunken_ship(player, grid[row - 1][col]);
        }
      }

    } else if (direction == 's') {
      while (length >= 0) {
        // move print cursor -> DELETE debugging
        move_terminal_cursor(30, count);
        count++;
        printf(
            "Sinking ship at (Column %d, Row %d), length %d direction %c.\n \n",
            col, row, length, direction);

        // sink the ship part
        grid[row][col] |= SUNK;
        row--;
        length--;
        if (length < 0) {
          // this will be sent to print_ship
          move_terminal_cursor(30, count);
          count++;
          printf("Finished sinking ship of original length %d",
                 original_length);
        }
      }

    } else if (direction == 'a') {
      while (length >= 0) {
        // move print cursor -> DELETE debugging
        move_terminal_cursor(30, count);
        count++;
        printf(
            "Sinking ship at (Column %d, Row %d), length %d direction %c.\n \n",
            col, row, length, direction);

        // sink the ship part
        grid[row][col] |= SUNK;
        col++;
        length--;
        if (length < 0) {
          // this will be sent to print_ship
          move_terminal_cursor(30, count);
          count++;
          printf("Finished sinking ship of original length %d",
                 original_length);
        }
      }

    } else if (direction == 'd') {
      while (length >= 0) {
        // TODO: delete, print statements for debugging
        // move print cursor
        // move_terminal_cursor(30, count);
        // count++;
        // printf("Sinking ship at (Column %d, Row %d), length %d direction
        // %c.\n \n", col, row, length, direction);

        // sink the ship part
        grid[row][col] |= SUNK;
        col--;
        length--;
        if (length < 0) {
          // TODO: delete, print statements for debugging
          // this will be sent to print_ship
          // move_terminal_cursor(30, count);
          // count++;
          // printf("Finished sinking ship of original length %d",
          // original_length);
          return print_sunken_ship(player, grid[row][col + 1]);
        }
      }
    }
  }
}

// check entire board for any ships that have been hit and not sunk, send
// coordinates and direction to sink_ship
void check_for_sunken_ships(uint8_t player, uint8_t grid[8][8]) {
  // loop through the board
  for (int8_t row = 7; row >= 0; row--) {
    for (int8_t col = 0; col < 8; col++) {
      // find the end of a ship
      if ((grid[row][col] & SHIP_END) && !(grid[row][col] & SUNK)) {
        // if ship is horizontal and sea or out of bounds to the left, go right,
        // else go left
        if (grid[row][col] & HORIZONTAL) {
          if ((col - 1 < 0) || ((grid[row][col - 1] & ~HIT) == SEA)) {
            sink_ship(player, col, row, 'd', grid);

          } else {
            sink_ship(player, col, row, 'a', grid);
          }
        } else {
          // else if no sea down or out of bounds, go up, else go down
          if ((row - 1 < 0) || ((grid[row - 1][col] & ~HIT) == SEA)) {
            sink_ship(player, col, row, 's', grid);
          } else {
            sink_ship(player, col, row, 'w', grid);
          }
        }
      }
    }
  }

  // TODO: delete, print statements for debugging
  // move_terminal_cursor(80, 0);
  // if (player == 1) {
  //	printf("FINISHED for player");
  //} else {
  //	printf("FINISHED for computer");
  //}
}

void player_turn(void) {
  // handle invalid move
  if (computer_grid[7 - cursor_y][cursor_x] & HIT) {
    move_terminal_cursor(0, 1);

    char invalidMoveMessage[20] = "Invalid move";
    uint8_t messageLength = strlen(invalidMoveMessage);

    for (uint8_t i = 0;
         i < invalidMoves && messageLength < sizeof(invalidMoveMessage) - 1;
         i++) {
      invalidMoveMessage[messageLength++] = '!';
    }
    invalidMoveMessage[messageLength] = '\0';

    printf("%s", invalidMoveMessage);

    if (invalidMoves < 3) {
      invalidMoves++;
    }
    return;
  }

  // draw red for hit green for miss
  if (computer_grid[7 - cursor_y][cursor_x] != 0) {
    computer_grid[7 - cursor_y][cursor_x] |= HIT;
    check_for_sunken_ships(1, computer_grid);
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_RED);
  } else {
    computer_grid[7 - cursor_y][cursor_x] |= HIT;
    ledmatrix_draw_pixel_in_computer_grid(cursor_x, cursor_y, COLOUR_GREEN);
  }

  // clear terminal and invalid moves value on valid move
  if (invalidMoves != 0) {
    move_terminal_cursor(0, 0);
    printf("                        ");
    invalidMoves = 0;
  }

  computer_turn();
}

void computer_turn(void) {
  // step through row, column and hit the first available space that hasn't been
  // hit yet
  for (int8_t row = 7; row >= 0; row--) {
    for (int8_t col = 0; col < 8; col++) {
      if ((human_grid[row][col] & HIT) == 0) {
        if ((human_grid[row][col] != 0) && (human_grid[row][col] & SHIP_MASK)) {
          ledmatrix_draw_pixel_in_human_grid(col, row, COLOUR_RED);
          human_grid[row][col] |= HIT;
          check_for_sunken_ships(0, human_grid);
          return;
        } else {
          ledmatrix_draw_pixel_in_human_grid(col, row, COLOUR_GREEN);
          human_grid[row][col] |= HIT;
          return;
        }
      }
    }
  }
}

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void) {
  // Detect if the game is over i.e. if a player has won.
  // return 0;

  for (int8_t row = 7; row >= 0; row--) {
    for (int8_t col = 0; col < 8; col++) {
      if ((human_grid[row][col] & SHIP_MASK) &&
          !(human_grid[row][col] & SUNK)) {
        return 0;
      }
    }
  }

  for (int8_t row = 7; row >= 0; row--) {
    for (int8_t col = 0; col < 8; col++) {
      if ((computer_grid[row][col] & SHIP_MASK) &&
          !(computer_grid[row][col] & SUNK)) {
        return 0;
      }
    }
  }
  return 1;
}
