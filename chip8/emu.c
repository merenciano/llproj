#include <stdio.h> // Remove this at the end
#include <stdint.h>

union mem 
{
    int8_t raw[4096];
    struct // Has to be less than 512
    {
        uint16_t stack[16];
        uint16_t pc; // Program counter
        uint16_t i; // I Register
        uint8_t display[64*32/8]; // Bitmap in order to fit the struct in 512 B
        uint8_t fonts[16][5];
        uint8_t v[16]; // Registers
        uint8_t keys[16];
        uint8_t sp; // Stack pointer
        uint8_t delay;
        uint8_t sound;
    };
};

int main()
{
    return 0;
}
