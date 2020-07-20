#include <ffmpeg/decoder.h>
#include <ffmpeg/resampler.h>
#include <portaudio/init.h>
#include <portaudio/playback.h>

#include <iostream>
#include <cstdlib>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>


// function for the audio thread, defined below main
void audio_thread_func(FFmpeg::Decoder &decoder,
                       bool audio_available,
                       bool video_available,
                       std::mutex &mutex,
                       std::condition_variable &start_cv);


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
    bool video_available{false};

    
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

    // if audio is available start audio playing thread
    std::thread audio_thread{audio_thread_func,
                             std::ref(audio_decoder),
                             audio_available,
                             video_available,
                             std::ref(mutex),
                             std::ref(start_cv)};

    // if video is available start video playing thread

    audio_thread.join();
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

    if(video_available)
    {
        std::unique_lock<std::mutex> lock{mutex};
        start_cv.wait(lock);
        lock.unlock();
    }

    status = playback.start_stream();
    if(status == STATUS_FAILURE)
    {
        poll_errors(playback);
        std::exit(1);
    }

    while(!decoder.end_of_file_reached())
    {
        resampled_frame = resampler.resample_frame(decoded_frame);
        if(!resampled_frame)
        {
            poll_errors(resampler);
            std::exit(1);
        }
    
        playback.write(resampled_frame->extended_data[0], resampled_frame->nb_samples);

        decoded_frame = decoder.decode_frame();
        if(!decoded_frame)
        {
            poll_errors(decoder);
            std::exit(1);
        }
    }
}
