#pragma once
// library headers
extern "C"
{
#include <portaudio.h>
}

// C++ standard library headers
#include <string>
#include <queue>

#ifndef RETURN_STATUS
#define RETURN_STATUS
enum Return_Status
{
    STATUS_SUCCESS,
    STATUS_FAILURE,
};
#endif

// Function definitions can be found in playback.cpp
namespace PortAudio
{
    class Playback
    {
        public:
            Playback(PaHostApiIndex, PaDeviceIndex, int, PaSampleFormat, PaTime, void*, int, PaStreamCallback*, PaStreamFinishedCallback*, void*);
            Playback(const Playback&) = delete;
            ~Playback();

            Return_Status init();
            Return_Status start_stream();
            Return_Status stop_stream();
            Return_Status write(const void*, unsigned long);


            void reset_options(PaHostApiIndex, PaDeviceIndex, int, PaSampleFormat, PaTime, void*, int, PaStreamCallback*, PaStreamFinishedCallback*, void*);

            PaHostApiIndex host_api_index() const;
            void set_host_api_index(PaHostApiIndex);

            PaDeviceIndex device_index() const;
            void set_device_index(PaDeviceIndex);

            int channels() const;
            void set_channels(int);

            PaSampleFormat sample_format() const;
            void set_sample_format(PaSampleFormat);

            PaTime suggested_latency() const;
            void set_suggested_latency(PaTime);

            void *host_api_specific_stream_info();
            const void *host_api_specific_stream_info() const;
            void set_host_api_specific_stream_info(void*);

            int sample_rate() const;
            void set_sample_rate(int);

            PaStreamCallback *stream_callback();
            const PaStreamCallback *stream_callback() const;
            void set_stream_callback(PaStreamCallback*);

            PaStreamFinishedCallback *stream_finished_callback();
            const PaStreamFinishedCallback *stream_finished_callback() const;
            void set_stream_finished_callback(PaStreamFinishedCallback*);

            void *callback_user_data();
            const void *callback_user_data() const;
            void set_callback_user_data(void*);

            std::string poll_error();

        private:

            PaHostApiIndex m_host_api_index;
            PaDeviceIndex m_device_index;

            int m_channels;
            PaSampleFormat m_sample_format;
            PaTime m_suggested_latency;
            void *m_host_api_specific_stream_info;
            int m_sample_rate;

            PaStreamCallback *m_stream_callback;
            PaStreamFinishedCallback *m_stream_finished_callback;
            void *m_callback_user_data;

            PaStream *m_stream;

            std::queue<std::string> m_errors;

            void enqueue_error(const std::string&);
            void enqueue_error(PaError);
    };
}

// defined in playback.cpp
void poll_errors(PortAudio::Playback &playback);
