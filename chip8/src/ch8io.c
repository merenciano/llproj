#include "ch8io.h"

#include <SDL2/SDL.h>

void InitIO(IOData *data)
{
    data->window = (void*)SDL_CreateWindow(
            "CHIP8", 
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            64 * 8,
            32 * 8,
            0);
    data->renderer = (void*)SDL_CreateRenderer(data->window, -1, 0);
    data->texture = (void*)SDL_CreateTexture(
            data->renderer,
            SDL_PIXELFORMAT_RGB332,
            SDL_TEXTUREACCESS_STREAMING,
            64, 32);
    
}

uint8_t GetInput(uint8_t *keys)
{
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
        if (e.type == SDL_QUIT)
        {
            return 0;
        }

        if (e.type == SDL_KEYDOWN)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_x:
                    keys[0] = 1;
                    break;

                case SDLK_1:
                    keys[1] = 1;
                    break;

                case SDLK_2:
                    keys[2] = 1;
                    break;

                case SDLK_3:
                    keys[3] = 1;
                    break;

                case SDLK_q:
                    keys[4] = 1;
                    break;

                case SDLK_w:
                    keys[5] = 1;
                    break;

                case SDLK_e:
                    keys[6] = 1;
                    break;

                case SDLK_a:
                    keys[7] = 1;
                    break;

                case SDLK_s:
                    keys[8] = 1;
                    break;

                case SDLK_d:
                    keys[9] = 1;
                    break;

                case SDLK_z:
                    keys[10] = 1;
                    break;

                case SDLK_c:
                    keys[11] = 1;
                    break;

                case SDLK_4:
                    keys[12] = 1;
                    break;

                case SDLK_r:
                    keys[13] = 1;
                    break;

                case SDLK_f:
                    keys[14] = 1;
                    break;

                case SDLK_v:
                    keys[15] = 1;
                    break;
            }
        }
        else if (e.type == SDL_KEYUP)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_x:
                    keys[0] = 0;
                    break;

                case SDLK_1:
                    keys[1] = 0;
                    break;

                case SDLK_2:
                    keys[2] = 0;
                    break;

                case SDLK_3:
                    keys[3] = 0;
                    break;

                case SDLK_q:
                    keys[4] = 0;
                    break;

                case SDLK_w:
                    keys[5] = 0;
                    break;

                case SDLK_e:
                    keys[6] = 0;
                    break;

                case SDLK_a:
                    keys[7] = 0;
                    break;

                case SDLK_s:
                    keys[8] = 0;
                    break;

                case SDLK_d:
                    keys[9] = 0;
                    break;

                case SDLK_z:
                    keys[10] = 0;
                    break;

                case SDLK_c:
                    keys[11] = 0;
                    break;

                case SDLK_4:
                    keys[12] = 0;
                    break;

                case SDLK_r:
                    keys[13] = 0;
                    break;

                case SDLK_f:
                    keys[14] = 0;
                    break;

                case SDLK_v:
                    keys[15] = 0;
                    break;
            }
        }
    }
    return  1;
}

// Create screen pixels from CHIP8 display bitmap
static void DisplayToPixel(uint8_t *display, uint8_t *pix)
{
    for (int i=0; i < 64*32/8; ++i)
    {
        uint8_t byte = display[i];
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

void RenderBitmap(IOData *data, uint8_t *bitmap)
{
    if (!data || !data->window)
    {
        // I/O data struct not initialized
        return;
    }

    SDL_Renderer *renderer = (SDL_Renderer*)data->renderer;
    SDL_Texture *texture = (SDL_Texture*)data->texture;

    uint8_t pix[64*32];
    DisplayToPixel(bitmap, pix);
    SDL_UpdateTexture(texture, NULL, pix, 64);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

uint32_t TimeMS()
{
    return SDL_GetTicks();
}
