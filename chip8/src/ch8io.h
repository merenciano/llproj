
#ifndef __CHIP8_INPUT_OUTPUT_H__
#define __CHIP8_INPUT_OUTPUT_H__

#include <stdint.h>

#define PIX_SIZE 16

// NOTE: The void **window param can be changed with
// void *window, void *renderer and void *texture.
// I've done it that way because I know beforehand that
// that three pointers are contiguous in memory so
// I prefer using only one param and increment it

// Create and release i/o data
void InitIO(void **window, const char *title);
void CloseIO(void **window);

// Keys ptr must have 16B space
uint8_t GetInput(uint8_t *keys);

// Draw CHIP8 display bitmap to screen
void RenderBitmap(void **window, uint8_t *bitmap);

// Returns time since begining of execution in miliseconds
uint32_t TimeMS();

#endif // __CHIP8_INPUT_OUTPUT_H__

