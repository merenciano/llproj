#ifndef __CHIP8_CPU_H__
#define __CHIP8_CPU_H__

#include <stdint.h>

typedef union
{
    uint8_t raw[4096];
    struct // Has to be less than 512
    {
        void *window;
        void *renderer;
        void *texture;
        uint32_t timers_span;
        uint32_t instr_span;
        uint32_t curr_time;
        uint16_t stack[16];
        uint16_t pc; // Program counter
        uint16_t i; // I Register
        uint8_t display[64*33/8]; // Bitmap in order to fit the struct in 512 B
        uint8_t fonts[16*5];
        uint8_t v[16]; // Registers
        uint8_t keys[16];
        uint8_t sp; // Stack pointer
        uint8_t delay;
        uint8_t sound;
        uint8_t pressed_keys[16];
        uint8_t min_instr_duration_ms;
    };
} Memory;

void Init(Memory *mem);
void LoadFonts(Memory *mem);
void LoadFile(Memory *mem, const char *filename);
void ExecuteInstruction(Memory *mem);
void UpdateTimers(Memory *mem);

#endif // __CHIP8_CPU_H__

