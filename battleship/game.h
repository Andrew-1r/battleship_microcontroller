/*
 * game.h
 *
 * Author: Jarrod Bennett, Cody Burnett
 *
 * Modified by: Andrew Wilson
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */

#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>

// Initialise the game by resetting the grid and beat
void initialise_game(void);

// flash the cursor
void flash_cursor(void);

// move the cursor in the x and/or y direction
void move_cursor(int8_t dx, int8_t dy);

// Returns 1 if the game is over, 0 otherwise.
uint8_t is_game_over(void);

// Handles the player turn
void player_turn(void);

// Handles the computer turn
void computer_turn(void);

// Check for sunken ships
void check_for_sunken_ships(uint8_t player, uint8_t grid[8][8]);

// Sink a ship
void sink_ship(uint8_t player, int8_t col, int8_t row, char direction,
               uint8_t grid[8][8]);

// Print to console when a ship is sunk
void print_sunken_ship(uint8_t player, uint8_t ship);

#define SEA 0b00000000
#define CARRIER 0b00000001
#define CRUISER 0b00000010
#define DESTROYER 0b00000011
#define FRIGATE 0b00000100
#define CORVETTE 0b00000101
#define SUBMARINE 0b00000110
#define SHIP_MASK 0b00000111
#define SHIP_END 0b00001000
#define HORIZONTAL 0b00010000
#define HIT 0b10000000
#define SUNK 0b01000000

#endif
