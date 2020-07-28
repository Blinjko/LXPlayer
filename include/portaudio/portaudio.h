#pragma once

extern "C"
{
#include <portaudio.h>
}

namespace PortAudio
{
    // Initializes portaudio on construction and terminate portaudio on destruction
    struct Initializer
    {
        Initializer();
        ~Initializer();
    };

    // Class to handle audio playback
    class Stream_Playback
    {
        public:
            Stream_Playback();
            Stream_Playback(const Stream_Playback&) = delete;

            ~Stream_Playback();

            PaError set_host_api_index(PaHostApiIndex);
            PaError set_device_index(PaDeviceIndex);

            PaError open_stream(int, PaSampleFormat, PaTime, int, PaStreamFlags);

            PaError start_stream();
            PaError stop_stream();

            PaError write(const void*, unsigned long);
            PaError reset();

            PaTime actual_latency();

            bool steam_stopped() const;
            int channel_count() const;
            PaSampleFormat sample_format() const;
            PaTime suggested_latency() const;
            int sample_rate() const;

            PaStream *stream();
            const PaStream *stream() const;

        private:
            PaHostApiIndex m_host_api;
            PaDeviceIndex m_device;

            PaStream *m_stream;

            int m_channel_count;
            PaSampleFormat m_sample_format;
            PaTime m_suggested_latency;
            int m_sample_rate;

            bool m_stream_stopped;
    };
}
