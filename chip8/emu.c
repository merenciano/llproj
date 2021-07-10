#include <stdio.h> // Remove this at the end
#include <stdlib.h> // rand
#include <time.h> // rand seed
#include <stdint.h>
#include <string.h> // memset
#include <SDL2/SDL.h>

#define C8_ASSERT(x) if (!x) *(int*)0 = 2

typedef union TMemory
{
    uint8_t raw[4096];
    struct // Has to be less than 512
    {
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
    };
} Memory;

int loop_count = 0;
uint8_t keys_pressed[16];
uint8_t quit = 0;

void LoadFonts(Memory *mem)
{
    uint8_t font_data[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    
    for (int i=0; i < 16*5; ++i)
    {
        mem->fonts[i] = font_data[i];
    }
}

void Init(Memory *mem)
{
    memset(mem->raw, 0, 4096);
    LoadFonts(mem);
    srand(time(NULL));
}

void GetInput(SDL_Event *e)
{
    while (SDL_PollEvent(e) > 0)
    {
        if (e->type == SDL_QUIT)
        {
            quit = 1;
        }

        if (e->type == SDL_KEYDOWN)
        {
            switch(e->key.keysym.sym)
            {
                case SDLK_x:
                    keys_pressed[0] = 1;
                    break;

                case SDLK_1:
                    keys_pressed[1] = 1;
                    break;

                case SDLK_2:
                    keys_pressed[2] = 1;
                    break;

                case SDLK_3:
                    keys_pressed[3] = 1;
                    break;

                case SDLK_q:
                    keys_pressed[4] = 1;
                    break;

                case SDLK_w:
                    keys_pressed[5] = 1;
                    break;

                case SDLK_e:
                    keys_pressed[6] = 1;
                    break;

                case SDLK_a:
                    keys_pressed[7] = 1;
                    break;

                case SDLK_s:
                    keys_pressed[8] = 1;
                    break;

                case SDLK_d:
                    keys_pressed[9] = 1;
                    break;

                case SDLK_z:
                    keys_pressed[10] = 1;
                    break;

                case SDLK_c:
                    keys_pressed[11] = 1;
                    break;

                case SDLK_4:
                    keys_pressed[12] = 1;
                    break;

                case SDLK_r:
                    keys_pressed[13] = 1;
                    break;

                case SDLK_f:
                    keys_pressed[14] = 1;
                    break;

                case SDLK_v:
                    keys_pressed[15] = 1;
                    break;
            }
        }

        if (e->type == SDL_KEYUP)
        {
            switch(e->key.keysym.sym)
            {
                case SDLK_x:
                    keys_pressed[0] = 0;
                    break;

                case SDLK_1:
                    keys_pressed[1] = 0;
                    break;

                case SDLK_2:
                    keys_pressed[2] = 0;
                    break;

                case SDLK_3:
                    keys_pressed[3] = 0;
                    break;

                case SDLK_q:
                    keys_pressed[4] = 0;
                    break;

                case SDLK_w:
                    keys_pressed[5] = 0;
                    break;

                case SDLK_e:
                    keys_pressed[6] = 0;
                    break;

                case SDLK_a:
                    keys_pressed[7] = 0;
                    break;

                case SDLK_s:
                    keys_pressed[8] = 0;
                    break;

                case SDLK_d:
                    keys_pressed[9] = 0;
                    break;

                case SDLK_z:
                    keys_pressed[10] = 0;
                    break;

                case SDLK_c:
                    keys_pressed[11] = 0;
                    break;

                case SDLK_4:
                    keys_pressed[12] = 0;
                    break;

                case SDLK_r:
                    keys_pressed[13] = 0;
                    break;

                case SDLK_f:
                    keys_pressed[14] = 0;
                    break;

                case SDLK_v:
                    keys_pressed[15] = 0;
                    break;
            }
        }
    }
}

void LoadFile(Memory *mem, const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Error opening file\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    int16_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size != fread(&(mem->raw[512]), sizeof(int8_t), size, f))
    {
        printf("Error reading file\n");
        return;
    }
    fclose(f);
}

void ExecuteInstruction(Memory *mem)
{
    uint16_t op = mem->raw[mem->pc] << 8;
    op |= mem->raw[mem->pc+1];
    mem->pc += 2;
    //printf("%05d: 0x%04X\n", loop_count, op);
    //getchar();

    switch ((op >> 12) & 0xF)
    {
        case 0x0:
        {
            if ((op & 0xF) == 0x0)
            {
                // CLS
                memset(mem->display, 0, 64*32/8);
            }
            else if ((op & 0xF) == 0xE)
            {
                // RET
                mem->pc = mem->stack[mem->sp--];
            }
            break;
        }

        case 0x1:
        {
            // Jump
            mem->pc = op & 0xFFF;
            break;
        }

        case 0x2:
        {
            // Call
            mem->stack[++mem->sp] = mem->pc;
            mem->pc = op & 0xFFF;
            break;
        }

        case 0x3:
        {
            // Skip if eq
            uint8_t x = (op >> 8) & 0xF;
            if (mem->v[x] == (op & 0xFF))
            {
                mem->pc += 2;
            }
            break;
        }

        case 0x4:
        {
            // Skip if neq
            uint8_t x = (op >> 8) & 0xF;
            if (mem->v[x] != (op & 0xFF))
            {
                mem->pc += 2;
            }
            break;
        }

        case 0x5:
        {
            // Skip if eq (reg)
            uint8_t x = (op >> 8) & 0xF;
            uint8_t y = (op >> 4) & 0xF;
            if (mem->v[x] == mem->v[y])
            {
                mem->pc += 2;
            }
            break;
        }

        case 0x6:
        {
            // Set (MOV)
            uint8_t x = (op >> 8) & 0xF;
            mem->v[x] = op & 0xFF;
            break;
        }

        case 0x7:
        {
            // Add
            uint8_t x = (op >> 8) & 0xF;
            mem->v[x] += (op & 0xFF);
            break;
        }

        case 0x8:
        {
            uint8_t x = (op >> 8) & 0xF;
            uint8_t y = (op >> 4) & 0xF;
            uint8_t n = op & 0xF;

            switch(n)
            {
                case 0x0:
                    // Mov
                    mem->v[x] = mem->v[y];
                    break;

                case 0x1:
                    // OR
                    mem->v[x] |= mem->v[y];
                    break;

                case 0x2:
                    // AND
                    mem->v[x] &= mem->v[y];
                    break;

                case 0x3:
                    // XOR
                    mem->v[x] ^= mem->v[y];
                    break;            

                case 0x4:
                {
                    // Add
                    uint16_t r = mem->v[x] + mem->v[y];
                    mem->v[x] = r & 0xFF;
                    if (r > 255)
                    {
                        mem->v[15] = 1;
                    }
                    else
                    {
                        mem->v[15] = 0;
                    }

                    break;
                }

                case 0x5:
                    // Sub
                    if (mem->v[x] > mem->v[y])
                    {
                        mem->v[15] = 1;
                    }
                    else
                    {
                        mem->v[15] = 0;
                    }
                    mem->v[x] -= mem->v[y];
                    break;

                case 0x6:
                    // Shift right
                    // NOTE(Lucas): Test this since Registers are signed (Arithmetic shift)
                    if (mem->v[x] & 1)
                    {
                        mem->v[15] = 1;
                    }
                    else
                    {
                        mem->v[15] = 0;
                    }
                    mem->v[x] >>= 1;
                    break;

                case 0x7:
                    // SubN
                    if (mem->v[y] > mem->v[x])
                    {
                        mem->v[15] = 1;
                    }
                    else
                    {
                        mem->v[15] = 0;
                    }
                    mem->v[x] = mem->v[y] - mem->v[x];
                    break;

                case 0xE:
                    // Shift left
                    if (mem->v[x] & 0x80)
                    {
                        mem->v[15] = 1;
                    }
                    else
                    {
                        mem->v[15] = 0;
                    }
                    mem->v[x] <<= 1;
                    break;
            }

            break;
        }

        case 0x9:
        {
            // Skip if neq
            uint8_t x = (op >> 8) & 0xF;
            uint8_t y = (op >> 4) & 0xF;
            if (mem->v[x] != mem->v[y])
            {
                mem->pc += 2;
            }
            break;
        }

        case 0xA:
        {
            // Set I
            mem->i = op & 0xFFF;
            break;
        }

        case 0xB:
        {
            // Jump from
            mem->pc = (op & 0xFFF) + mem->v[0];
            break;
        }

        case 0xC:
        {
            // Random
            uint8_t rnd = (uint8_t)rand();
            uint8_t x = (op >> 8) & 0xF;
            mem->v[x] = rnd & (op & 0xFF);
            break;
        }

        case 0xD:
        {
            // Draw sprite
            uint16_t I = mem->i;
            uint8_t x = (op >> 8) & 0xF;
            uint8_t y = (op >> 4) & 0xF;
            x = mem->v[x];
            y = mem->v[y];
            uint8_t n = op & 0xF;
            int8_t collision = 0;

            for (int i = 0; i < n; ++i)
            {
                uint8_t off = x & 7;
                uint16_t row = mem->raw[I++] << (8-off);
                int8_t off_row = (x + 16) - 64;
                if (off_row < 0) off_row = 0;
                // Clip
                row &= (uint16_t)(0xFFFF << off_row);
                uint16_t index = (64*y + x)/8;
                uint8_t tmp = mem->display[index];
                mem->display[index] ^= (uint8_t)((row >> 8) & 0xFF);
                if ((tmp & mem->display[index]) != tmp) collision++;

                tmp = mem->display[++index];
                mem->display[index] ^= (uint8_t)(row & 0xFF);
                if ((tmp & mem->display[index]) != tmp) collision++;

                ++y;
                if (y > 31) break;
            }

            if (collision)
            {
                mem->v[15] = 1;
            }
            else
            {
                mem->v[15] = 0;
            }
            break;
        }

        case 0xE:
        {
            if ((op & 0xFF) == 0x9E)
            {
                // Skip if key pressed
                uint8_t key = (op >> 8) & 0xF;
                key = mem->v[key];

                if (keys_pressed[key])
                {
                    mem->pc += 2;
                }
            }
            else if ((op & 0xFF) == 0xA1)
            {
                // Skip if not pressed
                uint8_t key = (op >> 8) & 0xF;
                key = mem->v[key];
                if (!keys_pressed[key])
                {
                    mem->pc += 2;
                }
            }
            break;
        }

        case 0xF:
        {
            uint8_t x = (op >> 8) & 0xF;
            switch (op & 0xFF)
            {
                case 0x07:
                    mem->v[x] = mem->delay;
                    break;

                case 0x0A:
                {
                    // Wait until a key is pressed and store it in Vx
                    uint8_t pressed = 0;
                    for (int i = 0; i < 16; ++i)
                    {
                        if (keys_pressed[i])
                        {
                            mem->v[x] = i;
                            pressed = 1;
                            break;
                        }
                    }

                    if (!pressed)
                    {
                        mem->pc -= 2;
                    }
                    break;
                }

                case 0x15:
                    mem->delay = mem->v[x];
                    break;

                case 0x18:
                    mem->sound = mem->v[x];
                    break;

                case 0x1E:
                    mem->i += mem->v[x];
                    break;

                case 0x29:
                    mem->i = &(mem->fonts[x*5]) - mem->raw;
                    break;

                case 0x33:
                    mem->raw[mem->i] = mem->v[x] / 100;
                    mem->raw[mem->i + 1] = (mem->v[x] % 100) / 10;
                    mem->raw[mem->i + 2] = mem->v[x] % 10;
                    break;
             
                case 0x55:
                    for (int i = 0; i <= x; ++i)
                    {
                        mem->raw[mem->i++] = mem->v[i];
                    }
                    break;

                case 0x65:
                    for (int i = 0; i <= x; ++i)
                    {
                        mem->v[i] = mem->raw[mem->i++];
                    }
                    break;
            }
            break;
        }

        default:
            printf("Default case Operation");
            break;
    }
}

void DisplayToPixel(Memory *mem, uint8_t *pix)
{
    for (int i=0; i < 64*32/8; ++i)
    {
        uint8_t byte = mem->display[i];
        for (int j = 7; j >= 0; --j)
        {
            if ((byte >> j) & 1)
            {
                *pix++ = 0xFF;
            }
            else
            {
                *pix++ = 0x00;
            }
        }
    }
}

void Update(Memory *mem)
{
    if (mem->delay > 0)
    {
        mem->delay--;
        printf("Delay: %d\n", mem->delay);
    }

    if (mem->sound > 0)
    {
        // TODO: Beep 
        mem->sound--;
    }
}


int main(int argc, char **argv)
{
    Memory mem;
    Init(&mem);
    LoadFile(&mem, argv[1]);
    // Set PC to first instruction
    mem.pc = 512;

    SDL_Window *window = SDL_CreateWindow(
            argv[1], 
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            64 * 8,
            32 * 8,
            0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGB332,
            SDL_TEXTUREACCESS_STREAMING,
            64, 32);

    SDL_Event event;

    uint32_t timers_span = SDL_GetTicks();
    uint32_t last_instr = SDL_GetTicks();

    while (!quit)
    {
        if ((SDL_GetTicks() - timers_span) > 16)
        {
            timers_span = SDL_GetTicks();
            Update(&mem);
        }
        if (SDL_GetTicks() > last_instr)
        {
            last_instr = SDL_GetTicks();
            GetInput(&event);
            ExecuteInstruction(&mem);
            // Render
            uint8_t pix[64*32];
            DisplayToPixel(&mem, pix);
            SDL_UpdateTexture(texture, NULL, pix, 64);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            ++loop_count;
        }
        //printf("Loop count: %d\n", ++loop_count);
    }

    SDL_Quit();
    return 0;
}

