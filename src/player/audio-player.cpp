#include <ffmpeg/decoder.h>
#include <ffmpeg/resample.h>
#include <ffmpeg/frame.h>
#include <portaudio/portaudio.h>
#include <utility/utility.h>

#include <iostream>
#include <string>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>

void interrupt_signal(int signal)
{
    std::cout << "Signal: " << signal << std::endl;
    std::cout << "Interrupted" << std::endl;
    std::exit(1);
}

void terminate_signal(int signal)
{
    std::cout << "Signal: " << signal << std::endl;
    std::cout << "Terminated" << std::endl;
    std::exit(1);
}

void listen_thread_func(std::atomic<bool>*, std::atomic<bool>*, std::size_t*, std::condition_variable*);

void shuffle_vector(std::vector<std::string>&);

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        std::cerr << "Invalid Usage" << std::endl;
        std::cerr << "Valid Usage: " << argv[0] << " <shuffle> <files1> <file2> <file3> ..." << std::endl;
        std::cerr << "To shuffle: --shuffle" << std::endl;;
        return 1;
    }

    std::vector<std::string> files;

    bool shuffling{false};

    for(int i{1}; i != argc; ++i)
    {
        std::string current_file{argv[i]};

        if(current_file == "--shuffle")
        {
            shuffling = true;
        }

        else
        {
            files.push_back(current_file);
        }
    }

    if(shuffling)
    {
        files.shrink_to_fit();
        shuffle_vector(files);
    }

    std::signal(SIGINT, interrupt_signal);
    std::signal(SIGTERM, terminate_signal);

    // initialize portaudio
    PortAudio::Initializer portaudio_init{};

    // print command information
    std::cout << "Commands: " << std::endl;
    std::cout << "exit" << std::endl;
    std::cout << "pause" << std::endl;
    std::cout << "play" << std::endl;
    std::cout << "next" << std::endl;
    std::cout << "prev" << std::endl;

    for(std::size_t i{0}; i != files.size(); ++i)
    {
        FFmpeg::Decoder decoder{};
        FFmpeg::Resample resampler{};

        int error{0};

        // open the file
        error = decoder.init_format_context(files.at(i), nullptr);
        Utility::error_assert((error >= 0), "Failed to open file", error);

        // find a stream
        error = decoder.find_stream(AVMEDIA_TYPE_AUDIO);
        Utility::error_assert((error >= 0), "Failed to find stream", error);

        // start the decoder
        error = decoder.init_codec_context(nullptr, 1);
        Utility::error_assert((error >= 0), "Failed to initialize codec", error);

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

            Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to send packet to decoder", error);
        }

        error = decoder.receive_frame(&decoded_frame);
        Utility::error_assert((error >= 0), "Failed to receive frame from decoder", error);

        error = 0;

        PortAudio::Stream_Playback playback{};

        // use default host api
        error = playback.set_host_api_index(-1);
        Utility::portaudio_error_assert((error >= 0), "Failed to set host api", error);

        // set device index
        error = playback.set_device_index(-1);
        Utility::portaudio_error_assert((error >= 0), "Failed to set device", error);

        enum AVSampleFormat FFMPEG_INPUT_FORMAT{static_cast<enum AVSampleFormat>(decoded_frame->format)};
        enum AVSampleFormat FFMPEG_OUTPUT_FORMAT;
        PaSampleFormat PORTAUDIO_OUTPUT_FORMAT;
        // determine if resampling is needed, and get the PORTAUDIO_OUTPUT_FORMAT
        bool RESAMPLING_NEEDED{ Utility::resampling_needed(FFMPEG_INPUT_FORMAT, FFMPEG_OUTPUT_FORMAT, PORTAUDIO_OUTPUT_FORMAT) };

        // open a playback stream
        error = playback.open_stream(decoded_frame->channels,   // number of output channels
                                     PORTAUDIO_OUTPUT_FORMAT,   // output format
                                     0.05,                      // suggested latency -1.0 for default
                                     decoded_frame->sample_rate,// sample rate
                                     paNoFlag);                 // flags
        Utility::portaudio_error_assert((error >= 0), "Failed to open stream", error);

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
                return 1;
            }

            // initialize the resampler
            error = swr_init(resampler);
            Utility::error_assert((error >= 0), "Failed to initialize resampler", error);
        }

        std::atomic<bool> paused{false};
        std::atomic<bool> skipping{false};
        std::condition_variable cv{};
        std::mutex mutex{};

        // create the listening thread
        std::thread listen_thread{listen_thread_func,
                                  &paused,
                                  &skipping,
                                  &i,
                                  &cv};
        listen_thread.detach();

        std::cout << "Now Playing: " << files.at(i) << std::endl;

        // start the stream
        error = playback.start_stream();
        Utility::portaudio_error_assert((error >= 0), "Failed to start playback stream", error);

        std::cout << "Latency: " << playback.actual_latency() << std::endl;


        // main loop
        while(error != AVERROR(EAGAIN) || !end_of_file_reached)
        {
            if(std::atomic_load<bool>(&paused))
            {
                // stop the playback stream
                error = playback.stop_stream();
                Utility::portaudio_error_assert((error >= 0), "Failed to start playback stream", error);

                if(std::atomic_load<bool>(&skipping))
                {
                    break;
                }

                std::unique_lock<std::mutex> lock{mutex};
                cv.wait(lock);
                lock.unlock();

                if(std::atomic_load<bool>(&skipping))
                {
                    break;
                }

                // start the stream
                error = playback.start_stream();
                Utility::portaudio_error_assert((error >= 0), "Failed to start playback stream", error);
            }

            if(RESAMPLING_NEEDED)
            {
                // set needed values
                resampled_frame->channel_layout = CHANNEL_LAYOUT;
                resampled_frame->sample_rate = SAMPLE_RATE;
                resampled_frame->format = static_cast<int>(FFMPEG_OUTPUT_FORMAT);

                // resample frame
                error = swr_convert_frame(resampler, resampled_frame, decoded_frame);
                Utility::error_assert((error >= 0), "Failed to resample frame", error);

                // play audio
                error = playback.write(resampled_frame->extended_data[0], resampled_frame->nb_samples);
                Utility::portaudio_error_assert((error == -9980 || error >= 0), "Failed to play resampled frame", error); // -9980 == underrun

                av_frame_unref(resampled_frame);
            }

            else
            {
                // play audio
                error = playback.write(decoded_frame->extended_data, decoded_frame->nb_samples);
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
                Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to send packet to decoder", error);
            }

            error = decoder.receive_frame(&decoded_frame);
            Utility::error_assert((error == AVERROR(EAGAIN) || error >= 0), "Failed to receive packet from decoder", error);
        }

    }
    return 0;
}

void listen_thread_func(std::atomic<bool> *paused,    // boolean indicating if playback is paused
                        std::atomic<bool> *skipping,  // boolean indicating if skipping to next track or previous track
                        std::size_t *loop_index,      // current track / loop index
                        std::condition_variable *cv)  // condition variable to wake up playback thread
{

    std::string command{};
    while(1)
    {
        command = "";
        std::getline(std::cin, command);

        if(command == "exit")
        {
            std::exit(1);
        }

        else if(command == "pause" && !std::atomic_load<bool>(paused))
        {
            std::atomic_store<bool>(paused, true);
            std::cout << "Playback Paused" << std::endl;
        }

        else if(command == "play" && std::atomic_load<bool>(paused))
        {
            std::atomic_store<bool>(paused, false);
            cv->notify_one();
            std::cout << "Playback Resumed" << std::endl;
        }

        else if(command == "next")
        {
            if(std::atomic_load<bool>(paused))
            {
                std::atomic_store<bool>(skipping, true);
                cv->notify_one();
            }

            else
            {
                std::atomic_store<bool>(skipping, true);
                std::atomic_store<bool>(paused, true);
            }
        }

        else if(command == "prev")
        {
            if((*loop_index) == 0)
            {
                std::cout << "Already at first track" << std::endl;
            }

            else
            {

                (*loop_index) -= 2;
                if(std::atomic_load<bool>(paused))
                {
                    std::atomic_store<bool>(skipping, true);
                    cv->notify_one();
                }

                else
                {
                    std::atomic_store<bool>(skipping, true);
                    std::atomic_store<bool>(paused, true);
                }
            }

        }

        else
        {
            std::cout << "Unkown command" << std::endl;
        }
    }
}

void shuffle_vector(std::vector<std::string> &files)
{
    static std::mt19937 mt{static_cast<std::size_t>(std::time(nullptr))};

    std::shuffle(files.begin(), files.end(), mt);
}
