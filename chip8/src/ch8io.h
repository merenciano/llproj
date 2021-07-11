#ifndef __CHIP8_INPUT_OUTPUT_H__
#define __CHIP8_INPUT_OUTPUT_H__

#include <stdint.h>

typedef struct
{
    void *window;
    void *renderer;
    void *texture;
} IOData;

// Init i/o data struct
void InitIO(IOData *data);

// Keys ptr must have 16B space
uint8_t GetInput(uint8_t *keys);

// Draw CHIP8 display bitmap to screen
void RenderBitmap(IOData *data, uint8_t *bitmap);

// Returns time since begining of execution in miliseconds
uint32_t TimeMS();

#endif // __CHIP8_INPUT_OUTPUT_H__

