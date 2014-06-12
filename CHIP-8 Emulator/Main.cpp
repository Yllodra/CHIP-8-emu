#include "CHIP8.h" // Cpu core implementation.
#include <iostream>
#include <SDL.h>
#include <string>
#include <vector>


CHIP8 CHIP8_core;

const int WINDOW_WIDTH = 64;
const int WINDOW_HEIGHT = 32;
int WINDOW_MODIFIER = 10;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

bool quit = false;
SDL_Event e; // SDL_Event is implemented as a queue with SDL_PollEvent reading the oldest event.

/**
* Log an SDL error with some error messages to the output stream of our choice.
* @param os. The output stream to write the message to.
* @param msg. The error message. Format will be @msg error: SDL_GetError().
*/
void logSDLError(std::ostream &os, const std::string &msg)
{
    os << msg << " error: " << SDL_GetError() << std::endl;
}


int setupSDL()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        logSDLError(std::cout, "SDL_Init");

        SDL_Quit();
        return -1;
    }

    window = SDL_CreateWindow("CHIP-8 Emulator", 100, 100, WINDOW_WIDTH*WINDOW_MODIFIER,
                              WINDOW_HEIGHT*WINDOW_MODIFIER, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        logSDLError(std::cout, "CreateWindow");

        SDL_DestroyWindow(window);
        SDL_Quit();
        return -2;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED |
                                  SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        logSDLError(std::cout, "CreateRenderer");

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    SDL_RenderSetScale(renderer, (float)WINDOW_MODIFIER, (float)WINDOW_MODIFIER);
    return 0;
}

void draw()
{
    auto gfx = CHIP8_core.gfx();
    std::vector<SDL_Point> white_points;
    std::vector<SDL_Point> black_points;

    for (int x = 0; x < WINDOW_WIDTH; ++x)
    {
        for (int y = 0; y < WINDOW_HEIGHT; ++y)
        {
            if (gfx.at(x+y*WINDOW_WIDTH))
            {
                white_points.emplace_back();
                white_points.back().x = x;
                white_points.back().y = y;
            }
            else
            {
                black_points.emplace_back();
                black_points.back().x = x;
                black_points.back().y = y;
            }
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White.
    SDL_RenderDrawPoints(renderer, white_points.data(), white_points.size());
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black.
    SDL_RenderDrawPoints(renderer, black_points.data(), black_points.size());

    SDL_RenderPresent(renderer);
    CHIP8_core.draw_flag(false);
}

void handleInput()
{
    // SDL_PollEvent reads the oldest event on the event queue.
    while (SDL_PollEvent(&e) != 0 )
    {
        if (e.type == SDL_QUIT) // "Xsing out of the window", ie pressing top right X button.
            quit = true;
        else if (e.type == SDL_KEYDOWN)
        {
            switch (e.key.keysym.sym)
            {
            case 27:
                quit = true;
                break;
            case 49: // 1
                CHIP8_core.setKeys(0x1, true);
                break;
            case 50: // 2
                CHIP8_core.setKeys(0x2, true);
                break;
            case 51: // 3
                CHIP8_core.setKeys(0x3, true);
                break;
            case 52: // 4
                CHIP8_core.setKeys(0xC, true);
                break;
            case 113: // q
                CHIP8_core.setKeys(0x4, true);
                break;
            case 119: // w
                CHIP8_core.setKeys(0x5, true);
                break;
            case 101: // e
                CHIP8_core.setKeys(0x6, true);
                break;
            case 114: // r
                CHIP8_core.setKeys(0xD, true);
                break;
            case 97: // a
                CHIP8_core.setKeys(0x7, true);
                break;
            case 115: // s
                CHIP8_core.setKeys(0x8, true);
                break;
            case 100: // d
                CHIP8_core.setKeys(0x9, true);
                break;
            case 102: // f
                CHIP8_core.setKeys(0xE, true);
                break;
            case 122: // z
                CHIP8_core.setKeys(0xA, true);
                break;
            case 120: // x
                CHIP8_core.setKeys(0x0, true);
                break;
            case 99: // c
                CHIP8_core.setKeys(0xB, true);
                break;
            case 118: // v
                CHIP8_core.setKeys(0xF, true);
                break;
            default:
                break;
            }
        }
        else if (e.type == SDL_KEYUP)
        {
            switch (e.key.keysym.sym)
            {
            case 49: // 1
                CHIP8_core.setKeys(0x1, false);
                break;
            case 50: // 2
                CHIP8_core.setKeys(0x2, false);
                break;
            case 51: // 3
                CHIP8_core.setKeys(0x3, false);
                break;
            case 52: // 4
                CHIP8_core.setKeys(0xC, false);
                break;
            case 113: // q
                CHIP8_core.setKeys(0x4, false);
                break;
            case 119: // w
                CHIP8_core.setKeys(0x5, false);
                break;
            case 101: // e
                CHIP8_core.setKeys(0x6, false);
                break;
            case 114: // r
                CHIP8_core.setKeys(0xD, false);
                break;
            case 97: // a
                CHIP8_core.setKeys(0x7, false);
                break;
            case 115: // s
                CHIP8_core.setKeys(0x8, false);
                break;
            case 100: // d
                CHIP8_core.setKeys(0x9, false);
                break;
            case 102: // f
                CHIP8_core.setKeys(0xE, false);
                break;
            case 122: // z
                CHIP8_core.setKeys(0xA, false);
                break;
            case 120: // x
                CHIP8_core.setKeys(0x0, false);
                break;
            case 99: // c
                CHIP8_core.setKeys(0xB, false);
                break;
            case 118: // v
                CHIP8_core.setKeys(0xF, false);
                break;
            default:
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    // Check correct argument usage.
    if (argc <= 1)
    {
        std::cout << "Program must take an argument, the full path to the file to be loaded." << std::endl;
        return 0;
    }

    // Set up SDL.
    setupSDL();

    // Load the program into memory.
    CHIP8_core.loadGame(argv[1]);
    
    // Emulation loop
    while(!quit)
    {
        // Emulate one cycle.
        CHIP8_core.emulateCycle();
        // If the draw flag is set, update the screen.
        if (CHIP8_core.draw_flag())
            draw();

        // Store key press state (press and release).
        handleInput();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}