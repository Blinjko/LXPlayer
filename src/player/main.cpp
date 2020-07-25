/* Steps of decoding a file using libav
 * 1. Allocate an AVFormatContext
 * 2. Open the file with the allocated AVFormatContext
 * 3. Read some stream info from the file to possibly get extra needed information for decoding
 * 4. Find the index of the stream we want to decode
 * 5. Get the proper AVCodec for the stream we want to decode
 * 6. Allocate an AVCodecContext using the found AVCodec
 * 7. Copy stream AVCodecParameters to the allocated AVCodecContext
 * 8. Open the allocated AVCodecContext
 * 9. Allocate an AVPacket
 * 10. Allocate an AVFrame
 * 11. Read data from the AVFormatContext into the allocated AVPacked
 * 12. Make sure the AVPacket holds data from the correct stream, if it doesn't unreference the AVPacket and goto step 11
 * 13. Send the AVPacket to the decoder
 * 14. Recieve decoded data from the decoder and put it into the allocated AVFrame
 * 16. If decoder needs more data, goto step 11 until it gives decoded data
 * 17. Unreference the frame and goto step 11
 */

#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>

#include <ffmpeg/decoder.h>
#include <ffmpeg/frame.h>
#include <sdl/sdl.h>
#include <utility/semaphore.h>

SDL_Rect get_native_resolution();

void initialize_sdl_window(SDL::Window&, SDL::Renderer&);
void render_frame(SDL::Texture&, SDL::Renderer&, AVFrame*);

void decoder_thread_function(FFmpeg::Decoder&, FFmpeg::Frame_Array&, Utility::Semaphore&, Utility::Semaphore&);

void print_error(int error_number);

void sig_interrupt_handler(int signal)
{
    std::cout << "Interrupted" << std::endl;
    std::exit(signal);
}

void sig_terminate_handler(int signal)
{
    std::cout << "Terminated" << std::endl;
    std::exit(signal);
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cerr << "Invalid Usage" << std::endl;
        std::cerr << "Valid Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::signal(SIGINT, sig_interrupt_handler);
    std::signal(SIGTERM, sig_terminate_handler);


    SDL::Initializer sdl_init{SDL_INIT_VIDEO};

    SDL::Window window{};
    SDL::Renderer renderer{};
    SDL::Texture texture{};

    initialize_sdl_window(window, renderer);

    std::string filename{argv[1]};

    FFmpeg::Decoder decoder{};

    int error{0};

    error = decoder.init_format_context(filename, nullptr);
    print_error(error);

    error = decoder.find_stream(AVMEDIA_TYPE_VIDEO);
    print_error(error);

    int thread_count{1};
    std::cout << "How many decoding threads would you like: ";
    std::cin >> thread_count;
    error = decoder.init_codec_context(nullptr, thread_count);
    print_error(error);

    std::cout << "Thread Count: " << decoder.codec_context()->thread_count << std::endl;


    AVFrame *frame{nullptr};
    while(1)
    {
        error = decoder.send_packet();
        if(error == AVERROR_EOF)
        {
            break;
        }

        else if(error == AVERROR(EAGAIN))
        {
            break;
        }

        print_error(error);
    }


    error = decoder.receive_frame(&frame);
    print_error(error);

    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_YV12,
                                SDL_TEXTUREACCESS_STATIC,
                                frame->width,
                                frame->height);
    if(!texture)
    {
        std::cerr << "Failed to create SDL_Texture: " << SDL_GetError() << std::endl;
        return 1;
    }



    const AVStream *stream{decoder.format_context()->streams[decoder.stream_number()]};
    double timebase{static_cast<double>(stream->time_base.num) / stream->time_base.den};
    int framerate{stream->r_frame_rate.num / stream->r_frame_rate.den};
    int buffer_size{framerate * 2};

    FFmpeg::Frame_Array decoded_frames{buffer_size};

    for(int i{0}; i != decoded_frames.size(); ++i)
    {
        error = decoded_frames[i].allocate(static_cast<enum AVPixelFormat>(frame->format),
                                           frame->width,
                                           frame->height);
        print_error(error);
    }

    Utility::Semaphore spots_filled{0};
    Utility::Semaphore spots_empty{buffer_size};

    std::thread decoder_thread{decoder_thread_function,
                               std::ref(decoder),
                               std::ref(decoded_frames),
                               std::ref(spots_filled),
                               std::ref(spots_empty)};
    
    while(spots_filled.count() != buffer_size)
    {
    }

    render_frame(texture, renderer, frame);

    int frames{0};
    int current_index{0};
    auto start_time{std::chrono::steady_clock::now()};
    auto last_frame_time{start_time};

    while(1)
    {
        spots_filled.wait();

        if(current_index == decoded_frames.size())
        {
            current_index = 0;
        }
        
        if(decoded_frames[current_index]->pts == AVERROR_EOF)
        {
            break;
        }

        double frame_display_time{decoded_frames[current_index]->pts * timebase};
        auto current_time{std::chrono::steady_clock::now()};
        std::chrono::duration<double> difference{current_time - start_time};

        while(difference.count() < frame_display_time)
        {
            current_time = std::chrono::steady_clock::now();
            difference = current_time - start_time;
        }


        render_frame(texture, renderer, decoded_frames[current_index]);

        spots_empty.post();

        frames++;
        current_index++;

        current_time = std::chrono::steady_clock::now();
        difference = current_time - last_frame_time;

        if(difference.count() >= 1.0)
        {
            std::cout << "Frames: " << frames << std::endl;
            frames = 0;

            last_frame_time = current_time;
        }
    }


    auto end_time{std::chrono::steady_clock::now()};
    std::chrono::duration<double> diff{end_time - start_time};
    std::cout << "Total time: " << diff.count() / 60 << std::endl;

    decoder_thread.join();
    return 0;
}

inline void print_error(int error_number)
{
    if(error_number == -1111)
    {
        std::cerr << "Failed to allocate one of the following: AVFrame, AVPacket, AVFormatContext, AVCodecContext, or failed to open AVCodecContext, or failed to get AVCodec" << std::endl;
        std::exit(1);
    }

    else if(error_number < 0)
    {
        std::string message{};
        int error{0};

        char buff[256];
        error = av_strerror(error_number, buff, sizeof(buff));

        // failed to find error value
        if(error < 0)
        {
            message = "Unkown Error Code";
        }

        else
        {
            message = buff;
        }

        std::cerr << "ERROR: " << message << std::endl;
        std::exit(1);
    }
}

SDL_Rect get_native_resolution()
{
    SDL_Rect rect;
    rect.w = -1;
    rect.h = -1;

    SDL::Window window{};
    window = SDL_CreateWindow("TEMP WINDOW", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_HIDDEN);

    if(!window)
    {
        return rect;
    }

    int display_index{0};

    display_index = SDL_GetWindowDisplayIndex(window);
    if(display_index < 0)
    {
        return rect;
    }

    display_index = SDL_GetDisplayUsableBounds(display_index, &rect);
    if(display_index < 0)
    {
        return rect;
    }

    return rect;
}

void initialize_sdl_window(SDL::Window &window, SDL::Renderer &renderer)
{
    SDL_Rect display_resolution{get_native_resolution()};
    if(display_resolution.w == -1)
    {
        std::cerr << "Failed to get display resolution: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    std::cout << "Native width: " << display_resolution.w << std::endl;
    std::cout << "Native height: " << display_resolution.h << std::endl;

    window = SDL_CreateWindow("FFmpeg Threading Test",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              display_resolution.w,
                              display_resolution.h,
                              SDL_WINDOW_RESIZABLE);

    if(!window)
    {
        std::cerr << "Failed to create SDL_Window: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer)
    {
        std::cerr << "Failed to create SDL_Renderer: " << SDL_GetError() << std::endl;
    }

}

void render_frame(SDL::Texture &texture, SDL::Renderer &renderer, AVFrame *frame)
{
    int error{0};

    error = SDL_UpdateYUVTexture(texture,
                                 nullptr,
                                 frame->data[0],
                                 frame->linesize[0],
                                 frame->data[1],
                                 frame->linesize[1],
                                 frame->data[2],
                                 frame->linesize[2]);

    if(error < 0)
    {
        std::cerr << "Failed to update texture: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    error = SDL_RenderClear(renderer);
    if(error < 0)
    {
        std::cerr << "Failed to clear renderer: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    /*
    int width, height;
    error = SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    if(error < 0)
    {
        std::cerr << "Failed to get texture attributes: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    SDL_Rect rect;
    rect.x = (1920 - width) / 2;
    rect.y = (1080 - height) / 2;
    rect.w = width;
    rect.h = height;
    */

    error = SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    if(error < 0)
    {
        std::cerr << "Failed to copy data from texture to renderer: " << SDL_GetError() << std::endl;
        std::exit(1);
    }
    SDL_RenderPresent(renderer);

}

void decoder_thread_function(FFmpeg::Decoder &decoder,
                             FFmpeg::Frame_Array &decoded_frames,
                             Utility::Semaphore &spots_filled,
                             Utility::Semaphore &spots_empty)
{
    bool end_of_file_reached{false};
    int current_index{0};
    int error{0};

    AVFrame *frame{nullptr};

    while(1)
    {

        error = decoder.send_packet();
        if(error == AVERROR_EOF)
        {
            end_of_file_reached = true;
        }

        else if(error != AVERROR(EAGAIN))
        {
            print_error(error);
        }

        error = decoder.receive_frame(&frame);
        if(error == AVERROR(EAGAIN) && end_of_file_reached)
        {
            decoded_frames[current_index]->pts = AVERROR_EOF;
            spots_filled.post();
            break;
        }

        else if(error == AVERROR(EAGAIN))
        {
            continue;
        }
        print_error(error);

        spots_empty.wait();

        error = decoded_frames[current_index].copy(frame);
        print_error(error);

        spots_filled.post();

        current_index++;
        if(current_index == decoded_frames.size())
        {
            current_index = 0;
        }

    }
}
