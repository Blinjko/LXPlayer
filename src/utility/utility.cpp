#include <utility/utility.h>
#include <sdl/sdl.h>

extern "C"
{
#include <SDL2/SDL.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
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

    // prints the provided std::string message and the error message for the given PaError code
    void portaudio_print_error(const std::string &message, PaError error_code)
    {
        std::string error_message{};

        if(error_code == paUnanticipatedHostError)
        {
            const PaHostErrorInfo *error_info{Pa_GetLastHostErrorInfo()};
            error_message.append(error_info->errorText);
        }
        else
        {
            error_message.append(Pa_GetErrorText(error_code));
        }

        std::cerr << "Programmer message: " << message << std::endl;
        std::cerr << "PortAudio error message: " << error_message << std::endl;
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

    void portaudio_error_assert(bool indicator, const std::string &message, PaError error_code)
    {
        if(!indicator)
        {
            portaudio_print_error(message, error_code);
            std::exit(1);
        }
    }

    // checks to see if image rescaling is needed, returns true for yes, false for no
    // if rescaling is needed, ffmpeg_output_format is set to the appropriate the format to convert to
    // sdl_format will always be set to the apropriate SDL_PixelFormat for output
    bool rescaling_needed(enum AVPixelFormat ffmpeg_input_format, enum AVPixelFormat &ffmpeg_output_format, uint32_t &sdl_format)
    {
        switch(ffmpeg_input_format)
        {
            case AV_PIX_FMT_YUV420P:
                sdl_format = SDL_PIXELFORMAT_YV12;
                ffmpeg_output_format = ffmpeg_input_format;
                return false;
            
            case AV_PIX_FMT_YUYV422:
            case AV_PIX_FMT_YUV422P:
            case AV_PIX_FMT_YUV444P:
            case AV_PIX_FMT_YUV410P:
            case AV_PIX_FMT_YUV411P:
            case AV_PIX_FMT_UYVY422:
            case AV_PIX_FMT_YUV440P:
            case AV_PIX_FMT_YUVA420P:
            case AV_PIX_FMT_YUV420P16LE:
            case AV_PIX_FMT_YUV420P16BE:
            case AV_PIX_FMT_YUV422P16LE:
            case AV_PIX_FMT_YUV422P16BE:
            case AV_PIX_FMT_YUV444P16LE:
            case AV_PIX_FMT_YUV444P16BE:
            case AV_PIX_FMT_YUV420P9LE:
            case AV_PIX_FMT_YUV420P10BE:
            case AV_PIX_FMT_YUV420P10LE:
            case AV_PIX_FMT_YUV422P10BE:
            case AV_PIX_FMT_YUV422P10LE:
            case AV_PIX_FMT_YUV444P9BE:
            case AV_PIX_FMT_YUV444P9LE:
            case AV_PIX_FMT_YUV444P10BE:
            case AV_PIX_FMT_YUV444P10LE:
            case AV_PIX_FMT_YUV422P9BE:
            case AV_PIX_FMT_YUV422P9LE:
            case AV_PIX_FMT_YUVA422P:
            case AV_PIX_FMT_YUVA444P:
            case AV_PIX_FMT_YUVA420P9BE:
            case AV_PIX_FMT_YUVA420P9LE:
            case AV_PIX_FMT_YUVA422P9BE:
            case AV_PIX_FMT_YUVA422P9LE:
            case AV_PIX_FMT_YUVA444P9BE:
            case AV_PIX_FMT_YUVA444P9LE:
            case AV_PIX_FMT_YUVA420P10BE:
            case AV_PIX_FMT_YUVA420P10LE:
            case AV_PIX_FMT_YUVA422P10BE:
            case AV_PIX_FMT_YUVA422P10LE:
            case AV_PIX_FMT_YUVA444P10BE:
            case AV_PIX_FMT_YUVA444P10LE:
            case AV_PIX_FMT_YUVA420P16BE:
            case AV_PIX_FMT_YUVA420P16LE:
            case AV_PIX_FMT_YUVA422P16BE:
            case AV_PIX_FMT_YUVA422P16LE:
            case AV_PIX_FMT_YUVA444P16BE:
            case AV_PIX_FMT_YUVA444P16LE:
            case AV_PIX_FMT_YVYU422:
            case AV_PIX_FMT_YUV420P12BE:
            case AV_PIX_FMT_YUV420P12LE:
            case AV_PIX_FMT_YUV420P14BE:
            case AV_PIX_FMT_YUV420P14LE:
            case AV_PIX_FMT_YUV422P12BE:
            case AV_PIX_FMT_YUV422P12LE:
            case AV_PIX_FMT_YUV422P14BE:
            case AV_PIX_FMT_YUV422P14LE:
            case AV_PIX_FMT_YUV444P12BE:
            case AV_PIX_FMT_YUV444P12LE:
            case AV_PIX_FMT_YUV444P14BE:
            case AV_PIX_FMT_YUV444P14LE:
            case AV_PIX_FMT_YUV440P10LE:
            case AV_PIX_FMT_YUV440P10BE:
            case AV_PIX_FMT_YUV440P12LE:
                sdl_format = SDL_PIXELFORMAT_YV12;
                ffmpeg_output_format = AV_PIX_FMT_YUV420P;
                return true;

            case AV_PIX_FMT_NV12:
                sdl_format = SDL_PIXELFORMAT_NV12;
                ffmpeg_output_format = ffmpeg_input_format;
                return false;

            case AV_PIX_FMT_NV21:
                sdl_format = SDL_PIXELFORMAT_NV21;
                ffmpeg_output_format = ffmpeg_input_format;
                return false;

            default:
                ffmpeg_output_format = AV_PIX_FMT_RGB24;
                sdl_format = SDL_PIXELFORMAT_RGB24;
                return true;
        }
    }

    bool valid_rescaling_input(enum AVPixelFormat pixel_format)
    {
        int result{sws_isSupportedInput(pixel_format)};

        if(result > 0)
        {
            return true;
        }

        else
        {
            return false;
        }
    }


    // function that dictates wheather audio resampling is needed
    // If conversion is needed, output_format will be set to the appropriate AVSampleFormat, otherise it will be the same as input_format
    // portaudio_format will always be set to the output format for portaudio to use
    bool resampling_needed(enum AVSampleFormat input_format, enum AVSampleFormat &output_format, PaSampleFormat &portaudio_format, bool &interleaved)
    {
        switch(input_format)
        {
            case AV_SAMPLE_FMT_U8:
                portaudio_format = paUInt8;
                output_format = input_format;
                interleaved = true;
                return false;

            case AV_SAMPLE_FMT_S16:
                portaudio_format = paInt16;
                output_format = input_format;
                interleaved = true;
                return false;

            case AV_SAMPLE_FMT_S32:
                portaudio_format = paInt32;
                output_format = input_format;
                interleaved = true;
                return false;

            case AV_SAMPLE_FMT_FLT:
                portaudio_format = paFloat32;
                output_format = input_format;
                interleaved = true;
                return false;

            case AV_SAMPLE_FMT_DBL:
                portaudio_format = paFloat32;
                output_format = AV_SAMPLE_FMT_FLT;
                interleaved = true;
                return true;

            case AV_SAMPLE_FMT_U8P:
                portaudio_format = paUInt8 | paNonInterleaved;
                output_format = input_format;
                interleaved = false;
                return false;

            case AV_SAMPLE_FMT_S16P:
                portaudio_format = paInt16 | paNonInterleaved;
                output_format = input_format;
                interleaved = false;
                return false;

            case AV_SAMPLE_FMT_S32P:
                portaudio_format = paInt32 | paNonInterleaved;
                output_format = input_format;
                interleaved = false;
                return false;

            case AV_SAMPLE_FMT_FLTP:
                portaudio_format = paFloat32 | paNonInterleaved;
                output_format = input_format;
                interleaved = false;
                return false;

            case AV_SAMPLE_FMT_DBLP:
                portaudio_format = paFloat32 | paNonInterleaved;
                output_format = AV_SAMPLE_FMT_FLTP;
                interleaved = false;
                return true;

            case AV_SAMPLE_FMT_S64:
                portaudio_format = paInt32;
                output_format = AV_SAMPLE_FMT_S32;
                interleaved = true;
                return true;

            case AV_SAMPLE_FMT_S64P:
                portaudio_format = paInt32 | paNonInterleaved;
                output_format = AV_SAMPLE_FMT_S32P;
                interleaved = false;
                return true;

            default:
                std::cerr << "Unkonw FFmpeg sample format, cannot convert" << std::endl;
                return true;
        }
    }
}
