/*
 * pixel_colour.h
 *
 * Author: Peter Sutton
 */ 

#ifndef PIXEL_COLOUR_H_
#define PIXEL_COLOUR_H_

#include <stdint.h>

// Each PixelType is an 8 bit number - 4 bits of green in the high bits,
// 4 bits of red in the low bits
typedef uint8_t PixelColour;

// Some useful colour definitions
#define COLOUR_BLACK		(0x00)
#define COLOUR_RED			(0x0F)
#define COLOUR_GREEN		(0xF0)
#define COLOUR_ORANGE		(0x3C)
#define COLOUR_YELLOW		(0xFF)
#define COLOUR_DARK_YELLOW (0x6A)

#endif /* PIXEL_COLOUR_H_ */
