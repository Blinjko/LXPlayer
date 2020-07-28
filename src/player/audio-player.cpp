#include <ffmpeg/decoder.h>
#include <ffmpeg/resample.h>
#include <ffmpeg/frame.h>
#include <portaudio/portaudio.h>
#include <utility/utility.h>

#include <iostream>
#include <string>
#include <csignal>
#include <cstdlib>

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

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        std::cerr << "Invalid Usage" << std::endl;
        std::cout << "Valid Usage: " << argv[0] << " <files1> <file2> <file3> ..." << std::endl;
        return 1;
    }

    std::signal(SIGINT, interrupt_signal);
    std::signal(SIGTERM, terminate_signal);

    for(int i{1}; i != argc; ++i)
    {
        FFmpeg::Decoder decoder{};
        FFmpeg::Resample resampler{};

        int error{0};

        // open the file
        error = decoder.init_format_context(std::string{argv[i]}, nullptr);
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
        // initialize portaudio
        PortAudio::Initializer portaudio_init{};

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

        // start the stream
        error = playback.start_stream();
        Utility::portaudio_error_assert((error >= 0), "Failed to start playback stream", error);


        std::cout << "Latency: " << playback.actual_latency() << std::endl;
        // main loop
        while(error != AVERROR(EAGAIN) || !end_of_file_reached)
        {
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
                Utility::portaudio_error_assert((error >= 0), "Failed to play resampled frame", error);

                av_frame_unref(resampled_frame);
            }

            else
            {
                // play audio
                error = playback.write(decoded_frame->extended_data, decoded_frame->nb_samples);
                Utility::portaudio_error_assert((error >= 0), "Failed to play frame", error);
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
