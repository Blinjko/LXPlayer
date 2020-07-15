#pragma once

// FFmpeg library headers
extern "C"
{
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
}

// C++ Standard Library headers
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

namespace FFmpeg
{
    /* Frame_Resampler Class, resamples AVFrames into a given format
     * @note This Class only works with audio
     * @member m_swr_ctx, struct SwrContext* that is used for libswresample resampling functions
     * @member m_frame, AVFrame* used to hold resampled audio AVFrame
     * @member m_out_channel_layout, the output channel layout
     * @member m_out_sample_format, the output sample format EX: 16 bit native endian
     * @member m_sample_rate, the output sample rate EX: 48000 Hz
     * @member m_in_channel_layout, the input channel layout
     * @member m_in_sample_format, the input sample format
     * @member m_in_sample_rate the input sample rate
     * @member m_errors, a std::queue<std::string> that holds error messages
     */
    class Frame_Resampler
    {
        public:

            Frame_Resampler(int64_t, enum AVSampleFormat, int, int64_t, enum AVSampleFormat, int);
            ~Frame_Resampler();

            Return_Status init();
            AVFrame *resample_frame(AVFrame*);
            
            void reset_options(int64_t, enum AVSampleFormat, int, int64_t, enum AVSampleFormat, int);


            int64_t &out_channel_layout();
            const int64_t &out_channel_layout() const;

            enum AVSampleFormat &out_sample_format();
            const enum AVSampleFormat &out_sample_format() const;

            int &out_sample_rate();
            const int &out_sample_rate() const;

            int64_t &in_channel_layout();
            const int64_t &in_channel_layout() const;

            enum AVSampleFormat &in_sample_format();
            const enum AVSampleFormat &in_sample_format() const;

            int &in_sample_rate();
            const int &in_sample_rate() const;

            //struct SwrContext *swr_context();
            const struct SwrContext *swr_context() const;

            std::string poll_error();

        private:

            struct SwrContext *m_swr_ctx;
            AVFrame *m_frame;

            int64_t                 m_out_channel_layout;
            enum AVSampleFormat     m_out_sample_format;
            int                     m_out_sample_rate;

            int64_t                 m_in_channel_layout;
            enum AVSampleFormat     m_in_sample_format;
            int                     m_in_sample_rate;

            std::queue<std::string> m_errors;

            void enqueue_error(const std::string &error);
            void enqueue_error(int error_code);
    };
}

// see resampler.cpp for definition
void poll_errors(FFmpeg::Frame_Resampler &resampler);
