#include <ffmpeg/decoder.h>
#include <ffmpeg/resampler.h>
#include <ffmpeg/color_converter.h>
#include <portaudio/init.h>
#include <portaudio/playback.h>
#include <sdl/init.h>
#include <sdl/window.h>
#include <sdl/renderer.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>


// function for the audio thread, defined below main
void audio_thread_func(FFmpeg::Decoder &decoder,
                       bool audio_available,
                       bool video_available,
                       PaTime &output_latency,
                       std::mutex &mutex,
                       std::condition_variable &start_cv);

// function for the video thread, defined below main
void video_thread_func(FFmpeg::Decoder &decoder,
                       bool audio_available,
                       bool video_available,
                       PaTime &output_latency,
                       std::mutex &mutex,
                       std::condition_variable &start_cv);

// function that decodes and resamples video
void video_decode_thread_func(FFmpeg::Decoder &decoder,
                              AVFrame *decoded_frame,
                              std::size_t buffer_size,
                              std::queue<FFmpeg::Color_Data> &frame_queue,
                              std::mutex &queue_mutex,
                              std::condition_variable &queue_cv);

// function for a thread that listens for input
void listen_thread_func();

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cerr << "Invalid Usage\n";
        std::cerr << "Valid Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    // boolean variables dictating wheather audio & video streams are available
    bool audio_available{true};
    bool video_available{true};

    
    // Create audio and video decoders
    FFmpeg::Decoder audio_decoder{argv[1], AVMEDIA_TYPE_AUDIO};
    FFmpeg::Decoder video_decoder{argv[1], AVMEDIA_TYPE_VIDEO};

    std::mutex mutex;
    std::condition_variable start_cv;
    Return_Status status;

    // try to open file for audio
    status = audio_decoder.open_file();
    if(status == STATUS_FAILURE)
    {
        poll_errors(audio_decoder);
        audio_available = false;
    }

    // try to open file for video
    status = video_decoder.open_file();
    if(status == STATUS_FAILURE)
    {
        poll_errors(video_decoder);
        video_available = false;
    }

    // there is no audio or video avaiable so exit
    if(!audio_available && !video_available)
    {
        return 1;
    }

    // variable used to hold audio latency
    PaTime output_latency {0.0};

    // if audio is available start audio playing thread
    std::thread audio_thread{audio_thread_func,
                             std::ref(audio_decoder),
                             audio_available,
                             video_available,
                             std::ref(output_latency),
                             std::ref(mutex),
                             std::ref(start_cv)};

    // if video is available start video playing thread
    std::thread video_thread{video_thread_func,
                             std::ref(video_decoder),
                             audio_available,
                             video_available,
                             std::ref(output_latency),
                             std::ref(mutex),
                             std::ref(start_cv)};

    std::thread listen_thread{listen_thread_func};

    listen_thread.detach();

    audio_thread.join();
    video_thread.join();
    return 0;
}




/* Function to decode, resample and play audio
 * @desc - this is the function that will be given to the audio playing thread
 * @param decoder - the FFmpeg::Decoder to use
 * @param video_available - a boolean indicating wheather video is available or not
 * @param mutex - a mutex for the contional variable(s)
 * @param start_cv - a conditional variable which will coordinate starting audio 
 * playback and video playback at the same time.
 */
void audio_thread_func(FFmpeg::Decoder &decoder,
                       bool audio_available,
                       bool video_available,
                       PaTime &output_latency,
                       std::mutex &mutex,
                       std::condition_variable &start_cv)
{
    if(!audio_available)
    {
        return;
    }

    Return_Status status{STATUS_SUCCESS};
    AVFrame *decoded_frame{nullptr};

    // initialize the decoder
    status = decoder.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(decoder);
        std::exit(1);
    }

    // decode a frame, needed for other initializations
    decoded_frame = decoder.decode_frame();
    if(!decoded_frame)
    {
        poll_errors(decoder);
        std::exit(1);
    }

    int CHANNELS{decoded_frame->channels};
    int64_t CHANNEL_LAYOUT{static_cast<int64_t>(decoded_frame->channel_layout)};
    int SAMPLE_RATE{decoded_frame->sample_rate};
    enum AVSampleFormat FFMPEG_SAMPLE_FORMAT{AV_SAMPLE_FMT_FLT};

    // Make the audio resampler
    FFmpeg::Frame_Resampler resampler{CHANNEL_LAYOUT,
                                      FFMPEG_SAMPLE_FORMAT,
                                      SAMPLE_RATE,
                                      static_cast<int64_t>(decoded_frame->channel_layout),
                                      static_cast<enum AVSampleFormat>(decoded_frame->format),
                                      decoded_frame->sample_rate};
    AVFrame *resampled_frame{nullptr};

    status = resampler.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(resampler);
        std::exit(1);
    }

    // Initialize PortAudio
    PortAudio::Initializer portaudio_init{};

    PaHostApiIndex DEFAULT_HOST_API{Pa_GetDefaultHostApi()};
    PaDeviceIndex DEFAULT_DEVICE{Pa_GetDefaultOutputDevice()};
    PaSampleFormat PA_SAMPLE_FORMAT{paFloat32};

    PortAudio::Playback playback{DEFAULT_HOST_API,
                                 DEFAULT_DEVICE,
                                 CHANNELS,
                                 PA_SAMPLE_FORMAT,
                                 -1.0,
                                  nullptr,
                                  SAMPLE_RATE,
                                  nullptr,
                                  nullptr,
                                  nullptr};

    status = playback.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(playback);
        std::exit(1);
    }

    resampled_frame = resampler.resample_frame(decoded_frame);
    if(!resampled_frame)
    {
        poll_errors(resampler);
        std::exit(1);
    }
    
    status = playback.start_stream();
    if(status == STATUS_FAILURE)
    {
        poll_errors(playback);
        std::exit(1);
    }

    output_latency = playback.output_latency();
    if(output_latency == -1.0)
    {
        poll_errors(playback);
        std::exit(1);
    }

    std::cout << "Output Latency: " << output_latency << std::endl;

    if(video_available)
    {
        std::unique_lock<std::mutex> lock{mutex};
        start_cv.wait(lock);
        lock.unlock();
    }

    while(!decoder.end_of_file_reached())
    {
        playback.write(resampled_frame->extended_data[0], resampled_frame->nb_samples);

        decoded_frame = decoder.decode_frame();
        if(!decoded_frame)
        {
            poll_errors(decoder);
            std::exit(1);
        }

        resampled_frame = resampler.resample_frame(decoded_frame);
        if(!resampled_frame)
        {
            poll_errors(resampler);
            std::exit(1);
        }
    }

}




/* Video thread function
 * @desc - this is the function that the video thread executes
 * @param decoder - the Decoder to use
 * @param audio_available - a boolean indicating if audio is available
 * @param video_available - a boolean indicating if video is available
 * @param mutex - a mutex referenced used for start_cv
 * @param start_cv - a cv to notify the audio thread when to start
 */
void video_thread_func(FFmpeg::Decoder &decoder,
                       bool audio_available,
                       bool video_available,
                       PaTime &output_latency,
                       std::mutex &mutex,
                       std::condition_variable &start_cv)
{
    if(!video_available)
    {
        return;
    }

    Return_Status status{STATUS_SUCCESS};

    // initialize the decoder
    status = decoder.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(decoder);
        std::exit(1);
    }

    // decode a frame
    AVFrame *decoded_frame = decoder.decode_frame();
    if(!decoded_frame)
    {
        poll_errors(decoder);
        std::exit(1);
    }

    int TEXTURE_WIDTH = decoded_frame->width;
    int TEXTURE_HEIGHT = decoded_frame->height;

    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::queue<FFmpeg::Color_Data> frame_queue;

    // get the stream that will be decoded
    // It is needed for some timing information
    const AVStream *stream{decoder.format_context()->streams[decoder.stream_number()]};

    int timescale{stream->time_base.den};

    // calculate timebase as a decimal
    double timebase{static_cast<double>(stream->time_base.num) / stream->time_base.den};

    // calculate framerate as a decimal
    double framerate{static_cast<double>(stream->r_frame_rate.num) / stream->r_frame_rate.den};

    // calcuate how much pts will increase per frame
    double pts_increase_amount{timescale / framerate};

    // calculate how much time will increase per frame
    double time_increase_amount{pts_increase_amount * timebase};

    std::size_t buffer_size{static_cast<std::size_t>(2*framerate)};

    // start decoding thread
    std::thread video_decoding_thread{video_decode_thread_func,
                                      std::ref(decoder),
                                      decoded_frame,
                                      buffer_size,
                                      std::ref(frame_queue),
                                      std::ref(queue_mutex),
                                      std::ref(queue_cv)};


    // initialize SDL
    SDL::Initializer sdl_init{SDL_INIT_VIDEO};

    // create an SDL window
    int WINDOW_WIDTH{1920};
    int WINDOW_HEIGHT{1080};
    uint32_t WINDOW_FLAGS{SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN};
    SDL::Window window{"LXPlayer",
                       SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED,
                       WINDOW_WIDTH,
                       WINDOW_HEIGHT,
                       WINDOW_FLAGS};

    // initialize the window
    status = window.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(window);
        std::exit(1);
    }

    uint32_t RENDERER_FLAGS{SDL_RENDERER_PRESENTVSYNC};
    uint32_t TEXTURE_PIXEL_FORMAT{SDL_PIXELFORMAT_RGB24};
    SDL::Renderer renderer{window,
                           -1,
                           RENDERER_FLAGS,
                           TEXTURE_PIXEL_FORMAT,
                           SDL_TEXTUREACCESS_STREAMING,
                           TEXTURE_WIDTH,
                           TEXTURE_HEIGHT};

    // initialize the renderer
    status = renderer.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(renderer);
        std::exit(1);
    }

    // wait for the queue to fill
    if(frame_queue.size() < buffer_size)
    {
        std::unique_lock<std::mutex> lock{queue_mutex};
        queue_cv.wait(lock);
        lock.unlock();
    }

    // notify audio thread that it is time to start
    if(audio_available)
    {
        start_cv.notify_one();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(output_latency*1000)));
    // create a time point, will always hold the time the last frame was played
    auto last_time{std::chrono::steady_clock::now()};

    // rendering loop
    while(1)
    {
        // lock mutex to prevent data race with decoding queue
        queue_mutex.lock();
        if(decoder.end_of_file_reached() && frame_queue.empty())
        {
            queue_mutex.unlock();
            break;
        }

        queue_mutex.unlock();

        // if the queue is empty tell the decoder to wake up, then wait until it finishes
        if(frame_queue.empty())
        {
            queue_cv.notify_one();

            std::unique_lock<std::mutex> lock{queue_mutex};
            queue_cv.wait(lock);
            lock.unlock();
        }

        // if the queue is not full tell the decoder to wake up and do some work
        else if(frame_queue.size() < buffer_size)
        {
            queue_cv.notify_one();
        }

        // get a frame from the frame_queue
        FFmpeg::Color_Data pixel_data{frame_queue.front()};
        frame_queue.pop();

        // calcuate the timing
        auto current_time{std::chrono::steady_clock::now()};
        std::chrono::duration<double> difference{current_time - last_time};

        // wait until the proper amount of time has elapsed, this keeps framerate in check
        while(difference.count() < time_increase_amount)
        {
            current_time = std::chrono::steady_clock::now();
            difference = current_time - last_time;
        }

        last_time = current_time;

        // clear the screen
        status = renderer.clear();
        if(status == STATUS_FAILURE)
        {
            poll_errors(renderer);
            std::exit(1);
        }

        // render the pixel data
        status = renderer.render(nullptr, pixel_data.data[0], pixel_data.linesize[0], nullptr, nullptr);
        if(status == STATUS_FAILURE)
        {
            poll_errors(renderer);
            std::exit(1);
        }

        // present the data
        status = renderer.present();
        if(status == STATUS_FAILURE)
        {
            poll_errors(renderer);
            std::exit(1);
        }

        // free the data
        free_Color_Data(pixel_data);
        
        // repeat
    }

    video_decoding_thread.join();
}

// function that decodes and resamples video
void video_decode_thread_func(FFmpeg::Decoder &decoder,
                              AVFrame *decoded_frame,
                              std::size_t buffer_size,
                              std::queue<FFmpeg::Color_Data> &frame_queue,
                              std::mutex &queue_mutex,
                              std::condition_variable &queue_cv)
{
    enum AVPixelFormat FFMPEG_PIXEL_FORMAT{AV_PIX_FMT_RGB24};
    int IMAGE_WIDTH{decoded_frame->width};
    int IMAGE_HEIGHT{decoded_frame->height};

    // create a Color_Converter
    FFmpeg::Color_Converter color_converter{IMAGE_WIDTH,
                                            IMAGE_HEIGHT,
                                            static_cast<enum AVPixelFormat>(decoded_frame->format),
                                            IMAGE_WIDTH,
                                            IMAGE_HEIGHT,
                                            FFMPEG_PIXEL_FORMAT};

    Return_Status status{STATUS_SUCCESS};

    // initialize the color converter
    status = color_converter.init();
    if(status == STATUS_FAILURE)
    {
        poll_errors(color_converter);
        std::exit(1);
    }

    while(!decoder.end_of_file_reached())
    {
        // the queue is full so notify anyone waiting, and sleep
        if(frame_queue.size() >= buffer_size)
        {
            queue_cv.notify_one();

            std::unique_lock<std::mutex> lock{queue_mutex};
            queue_cv.wait(lock);
            lock.unlock();
        }

        // rescale a frame
        FFmpeg::Color_Data &converted_data{color_converter.convert(decoded_frame->data, decoded_frame->linesize)};

        // check for failure
        if(converted_data.linesize[0] == -1)
        {
            poll_errors(color_converter);
            std::exit(1);
        }

        // copy of the convertered data
        FFmpeg::Color_Data data_copy;

        // allocate space for the data to be copied
        status = alloc_Color_Data(data_copy, IMAGE_WIDTH, IMAGE_HEIGHT, FFMPEG_PIXEL_FORMAT);
        if(status == STATUS_FAILURE)
        {
            std::exit(1);
        }

        // copy the data
        copy_Color_Data(converted_data, data_copy, IMAGE_WIDTH, IMAGE_HEIGHT, FFMPEG_PIXEL_FORMAT);

        // put the copied data onto the frame queue
        frame_queue.push(data_copy);

        // decode more frames, must use mutex to avoid data race
        queue_mutex.lock();
        decoded_frame = decoder.decode_frame();
        queue_mutex.unlock();

        if(!decoded_frame)
        {
            poll_errors(decoder);
            std::exit(1);
        }
    }
}




void listen_thread_func()
{
    std::string input{};

    while(1)
    {
        input = "";
        std::cout << "Type a command: ";
        std::cin >> input;

        if(input == "stop")
        {
            std::exit(0);
        }

    }
}
