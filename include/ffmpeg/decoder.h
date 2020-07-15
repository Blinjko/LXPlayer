#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}
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
    // SEE "ffmpeg_decoder.cpp" for comments on functions //

    /* FFmpeg_Decoder Class
     * @member m_fmt_ctx, AVFormatContext* holds information about the opened file
     * @member m_codec_ctx, AVCodecContext* holds codec information for the decoder
     * @member m_stream_number, the stream number in AVFormatContext::streams[] that is being decoded
     * @member m_packet, AVPacket* holds encoded data read from the opened file
     * @member m_frame, AVFrame* holds decoded data and information about it
     * @member m_media_type, enum AVMediaType to tell the program what media type is to be decoded
     * @member m_end_of_file, a boolean that keeps note if the end of the file was reached.
     * @member m_filename, std::string that holds the filename
     * @member m_errors, std::queue<std::string>, a queue of std::strings holding error messages
     * @note For information on class functions see "ffmpeg_decoder.cpp"
     */
    class Decoder
    {
        public:

            Decoder(const std::string&, enum AVMediaType);
            Decoder(const char*, enum AVMediaType);
            ~Decoder();

            Return_Status open_file();
            Return_Status init();
            Return_Status drain();
            void reset(const std::string&, enum AVMediaType);

            AVFrame *decode_frame();

            std::string poll_error();

            const AVFormatContext *get_format_context() const;
            const AVCodecContext *get_codec_context() const;

            enum AVMediaType get_media_type() const;
            std::string get_filename() const;
            bool end_of_file_reached() const;

        private:

            std::string m_filename;

            AVFormatContext *m_fmt_ctx;
            AVCodecContext *m_codec_ctx;
            enum AVMediaType m_media_type;

            AVPacket *m_packet;
            AVFrame *m_frame;

            int m_stream_number;
            bool m_end_of_file;

            std::queue<std::string> m_errors;

            Return_Status decoder_fill();
            void enqueue_error(int error_code);
            void enqueue_error(const std::string &message);
    };
}

// defined in decoder.cpp
void poll_errors(FFmpeg::Decoder &decoder);
