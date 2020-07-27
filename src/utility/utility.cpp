#include <utility/utility.h>
#include <sdl/sdl.h>

extern "C"
{
#include <SDL2/SDL.h>
#include <libavutil/error.h>
}

#include <string>
#include <iostream>
#include <cstdlib>

namespace Utility
{
    // Function to get screens native resolution
    SDL_Rect get_native_resolution()
    {
        SDL_Rect rect;
        rect.w = -1;
        rect.h = -1;

        // create a temporary window
        SDL::Window window{};
        window = SDL_CreateWindow("TEMP WINDOW", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_HIDDEN);

        if(!window)
        {
            return rect;
        }

        int display_index{0};

        // get the display index of the window
        display_index = SDL_GetWindowDisplayIndex(window);
        if(display_index < 0)
        {
            return rect;
        }

        // get display bounds(resolution) of the display at display index
        display_index = SDL_GetDisplayUsableBounds(display_index, &rect);
        if(display_index < 0)
        {
            return rect;
        }

        return rect;
    }

    // prints the provided std::string message and the result of SDL_GetError()
    void print_error(const std::string &message)
    {
        std::cerr << "Programmer message: " << message << std::endl;
        std::cerr << "SDL error message: " << SDL_GetError() << std::endl;
    }

    // prints the provided std::string message and the error message for the given error code
    void print_error(const std::string &message, int error_code)
    {
        std::string error_message{};
        char buff[256];

        // get error message
        error_code = av_strerror(error_code, buff, sizeof(buff));

        if(error_code < 0)
        {
            error_message = "Unknown Error Code";
        }

        else
        {
            error_message = buff;
        }

        std::cerr << "Programmer message: " << message << std::endl;
        std::cerr << "FFmpeg error message: " << error_message << std::endl;

    }

    // Assert functions, if indicator is false then call the apropriate print error function, then exit the program
    void error_assert(bool indicator, const std::string &message)
    {
        if(!indicator)
        {
            print_error(message);
            std::exit(1);
        }
    }

    void error_assert(bool indicator, const std::string &message, int error_code)
    {
        if(!indicator)
        {
            print_error(message, error_code);
            std::exit(1);
        }
    }
}
