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
#include <ffmpeg/scale.h>
#include <sdl/sdl.h>
#include <utility/semaphore.h>
#include <utility/utility.h>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
int initialize_decoder(FFmpeg::Decoder&, const std::string&, enum AVMediaType, int);

int render_yuv_frame(SDL::Texture&, SDL_Rect*, SDL::Renderer&, AVFrame*);
int render_frame(SDL::Texture&, SDL_Rect*, SDL::Renderer&, AVFrame*);

void decoder_thread_function(FFmpeg::Decoder&, FFmpeg::Scale&, bool, FFmpeg::Frame_Array&, Utility::Semaphore&, Utility::Semaphore&);

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

    // Setup Signals
    std::signal(SIGINT, sig_interrupt_handler);
    std::signal(SIGTERM, sig_terminate_handler);


    // put passed filename into std::string
    std::string filename{argv[1]};

    // Decoder Setup Start //

    FFmpeg::Decoder decoder{};
    int thread_count{1};

    // prompt user for amount of threads to use
    std::cout << "How many decoding threads would you like: ";
    std::cin >> thread_count;

    int error{0};
    error = initialize_decoder(decoder, filename, AVMEDIA_TYPE_VIDEO, thread_count);
    Utility::error_assert((error >= 0), "Failed to initialize FFmpeg Decoder", error);

    // Fill decoder up with data
    while(error != AVERROR(EAGAIN))
    {
        error = decoder.send_packet();
        Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to send packet do FFmpeg Decoder", error);
    }

    AVFrame *decoded_frame{nullptr};
    
    // get a decoded frame from the decoder
    error = decoder.receive_frame(&decoded_frame);
    Utility::error_assert((error >= 0), "Failed to reveive frame from FFmpeg Decoder", error);

    // Decoder Setup End //


    // Get Pixel Format information //

    // pixel format variables
    enum AVPixelFormat FFMPEG_INPUT_FORMAT{static_cast<enum AVPixelFormat>(decoded_frame->format)};
    enum AVPixelFormat FFMPEG_OUTPUT_FORMAT;
    uint32_t SDL_TEXTURE_FORMAT;
    bool YUV_IMAGE_OUTPUT;

    bool RESCALING_NEEDED { Utility::rescaling_needed(FFMPEG_INPUT_FORMAT, FFMPEG_OUTPUT_FORMAT, SDL_TEXTURE_FORMAT) };

    // if rescaling is needed, but the FFMPEG_INPUT_FORMAT is not supported then the video cannot be played
    if(RESCALING_NEEDED && !Utility::valid_rescaling_input(FFMPEG_INPUT_FORMAT))
    {
        std::cerr << "Cannot play video unsupported pixel format" << std::endl;
        return 1;
    }

    // check for yuv image output
    if(FFMPEG_OUTPUT_FORMAT == AV_PIX_FMT_YUV420P ||
       FFMPEG_OUTPUT_FORMAT == AV_PIX_FMT_NV12 ||
       FFMPEG_OUTPUT_FORMAT == AV_PIX_FMT_NV21)
    {
        YUV_IMAGE_OUTPUT = true;
    }

    // Instantiate a Scale class
    FFmpeg::Scale rescaler{};


    // SDL Setup Start //

    SDL::Initializer sdl_initializer{SDL_INIT_VIDEO};

    SDL::Window window{};
    std::string window_title{"LXPlayer: "};
    SDL::Renderer renderer{};
    SDL::Texture texture{};
    //SDL_Rect display_rect;

    const SDL_Rect screen_resolution{Utility::get_native_resolution()};
    Utility::error_assert((screen_resolution.w > 0), "Failed to get native screen resolution");

    window_title.append(filename);
    // create the window
    window = SDL_CreateWindow(window_title.c_str(),   // Window Title
                              SDL_WINDOWPOS_CENTERED, // X Position
                              SDL_WINDOWPOS_CENTERED, // Y Position
                              screen_resolution.w,    // Width
                              screen_resolution.h,    // Height
                              SDL_WINDOW_RESIZABLE);  // Flags
    Utility::error_assert(window, "Failed to create window");

    // create the renderer
    renderer = SDL_CreateRenderer(window, // Window to use
                                  -1,     // Driver Index -1 for auto selection
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // flags
    Utility::error_assert(renderer, "Failed to create renderer");

    // create a texture
    texture = SDL_CreateTexture(renderer,                 // Renderer to use
                                SDL_TEXTURE_FORMAT,       // Pixel format
                                SDL_TEXTUREACCESS_STATIC, // Texture access
                                decoded_frame->width,     // Texture width
                                decoded_frame->height);   // Texture height
    Utility::error_assert(texture, "Failed to create texture");

    /*
    // set the display rectangle
    display_rect.x = (screen_resolution.w - decoded_frame->width) / 2;
    display_rect.y = (screen_resolution.h - decoded_frame->height) / 2;
    display_rect.w = decoded_frame->width;
    display_rect.h = decoded_frame->height;
    */

    // SDL Setup End //

    // Setup image rescaler if needed //
    if(RESCALING_NEEDED)
    {
        rescaler = sws_getContext(decoded_frame->width,   // source width
                                  decoded_frame->height,  // source height
                                  FFMPEG_INPUT_FORMAT,    // source pixel format
                                  decoded_frame->width,   // destination width
                                  decoded_frame->height,  // destination height
                                  FFMPEG_OUTPUT_FORMAT,   // destination pixel format
                                  0,                      // flags
                                  nullptr,                // src filter
                                  nullptr,                // dst filter
                                  nullptr);                

        Utility::error_assert(rescaler.swscontext(), "Failed to get rescaling context", -1111);
    }

    // Get the timebase, framerate and set a buffer size
    const AVStream *stream{decoder.format_context()->streams[decoder.stream_number()]};
    double timebase{static_cast<double>(stream->time_base.num) / stream->time_base.den};
    int framerate{stream->r_frame_rate.num / stream->r_frame_rate.den};
    int buffer_size{framerate * 2}; // by default the buffersize is enough frames for 2 seconds

    // make an array for the decoded video frames
    FFmpeg::Frame_Array decoded_frames{buffer_size};

    // allocate every frame in the array
    // This helps with performance by reducing the allocate / deallocate system calls
    // The frames are allocated once and deallocated once
    for(int i{0}; i != decoded_frames.size(); ++i)
    {
        error = decoded_frames[i].allocate(FFMPEG_OUTPUT_FORMAT,
                                           decoded_frame->width,
                                           decoded_frame->height);
        Utility::error_assert((error >= 0), "Failed to allocate an AVFrame", error);
    }

    // Create some counting semaphores for synchonization
    Utility::Semaphore spots_filled{0};
    Utility::Semaphore spots_empty{buffer_size};

    // create a new thread to decode the frames
    std::thread decoder_thread{decoder_thread_function,  // function for thread to call
                               std::ref(decoder),        // Decoder to use
                               std::ref(rescaler),       // The image rescaler to use
                               RESCALING_NEEDED,         // a boolean indicating to rescale the image
                               std::ref(decoded_frames), // Frame_Array to store decoded frames
                               std::ref(spots_filled),   // Semaphore holding total spots filled
                               std::ref(spots_empty)};   // Semaphore holding total spots empty / not filled
    

    // Inital Frame Setup //
    FFmpeg::Frame initial_frame{};
    error = initial_frame.allocate(FFMPEG_OUTPUT_FORMAT, 
                                   decoded_frame->width,
                                   decoded_frame->height);

    Utility::error_assert((error >= 0), "Failed to allocate initial frame", error);

    // rescale the first image if needed
    if(RESCALING_NEEDED)
    {
        error = sws_scale(rescaler,                       // Rescaling context to use
                decoded_frame->data,                      // Source data
                decoded_frame->linesize,                  // Source linesize
                0,                                        // Y position in source image
                decoded_frame->height,                            // height of the source image
                initial_frame->data,                      // destination data
                initial_frame->linesize);                 // destination linesize

        Utility::error_assert((error >= 0), "Failed to rescale initial image", error);
    }

    else
    {
        initial_frame.copy(decoded_frame);
    }

    // Wait for the decoder thread to fill the buffer / Frame_Array
    while(spots_filled.count() != buffer_size)
    {
    }

    // render the first image
    if(YUV_IMAGE_OUTPUT)
    {
        error = render_yuv_frame(texture, nullptr, renderer, initial_frame);
        Utility::error_assert((error >= 0), "Failed to render initial YUV frame");
    }

    else
    {
        error = render_frame(texture, nullptr, renderer, initial_frame);
        Utility::error_assert((error >= 0), "Failed to render initial frame");
    }

    int frames{0};
    int current_index{0};
//    double wait_time{0.0}; // time spend waiting / paused

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

        if(YUV_IMAGE_OUTPUT)
        {
            error = render_yuv_frame(texture, nullptr, renderer, decoded_frames[current_index]);
            Utility::error_assert((error >= 0), "Failed to render YUV frame");
        }
        else 
        {
            error = render_frame(texture, nullptr, renderer, decoded_frames[current_index]);
            Utility::error_assert((error >= 0), "Failed to render frame");
        }

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

int initialize_decoder(FFmpeg::Decoder &decoder, const std::string &filename, enum AVMediaType media_type, int thread_count)
{
    int error{0};

    error = decoder.init_format_context(filename, nullptr);
    if(error < 0)
    {
        return error;
    }

    error = decoder.find_stream(media_type);
    if(error < 0)
    {
        return error;
    }

    error = decoder.init_codec_context(nullptr, thread_count);
    return error;
}

int render_yuv_frame(SDL::Texture &texture, SDL_Rect *dst_rect, SDL::Renderer &renderer, AVFrame *frame)
{
    int error{0};

    // update the texture with the provided frame
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
        return error;
    }

    // clear the screen
    error = SDL_RenderClear(renderer);
    if(error < 0)
    {
        return error;
    }

    // copy the data into the renderer
    error = SDL_RenderCopy(renderer, texture, nullptr, dst_rect);
    if(error < 0)
    {
        return error;
    }

    // present the image
    SDL_RenderPresent(renderer);
    return error;
}

int render_frame(SDL::Texture &texture, SDL_Rect *dst_rect, SDL::Renderer &renderer, AVFrame *frame)
{
    int error{0};

    // update the texture with the provided frame
    error = SDL_UpdateTexture(texture,
                              nullptr,
                              frame->data[0],
                              frame->linesize[0]);


    if(error < 0)
    {
        return error;
    }

    // clear the screen
    error = SDL_RenderClear(renderer);
    if(error < 0)
    {
        return error;
    }

    // copy the data into the renderer
    error = SDL_RenderCopy(renderer, texture, nullptr, dst_rect);
    if(error < 0)
    {
        return error;
    }

    // present the image
    SDL_RenderPresent(renderer);
    return error;
}

void decoder_thread_function(FFmpeg::Decoder &decoder,
                             FFmpeg::Scale &rescaler,
                             bool RESCALING_NEEDED,
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

        Utility::error_assert((error == AVERROR(EAGAIN) || error == AVERROR_EOF || error >= 0), "Failed to send packet to decoder", error);

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

        Utility::error_assert((error >= 0), "Failed to receive frame from decoder", error);

        spots_empty.wait();

        if(RESCALING_NEEDED)
        {
            error = sws_scale(rescaler,                                 // Rescaling context to use
                              frame->data,                              // Source data
                              frame->linesize,                          // Source linesize
                              0,                                        // Y position in source image
                              frame->height,                            // height of the source image
                              decoded_frames[current_index]->data,      // destination data
                              decoded_frames[current_index]->linesize); // destination linesize

            Utility::error_assert((error >= 0), "Failed to rescale image", error);

        }

        else
        {
            error = decoded_frames[current_index].copy(frame);
            Utility::error_assert((error >= 0), "Failed to copy frame", error);
        }

        spots_filled.post();

        current_index++;
        if(current_index == decoded_frames.size())
        {
            current_index = 0;
        }

    }
}
