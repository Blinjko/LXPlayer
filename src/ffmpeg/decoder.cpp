#include <ffmpeg/decoder.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <string>

namespace FFmpeg
{

    // Constructor
    Decoder::Decoder() :
        m_fmt_ctx{nullptr}, m_codec{nullptr}, m_codec_ctx{nullptr},
        m_packet{nullptr}, m_frame{nullptr}, m_filename{"DECODER CLASS DEFAULT FILENAME"}, 
        m_stream_number{-1}
    {}

    // Deconstructor
    Decoder::~Decoder()
    {
        free_resources();
    }

    /* init_format_context() function
     * Description: Takes a filename and options, and uses them to initialize a AVFormatContext
     * Parameter: filename - the name of the file to open
     * Parameter: options - an AVDictionary of options to pass to av_open_input()
     * Return: -1111 for failed allocation of the AVFormatContext, otherwise FFmpeg error codes, value >= 0 success
     */
    int Decoder::init_format_context(const std::string &filename, AVDictionary **options)
    {
        // allocate the format context
        m_fmt_ctx = avformat_alloc_context();
        if(!m_fmt_ctx)
        {
            return -1111;
        }

        int error{0};

        // open the file
        error = avformat_open_input(&m_fmt_ctx, filename.c_str(), nullptr, options);
        if(error < 0)
        {
            avformat_free_context(m_fmt_ctx);
            return error;
        }

        // read stream info for extra details and information
        error = avformat_find_stream_info(m_fmt_ctx, nullptr);

        return error;
    }

    /* find_stream function
     * Description: Finds the best stream in the opened file, of the given AVMediaType
     * Parameter: media_type - the media type of the desired stream
     * Return: FFmpeg error code on failure, a value >= 0 on success
     */
    int Decoder::find_stream(enum AVMediaType media_type)
    {
        int error{0};

        // find the best stream
        error = av_find_best_stream(m_fmt_ctx, media_type, -1, -1, nullptr, 0);

        m_stream_number = error * (error >= 0);

        return error;
    }

    /* init_codec_context() function
     * Description: Initializes an AVCodecContext for decoding
     * Parameter: options - AVDictionary of options to pass to the AVCodecContext open function
     * Parameter: threads - the number of threads to use for decoding
     * Return: -1111 on failed AVCodecContext allocation, or failed to find a AVCodec for decoding, otherwise FFmpeg error code, or a value >= 0 on success
     */
    int Decoder::init_codec_context(AVDictionary** options, int threads)
    {
        // get the stream selected for decoding
        AVStream *selected_stream{m_fmt_ctx->streams[m_stream_number]};

        // find the correct codec
        m_codec = avcodec_find_decoder(selected_stream->codecpar->codec_id);
        if(!m_codec)
        {
            return -1111;
        }

        // allocate a AVCodecContext
        m_codec_ctx = avcodec_alloc_context3(m_codec);
        if(!m_codec_ctx)
        {
            return -1111;
        }

        int error{0};

        // copy AVCodecParameters from the selected stream to the AVCodecContext
        error = avcodec_parameters_to_context(m_codec_ctx, selected_stream->codecpar);
        if(error < 0)
        {
            return error;
        }

        m_codec_ctx->thread_count = threads;
        error = avcodec_open2(m_codec_ctx, m_codec, options);
        if(error < 0)
        {
            return error;
        }

        // allocate a packet
        m_packet = av_packet_alloc();
        if(!m_packet)
        {
            return -1111;
        }

        // allocate a frame
        m_frame = av_frame_alloc();
        if(!m_frame)
        {
            return -1111;
        }

        return error;
    }

    /* send_packet() function
     * Description: reads a packet from the opened file, and sends it to the decoder
     * Return: AVERROR_EOF when end of file has been reached, AVERROR(EAGAIN) when the decoder needs data read from it, otherwise FFmpeg error code, or value >= 0 on success
     */
    int Decoder::send_packet()
    {
        int error{0};

        while(1)
        {
            // if the packet doesn't contain any data
            if(!m_packet->data)
            {
                // read data from the file
                error = av_read_frame(m_fmt_ctx, m_packet);
                if(error < 0)
                {
                    return error;
                }
            }

            // if the data contained in the packet is not from the right stream
            if(m_packet->stream_index != m_stream_number)
            {
                // unreference the packet
                av_packet_unref(m_packet);
            }


            else
            {
                break;
            }
        }

        // send the packet to the decoder
        error = avcodec_send_packet(m_codec_ctx, m_packet);

        // if an error occurrs the packet is not unreferenced
        if(error < 0)
        {
            return error;
        }

        // if no errors occurr unreference the packet
        av_packet_unref(m_packet);
        return error;
    }

    /* receive_frame() function
     * Description: Reads a frame from the decoder and puts into output_frame
     * Paramter: output_frame - the output for the read frame
     * Return: AVERROR(EAGAIN) when the decoder needs more data fed to it, othersise FFmpeg error code, or a value >= 0  on success
     */
    int Decoder::receive_frame(AVFrame** output_frame)
    {
        // unreference the frame
        av_frame_unref(m_frame);

        int error{0};

        // receive a new frame from the decoder
        error = avcodec_receive_frame(m_codec_ctx, m_frame);

        if(error < 0)
        {
            return error;
        }

        // set the output frame to the decoded frame
        *output_frame = m_frame;

        return error;
    }

    // Frees all allocated / initialized resources 
    void Decoder::free_resources()
    {
        if(m_frame)
        {
            av_frame_unref(m_frame);
            av_frame_free(&m_frame);
        }

        if(m_packet)
        {
            av_packet_unref(m_packet);
            av_packet_free(&m_packet);
        }

        if(m_codec_ctx)
        {
            avcodec_free_context(&m_codec_ctx);
        }

        m_codec = nullptr;

        if(m_fmt_ctx)
        {
            avformat_close_input(&m_fmt_ctx);
            avformat_free_context(m_fmt_ctx);
        }

        m_filename = "DECODER CLASS DEFAULT FILENAME";
        m_stream_number = -1;
    }


    // Getters //
    const AVFormatContext *Decoder::format_context() const { return m_fmt_ctx; }

    AVCodec *Decoder::codec() { return m_codec; }
    const AVCodec *Decoder::codec() const { return m_codec; }

    const AVCodecContext *Decoder::codec_context() const { return m_codec_ctx; }

    const AVPacket *Decoder::packet() const { return m_packet; }

    AVFrame *Decoder::frame() { return m_frame; }
    const AVFrame *Decoder::frame() const { return m_frame; }

    std::string Decoder::filename() const { return m_filename; }

    int Decoder::stream_number() const { return m_stream_number; }
}
