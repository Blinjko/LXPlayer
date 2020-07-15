#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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
    const int IMAGE_ALIGNMENT = 16;
    struct Color_Data
    {
        uint8_t *data[4];
        int linesize[4];
    };

    class Color_Converter
    {
        public:
            Color_Converter(int, int, enum AVPixelFormat, int, int, enum AVPixelFormat);
            ~Color_Converter();

            Return_Status init();
            Color_Data &convert(const uint8_t *const[], const int[]);
            void reset(int, int, enum AVPixelFormat, int, int, enum AVPixelFormat);

            int &source_width();
            const int &source_width() const;

            int &source_height();
            const int &source_height() const;

            enum AVPixelFormat &source_pixel_format();
            const enum AVPixelFormat &source_pixel_format() const;

            
            int &destination_width();
            const int &destination_width() const;

            int &destination_height();
            const int &destination_height() const;

            enum AVPixelFormat &destination_pixel_format();
            const enum AVPixelFormat &destination_pixel_format() const;

            struct SwsContext *sws_context();
            const struct SwsContext *sws_context() const;

            std::string poll_error();

        private:
            int m_src_width;
            int m_src_height;
            enum AVPixelFormat m_src_pixel_format;

            int m_dst_width;
            int m_dst_height;
            enum AVPixelFormat m_dst_pixel_format;

            struct SwsContext *m_sws_ctx;
            Color_Data m_dst_data;

            std::queue<std::string> m_errors;

            void enqueue_error(const std::string&);
            void enqueue_error(int error_code);
    };
}

// defined in color_converter.cpp
void poll_errors(FFmpeg::Color_Converter &converter);
