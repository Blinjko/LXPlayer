#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
} 
#include <string>

namespace FFmpeg
{
    /* The Decoder Class
     * Decsription: The Decoder class wraps up the FFmpeg data types and functions needed for decoding into a class,
     * this makes it easier to setup a decoder and use it. This class does not use exceptions so functions will return
     * the FFmpeg error codes if they fail, however some functions will return -1111 which is not an FFmpeg error code
     * and it means that a ffmpeg / libav function that doesn't return an error code failed. These functions are usually allation or opening functions.
     *
     * How to use: Assuming object has been constructed
     * 1. call init_format_context(filename, options) // This initializes the Decoder format context and opens the file supplied
     * 2. call find_stream(MEDIA_TYPE) // This function finds the best media stream in the opened file of the specified media type, for media types see AVMediaType in FFmpeg Documentation
     * 3. call init_codec_context(options, number_of_threads_to_use) // This function initializes a codec context for decoding the selected stream
     * 4. call send_packet() in a loop until it returns AVERROR_EOF or AVERROR(EAGAIN) // This feeds data to the decoder, the return value AVERROR(EAGAIN) means the decoder cannot take any more data
     * 5. call recive_frame(Frame) in a loop until it returns AVERROR(EAGAIN) inidicating the decoder needs more data // This function decodes the data fed to the decoder via send_packet(),
     * note this function outputs the decoded data into the passed AVFrame**
     * 6. Repeat step 4 & 5 until AVERROR_EOF is given by send_packet() indicating the end of the file has been reached
     */
    class Decoder
    {
        public:
            Decoder();
            Decoder(const Decoder&) = delete;

            ~Decoder();

            int init_format_context(const std::string&, AVDictionary**);
            int find_stream(enum AVMediaType);
            int init_codec_context(AVDictionary**, int);
            int send_packet();
            int receive_frame(AVFrame**);
            void free_resources();

            const AVFormatContext *format_context() const;

            AVCodec *codec();
            const AVCodec *codec() const;

            const AVCodecContext *codec_context() const;

            const AVPacket *packet() const;

            AVFrame *frame();
            const AVFrame *frame() const;

            std::string filename() const;

            int stream_number() const;

        private:
            AVFormatContext *m_fmt_ctx;
            AVCodec *m_codec;
            AVCodecContext *m_codec_ctx;
            AVPacket *m_packet;
            AVFrame *m_frame;

            std::string m_filename;
            int m_stream_number;

    };
}
