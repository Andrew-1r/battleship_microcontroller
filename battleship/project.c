/*
 * project.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett
 * Modified by: Andrew Wilson
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "buttons.h"
#include "display.h"
#include "game.h"
#include "ledmatrix.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);

/////////////////////////////// main //////////////////////////////////
int main(void) {
  // Setup hardware and call backs. This will turn on
  // interrupts.
  initialise_hardware();

  // Show the splash screen message. Returns when display
  // is complete.
  start_screen();

  // Loop forever and continuously play the game.
  while (1) {
    new_game();
    play_game();
    handle_game_over();
  }
}

void initialise_hardware(void) {
  ledmatrix_setup();
  init_button_interrupts();
  // Setup serial port for 19200 baud communication with no echo
  // of incoming characters
  init_serial_stdio(19200, 0);

  init_timer0();
  init_timer1();
  init_timer2();

  // Turn on global interrupts
  sei();
}

void start_screen(void) {
  // Clear terminal screen and output a message
  clear_terminal();
  hide_cursor();
  set_display_attribute(FG_WHITE);
  move_terminal_cursor(10, 4);
  printf_P(
      PSTR(" _______    ______  ________  ________  __        ________   "
           "______   __    __  ______  _______  "));
  move_terminal_cursor(10, 5);
  printf_P(
      PSTR("|       \\  /      \\|        \\|        \\|  \\      |        \\ "
           "/      \\ |  \\  |  \\|      \\|       \\ "));
  move_terminal_cursor(10, 6);
  printf_P(
      PSTR("| $$$$$$$\\|  $$$$$$\\\\$$$$$$$$ \\$$$$$$$$| $$      | $$$$$$$$|  "
           "$$$$$$\\| $$  | $$ \\$$$$$$| $$$$$$$\\"));
  move_terminal_cursor(10, 7);
  printf_P(
      PSTR("| $$__/ $$| $$__| $$  | $$      | $$   | $$      | $$__    | "
           "$$___\\$$| $$__| $$  | $$  | $$__/ $$"));
  move_terminal_cursor(10, 8);
  printf_P(
      PSTR("| $$    $$| $$    $$  | $$      | $$   | $$      | $$  \\    \\$$  "
           "  \\ | $$    $$  | $$  | $$    $$"));
  move_terminal_cursor(10, 9);
  printf_P(
      PSTR("| $$$$$$$\\| $$$$$$$$  | $$      | $$   | $$      | $$$$$    "
           "_\\$$$$$$\\| $$$$$$$$  | $$  | $$$$$$$ "));
  move_terminal_cursor(10, 10);
  printf_P(
      PSTR("| $$__/ $$| $$  | $$  | $$      | $$   | $$_____ | $$_____ |  "
           "\\__| $$| $$  | $$ _| $$_ | $$      "));
  move_terminal_cursor(10, 11);
  printf_P(
      PSTR("| $$    $$| $$  | $$  | $$      | $$   | $$     \\| $$     \\ \\$$ "
           "   $$| $$  | $$|   $$ \\| $$      "));
  move_terminal_cursor(10, 12);
  printf_P(
      PSTR(" \\$$$$$$$  \\$$   \\$$   \\$$       \\$$    \\$$$$$$$$ \\$$$$$$$$ "
           " \\$$$$$$  \\$$   \\$$ \\$$$$$$ \\$$      "));
  move_terminal_cursor(10, 14);
  // change this to your name and student number; remove the chevrons <>
  printf_P(PSTR("CSSE2010/7201 Project by Andrew Wilson - 48280411"));

  // Output the static start screen and wait for a push button
  // to be pushed or a serial input of 's'
  show_start_screen();

  uint32_t last_screen_update, current_time;
  last_screen_update = get_current_time();

  int8_t frame_number = -2 * ANIMATION_DELAY;

  // Wait until a button is pressed, or 's' is pressed on the terminal
  while (1) {
    // First check for if a 's' is pressed
    // There are two steps to this
    // 1) collect any serial input (if available)
    // 2) check if the input is equal to the character 's'
    char serial_input = -1;
    if (serial_input_available()) {
      serial_input = fgetc(stdin);
    }
    // If the serial input is 's', then exit the start screen
    if (serial_input == 's' || serial_input == 'S') {
      break;
    }
    // Next check for any button presses
    int8_t btn = button_pushed();
    if (btn != NO_BUTTON_PUSHED) {
      break;
    }

    // every 200 ms, update the animation
    current_time = get_current_time();
    if (current_time - last_screen_update > 200) {
      update_start_screen(frame_number);
      frame_number++;
      if (frame_number > ANIMATION_LENGTH) {
        frame_number -= ANIMATION_LENGTH + ANIMATION_DELAY;
      }
      last_screen_update = current_time;
    }
  }
}

void new_game(void) {
  // Clear the serial terminal
  clear_terminal();

  // Initialise the game and display
  initialise_game();

  // Clear a button push or serial input if any are waiting
  // (The cast to void means the return value is ignored.)
  (void)button_pushed();
  clear_serial_input_buffer();
}

void play_game(void) {
  uint32_t last_flash_time, current_time;
  int8_t btn;  // The button pushed

  last_flash_time = get_current_time();

  // We play the game until it's over
  while (!is_game_over()) {
    // We need to check if any button has been pushed, this will be
    // NO_BUTTON_PUSHED if no button has been pushed
    // Checkout the function comment in `buttons.h` and the implementation
    // in `buttons.c`.
    btn = button_pushed();

    char key = -1;
    if (serial_input_available()) {
      key = fgetc(stdin);
    }
    // move right
    if (btn == BUTTON0_PUSHED || key == 'D' || key == 'd') {
      move_cursor(1, 0);
    }
    // move down
    if (btn == BUTTON1_PUSHED || key == 'S' || key == 's') {
      move_cursor(0, -1);
    }
    // move up
    if (btn == BUTTON2_PUSHED || key == 'W' || key == 'w') {
      move_cursor(0, 1);
    }
    // move left
    if (btn == BUTTON3_PUSHED || key == 'A' || key == 'a') {
      move_cursor(-1, 0);
    }
    if (key == 'F' || key == 'f') {
      player_turn();
    }
    current_time = get_current_time();
    if (current_time >= last_flash_time + 200) {
      // 200ms (0.2 second) has passed since the last time we advance the
      // notes here, so update the advance the notes
      flash_cursor();

      // Update the most recent time the notes were advance
      last_flash_time = current_time;
    }
  }
  // We get here if the game is over.
  move_terminal_cursor(0, 3);
  printf("Game over!");
}

void handle_game_over() {
  move_terminal_cursor(10, 14);
  printf_P(PSTR("GAME OVER"));
  move_terminal_cursor(10, 15);
  printf_P(PSTR("Press a button or 's'/'S' to start a new game"));

  // Do nothing until a button is pushed. Hint: 's'/'S' should also start a
  // new game
  while (button_pushed() == NO_BUTTON_PUSHED) {
    ;  // wait
  }
}
