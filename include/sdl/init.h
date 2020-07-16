#pragma once
// SDL2 library headers
extern "C"
{
#include <SDL2/SDL.h>
}

#include <iostream>
#include <cstdlib>

namespace SDL
{
    /* A simple struct wrapper around the SDL_Init() and SDL_Quit() calls
     * Ensures that if SDL has been initialized it will always be unintialized
     */
    struct Initializer
    {
        Initializer(uint32_t flags)
        {
            int error = 0;
            error = SDL_Init(flags);

            if(error < 0)
            {
                std::cerr << "Failed to initialize SDL" << std::endl;
                std::exit(1);
            }
        }

        ~Initializer()
        {
            SDL_Quit();
        }
    };
}
