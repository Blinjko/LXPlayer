extern "C"
{
#include <portaudio.h>
}

#include <iostream>
#include <cstdlib>

namespace PortAudio
{

    // A simple struct wrapper around the Pa_Initialize, and Pa_Terminate calls
    // This will ensure Pa_Terminate will be called for every time Pa_Initialize was called
    struct Initializer
    {
        Initializer()
        {
            PaError error = 0;
            error = Pa_Initialize();

            if(error != paNoError)
            {
                std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(error) << std::endl;
                std::exit(1);
            }
        }

        ~Initializer()
        {
            PaError error = 0;
            error = Pa_Terminate();

            if(error != paNoError)
            {
                std::cerr << "Failed to terminate PortAudio: " << Pa_GetErrorText(error) << std::endl;
                std::exit(1);
            }
        }
    };
}
