#pragma once

extern "C"
{
#include <SDL2/SDL.h>
#include <libavutil/pixfmt.h>
}

#include <string>


namespace Utility
{
    // Function to get screens native resolution
    SDL_Rect get_native_resolution();

    // prints the provided std::string message and the result of SDL_GetError()
    void print_error(const std::string&);

    // prints the provided std::string message and the error message for the given error code
    void print_error(const std::string&, int);

    // assert functions, they test if the boolean is true, if it is, then the provided function is called
    // Note: This function will exit the program via std::exit
    void error_assert(bool, const std::string&);
    void error_assert(bool, const std::string&, int);

    bool rescaling_needed(enum AVPixelFormat, enum AVPixelFormat&, uint32_t&);

    bool valid_rescaling_input(enum AVPixelFormat);
}
