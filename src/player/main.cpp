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
#include <atomic>

#include <ffmpeg/decoder.h>
#include <ffmpeg/frame.h>
#include <ffmpeg/scale.h>
#include <ffmpeg/resample.h>
#include <portaudio/portaudio.h>
#include <sdl/sdl.h>
#include <utility/semaphore.h>
#include <utility/utility.h>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

// Shared_Varaibles struct, holds variables that are shared between threads
// Holds only data variables, no synchronization variables
struct Shared_Variables
{
    bool audio_playback;
    bool video_playback;

    bool video_waiting;
    bool audio_waiting;

    // audio latency in seconds
    double audio_latency;

    std::atomic<bool> paused;
    std::atomic<bool> audio_paused;

    SDL::Window window;
    bool fullscreen;
};

// video stuff
int render_yuv_frame(SDL::Texture&, SDL_Rect*, SDL::Renderer&, AVFrame*);
int render_frame(SDL::Texture&, SDL_Rect*, SDL::Renderer&, AVFrame*);
void decoder_thread_function(FFmpeg::Decoder&, FFmpeg::Scale&, bool, FFmpeg::Frame_Array&, Utility::Semaphore&, Utility::Semaphore&);

void video_playback(FFmpeg::Decoder&, Shared_Variables&, std::condition_variable&, std::condition_variable&, std::mutex&);


void audio_thread_func(FFmpeg::Decoder&, Shared_Variables&, std::condition_variable&, std::condition_variable&, std::mutex&);

void terminal_listen_thread_func(Shared_Variables&, std::condition_variable&);

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

    // setup decoder and shared variables
    FFmpeg::Decoder video_decoder{};
    FFmpeg::Decoder audio_decoder{};
    Shared_Variables shared_vars{};

    shared_vars.audio_playback = true;
    shared_vars.video_playback = true;

    int error{0};

    // open the file for both decoders
    error = video_decoder.init_format_context(filename, nullptr);
    Utility::error_assert((error >= 0), "Failed to open file, video decoder", error);

    error = audio_decoder.init_format_context(filename, nullptr);
    Utility::error_assert((error >= 0), "Failed to open file, audio decoder", error);

    // try to find video stream
    error = video_decoder.find_stream(AVMEDIA_TYPE_VIDEO);
    if(error == AVERROR_STREAM_NOT_FOUND)
    {
        shared_vars.video_playback = false;
        error = 0;
    }
    Utility::error_assert((error >= 0), "Failed to find video stream", error);

    // try to find audio stream
    error = audio_decoder.find_stream(AVMEDIA_TYPE_AUDIO);
    if(error == AVERROR_STREAM_NOT_FOUND)
    {
        shared_vars.audio_playback = false;
        error = 0;
    }
    Utility::error_assert((error >= 0), "Failed to find audio stream", error);

    shared_vars.video_waiting = false;
    shared_vars.audio_waiting = false;

    std::atomic_store<bool>(&shared_vars.paused, false);
    std::atomic_store<bool>(&shared_vars.audio_paused, false);

    std::condition_variable start_cv;
    std::condition_variable paused_cv;
    std::mutex mutex;

    // start audio thread, will automatically exit if there is no audio playback
    std::thread audio_thread{audio_thread_func,
                             std::ref(audio_decoder),
                             std::ref(shared_vars),
                             std::ref(start_cv),
                             std::ref(paused_cv),
                             std::ref(mutex)};

    // start a thread to listen for input
    std::thread listen_thread{terminal_listen_thread_func,
                              std::ref(shared_vars),
                              std::ref(paused_cv)};
    listen_thread.detach();

    // the main thread decodes video, will also automatically return if there is no video playback
    video_playback(video_decoder, shared_vars, start_cv, paused_cv, mutex);

    audio_thread.join();
    return 0;
}

void video_playback(FFmpeg::Decoder &decoder, Shared_Variables &shared_vars, std::condition_variable &start_cv, std::condition_variable &paused_cv, std::mutex &mutex)
{
    // check for video playback
    if(!shared_vars.video_playback)
    {
        return;
    }

    int error{0};

    int thread_count{4};

    error = decoder.init_codec_context(nullptr, thread_count);
    Utility::error_assert((error >= 0), "Failed to initialize FFmpeg Decoder", error);

    // Fill decoder up with data
    while(error != AVERROR(EAGAIN))
    {
        error = decoder.send_packet();
        if(error == AVERROR_EOF)
        {
            shared_vars.video_playback = false;
            shared_vars.video_waiting = true;
            return;
        }

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

    // check if rescaling is needed for pixel formats, and get SDL output format
    bool RESCALING_NEEDED { Utility::rescaling_needed(FFMPEG_INPUT_FORMAT, FFMPEG_OUTPUT_FORMAT, SDL_TEXTURE_FORMAT) };

    // if rescaling is needed, but the FFMPEG_INPUT_FORMAT is not supported then the video cannot be played
    if(RESCALING_NEEDED && !Utility::valid_rescaling_input(FFMPEG_INPUT_FORMAT))
    {
        std::cerr << "Cannot play video unsupported pixel format" << std::endl;
        return;
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

    std::string window_title{"LXPlayer"};
    SDL::Renderer renderer{};
    SDL::Texture texture{};
    SDL_Rect display_rect;

    // screen resolution
    SDL_Rect screen_resolution{Utility::get_native_resolution()};
    Utility::error_assert((screen_resolution.w > 0), "Failed to get native screen resolution");

    // image resolution 
    SDL_Rect image_resolution;
    image_resolution.w = decoded_frame->width;
    image_resolution.h = decoded_frame->height;

    // if the image is too big for the screen, downsize it
    if(image_resolution.w > screen_resolution.w || image_resolution.h > screen_resolution.h)
    {
        Utility::downsize_resolution(image_resolution, screen_resolution);
        RESCALING_NEEDED = true;
    }

    // create the window
    shared_vars.window = SDL_CreateWindow(window_title.c_str(),   // Window Title
                                          SDL_WINDOWPOS_CENTERED, // X Position
                                          SDL_WINDOWPOS_CENTERED, // Y Position
                                          screen_resolution.w,    // Width
                                          screen_resolution.h,    // Height
                                          SDL_WINDOW_RESIZABLE);  // Flags
    Utility::error_assert(shared_vars.window, "Failed to create window");
    shared_vars.fullscreen = false;

    // create the renderer
    renderer = SDL_CreateRenderer(shared_vars.window, // Window to use
                                  -1,     // Driver Index -1 for auto selection
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // flags
    Utility::error_assert(renderer, "Failed to create renderer");

    // create a texture
    texture = SDL_CreateTexture(renderer,                 // Renderer to use
                                SDL_TEXTURE_FORMAT,       // Pixel format
                                SDL_TEXTUREACCESS_STATIC, // Texture access
                                image_resolution.w,       // Texture width
                                image_resolution.h);      // Texture height
    Utility::error_assert(texture, "Failed to create texture");


    // calculate the display rectangle
    display_rect = Utility::calculate_display_rectangle(image_resolution, screen_resolution);

    // SDL Setup End //

    // Setup image rescaler if needed //
    if(RESCALING_NEEDED)
    {
        rescaler = sws_getContext(decoded_frame->width,   // source width
                                  decoded_frame->height,  // source height
                                  FFMPEG_INPUT_FORMAT,    // source pixel format
                                  image_resolution.w,     // destination width
                                  image_resolution.h,     // destination height
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

    // Inital Frame Setup //
    FFmpeg::Frame initial_frame{};
    error = initial_frame.allocate(FFMPEG_OUTPUT_FORMAT, 
                                   image_resolution.w,
                                   image_resolution.h);

    Utility::error_assert((error >= 0), "Failed to allocate initial frame", error);

    // rescale the first image if needed, has to be done before decoding loop is started
    if(RESCALING_NEEDED)
    {
        error = sws_scale(rescaler,                       // Rescaling context to use
                decoded_frame->data,                      // Source data
                decoded_frame->linesize,                  // Source linesize
                0,                                        // Y position in source image
                decoded_frame->height,                    // height of the source image
                initial_frame->data,                      // destination data
                initial_frame->linesize);                 // destination linesize

        Utility::error_assert((error >= 0), "Failed to rescale initial image", error);
    }

    else
    {
        initial_frame.copy(decoded_frame);
    }

    // make an array for the decoded video frames
    FFmpeg::Frame_Array decoded_frames{buffer_size};

    // allocate every frame in the array
    // This helps with performance by reducing the allocate / deallocate system calls
    // The frames are allocated once and deallocated once
    for(int i{0}; i != decoded_frames.size(); ++i)
    {
        error = decoded_frames[i].allocate(FFMPEG_OUTPUT_FORMAT,
                                           image_resolution.w,
                                           image_resolution.h);
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
    

    while(spots_filled.count() != buffer_size)
    {
    }

    // make sure audio thread and video thread, this thread, start at the same time
    std::unique_lock<std::mutex> lock{mutex};

    if(shared_vars.audio_playback)
    {
        if(shared_vars.audio_waiting)
        {
            lock.unlock();
            start_cv.notify_one();
        }

        else
        {
            shared_vars.video_waiting = true;
            start_cv.wait(lock);
            shared_vars.video_waiting = false;
            lock.unlock();
        }
    }
    else
    {
        lock.unlock();
    }

    // sleep for however long the audio latency is, this will negate the latency
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(shared_vars.audio_latency * 1000)));

    // render the first image
    if(YUV_IMAGE_OUTPUT)
    {
        error = render_yuv_frame(texture, &display_rect, renderer, initial_frame);
        Utility::error_assert((error >= 0), "Failed to render initial YUV frame");
    }

    else
    {
        error = render_frame(texture, &display_rect, renderer, initial_frame);
        Utility::error_assert((error >= 0), "Failed to render initial frame");
    }

    int frames{0};
    int current_index{0};
    double wait_time{0.0}; // time spend waiting / paused

    auto start_time{std::chrono::steady_clock::now()};
    auto last_frame_time{start_time};

    while(1)
    {
        // check if paused
        if(std::atomic_load<bool>(&shared_vars.audio_paused))
        {
            auto pause_start{std::chrono::steady_clock::now()};
            // if paused wait until awoken
            lock.lock();
            paused_cv.wait(lock);
            lock.unlock();
            auto pause_end{std::chrono::steady_clock::now()};
            std::chrono::duration<double> difference{pause_end - pause_start};

            wait_time += difference.count();
        }

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

        while((difference.count() - wait_time) < frame_display_time)
        {
            current_time = std::chrono::steady_clock::now();
            difference = current_time - start_time;
        }

        if(YUV_IMAGE_OUTPUT)
        {
            error = render_yuv_frame(texture, &display_rect, renderer, decoded_frames[current_index]);
            Utility::error_assert((error >= 0), "Failed to render YUV frame");
        }
        else 
        {
            error = render_frame(texture, &display_rect, renderer, decoded_frames[current_index]);
            Utility::error_assert((error >= 0), "Failed to render frame");
        }

        spots_empty.post();

        frames++;
        current_index++;

        /*
        current_time = std::chrono::steady_clock::now();
        difference = current_time - last_frame_time;

        if(difference.count() >= 1.0)
        {
            std::cout << "Frames: " << frames << std::endl;
            frames = 0;

            last_frame_time = current_time;
        }
        */
    }

    decoder_thread.join();
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

            decoded_frames[current_index]->pts = frame->pts;

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


void audio_thread_func(FFmpeg::Decoder &decoder, Shared_Variables &shared_vars, std::condition_variable &start_cv, std::condition_variable &paused_cv, std::mutex &mutex)
{
    // check if audio is being played back
    if(!shared_vars.audio_playback)
    {
        return;
    }

    // set audio latency
    shared_vars.audio_latency = 0.05;

    // initialize portaudio
    PortAudio::Initializer portadio_init{};

    FFmpeg::Resample resampler{};

    int error{0};

    // start the decoder
    error = decoder.init_codec_context(nullptr, 1);
    Utility::error_assert((error >= 0), "Failed to initialize audio decoder codec", error);

    bool end_of_file_reached{false};

    // fill the decoder
    AVFrame *decoded_frame{nullptr};
    FFmpeg::Frame resampled_frame{};
    while(error != AVERROR(EAGAIN))
    {
        error = decoder.send_packet();
        if(error == AVERROR_EOF)
        {
            end_of_file_reached = true;
        }

        Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to send packet to audio decoder", error);
    }

    error = decoder.receive_frame(&decoded_frame);
    Utility::error_assert((error >= 0), "Failed to receive frame from audio decoder", error);

    error = 0;

    PortAudio::Stream_Playback playback{};

    // use default host api
    error = playback.set_host_api_index(-1);
    Utility::portaudio_error_assert((error >= 0), "Failed to set audio host api", error);

    // set device index
    error = playback.set_device_index(-1);
    Utility::portaudio_error_assert((error >= 0), "Failed to set audio device", error);

    enum AVSampleFormat FFMPEG_INPUT_FORMAT{static_cast<enum AVSampleFormat>(decoded_frame->format)};
    enum AVSampleFormat FFMPEG_OUTPUT_FORMAT;
    PaSampleFormat PORTAUDIO_OUTPUT_FORMAT;
    bool SAMPLES_INTERLEAVED{false};

    // determine if resampling is needed, and get the PORTAUDIO_OUTPUT_FORMAT
    bool RESAMPLING_NEEDED{ Utility::resampling_needed(FFMPEG_INPUT_FORMAT, FFMPEG_OUTPUT_FORMAT, PORTAUDIO_OUTPUT_FORMAT, SAMPLES_INTERLEAVED) };

    // open a playback stream
    error = playback.open_stream(decoded_frame->channels,   // number of output channels
            PORTAUDIO_OUTPUT_FORMAT,   // output format
            shared_vars.audio_latency, // suggested latency -1.0 for default
            decoded_frame->sample_rate,// sample rate
            paNoFlag);                 // flags
    Utility::portaudio_error_assert((error >= 0), "Failed to open portaudio stream", error);

    // varialbes to hold channel layout and sample rate, for resampling, involves less overall calculations
    int64_t CHANNEL_LAYOUT{av_get_default_channel_layout(decoded_frame->channels)};
    int SAMPLE_RATE{decoded_frame->sample_rate};

    // setup resampler if appropriate
    if(RESAMPLING_NEEDED)
    {
        resampled_frame = av_frame_alloc();
        if(!resampled_frame)
        {
            std::cerr << "Failed to allocate resampled frame" << std::endl;
        }

        // create the resampler
        resampler = swr_alloc_set_opts(resampler,                      // resampling context
                CHANNEL_LAYOUT,                 // out channel layout
                FFMPEG_OUTPUT_FORMAT,           // out sample format
                SAMPLE_RATE,                    // out sample rate
                CHANNEL_LAYOUT,                 // in channel layout
                FFMPEG_INPUT_FORMAT,            // in sample format
                SAMPLE_RATE,                    // in sample rate
                0,                              // log offset
                nullptr);                       // log context
        if(!resampler)
        {
            std::cerr << "Failed to setup resampler" << std::endl;    
            return;
        }

        // initialize the resampler
        error = swr_init(resampler);
        Utility::error_assert((error >= 0), "Failed to initialize resampler", error);
    }

    // the following code makes sure the audio and video thread start at the same time
    std::unique_lock<std::mutex> lock{mutex};
    
    if(shared_vars.video_playback)
    {
        if(shared_vars.video_waiting)
        {
            lock.unlock();
            start_cv.notify_one();
        }
        else
        {
            shared_vars.audio_waiting = true;
            start_cv.wait(lock);
            shared_vars.audio_waiting = false;
            lock.unlock();
        }
    }

    else
    {
        lock.unlock();
    }

    // start the stream
    error = playback.start_stream();
    Utility::portaudio_error_assert((error >= 0), "Failed to start playback stream", error);


    // main loop
    while(error != AVERROR(EAGAIN) || !end_of_file_reached)
    {
        // check if paused
        if(std::atomic_load<bool>(&shared_vars.paused))
        {
            // stop playback stream
            error = playback.stop_stream();
            Utility::portaudio_error_assert((error >= 0), "Failed to pause playback stream", error);

            // set audio_paused to true
            std::atomic_store<bool>(&shared_vars.audio_paused, true);

            // wait
            lock.lock();
            paused_cv.wait(lock);
            lock.unlock();

            // when awoken start stream again
            error = playback.start_stream();
            Utility::portaudio_error_assert((error >= 0), "Failed to resume playback stream", error);
        }

        if(RESAMPLING_NEEDED)
        {
            // set needed values
            resampled_frame->channel_layout = CHANNEL_LAYOUT;
            resampled_frame->sample_rate = SAMPLE_RATE;
            resampled_frame->format = static_cast<int>(FFMPEG_OUTPUT_FORMAT);

            // a odd thing seems to happen with .wav files where the channel layout is 0
            // so we have to calculate and set it otherwise we get errors when resampling
            decoded_frame->channel_layout = CHANNEL_LAYOUT;

            // resample frame
            error = swr_convert_frame(resampler, resampled_frame, decoded_frame);
            Utility::error_assert((error >= 0), "Failed to resample frame", error);

            // play audio
            if(SAMPLES_INTERLEAVED)
            {
                error = playback.write(resampled_frame->extended_data[0], resampled_frame->nb_samples);
            }

            else
            {
                error = playback.write(resampled_frame->extended_data, resampled_frame->nb_samples);
            }

            Utility::portaudio_error_assert((error == -9980 || error >= 0), "Failed to play resampled frame", error); // -9980 == underrun

            av_frame_unref(resampled_frame);
        }

        else
        {
            // play audio
            if(SAMPLES_INTERLEAVED) 
            { 
                error = playback.write(decoded_frame->extended_data[0], decoded_frame->nb_samples); 
            }

            else 
            { 
                error = playback.write(decoded_frame->extended_data, decoded_frame->nb_samples); 
            }

            Utility::portaudio_error_assert((error == -9980 || error >= 0), "Failed to play frame", error); // -9980 == underrun
        }

        error = 0;
        while(!end_of_file_reached && error != AVERROR(EAGAIN))
        {
            error = decoder.send_packet();
            if(error == AVERROR_EOF)
            {
                end_of_file_reached = true;
                break;
            }
            Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to send packet to audio decoder", error);
        }

        error = decoder.receive_frame(&decoded_frame);
        Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to receive packet from audio decoder", error);
    }

}

void terminal_listen_thread_func(Shared_Variables &shared_vars, std::condition_variable &paused_cv)
{
    std::cout << "Commands: " << std::endl;
    std::cout << "pause" << std::endl;
    std::cout << "play" << std::endl;
    std::cout << "exit" << std::endl;

    std::string input{};

    while(1)
    {
        input = "";

        std::cout << "Type a command: ";
        std::getline(std::cin, input);

        if(input == "pause" && !std::atomic_load<bool>(&shared_vars.paused))
        {
            std::cout << "Playback paused" << std::endl;
            std::atomic_store<bool>(&shared_vars.paused, true);
        }

        else if(input == "play" && std::atomic_load<bool>(&shared_vars.paused))
        {
            std::cout << "Resuming playback" << std::endl;
            std::atomic_store<bool>(&shared_vars.paused, false);
            std::atomic_store<bool>(&shared_vars.audio_paused, false);

            paused_cv.notify_all();
        }

        else if(input == "exit")
        {
            std::exit(0);
        }

        else if(input == "fullscreen")
        {
            if(shared_vars.fullscreen)
            {
                int error{0};
                error = SDL_SetWindowFullscreen(shared_vars.window, 0);
                Utility::error_assert((error >= 0), "Failed to fullscreen window");
                shared_vars.fullscreen = false;
            }
            else
            {
                int error{0};
                error = SDL_SetWindowFullscreen(shared_vars.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                Utility::error_assert((error >= 0), "Failed to fullscreen window");
                shared_vars.fullscreen = true;
            }
        }

        else
        {
            std::cout << "Unkown command" << std::endl;
        }
    }
}
