// local headers
#include <ffmpeg/resampler.h>

// FFmpeg library headers
extern "C"
{
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
}

// C++ standard library headers
#include <string>
#include <queue>
#include <iostream>

namespace FFmpeg
{
    /* Frame_Resampler Constructror
     * @param out_channel_layout, the output channel layout
     * @param out_sample_format, the output sample format
     * @param out_sample_rate, the output sample rate
     * @param in_channel_layout, the input channel layout
     * @param in_sample_format, the input sample format
     * @param in_sample_rate, the input sample rate
     */
    Frame_Resampler::Frame_Resampler(int64_t out_channel_layout,            // the output channel layout, AVFrame.channel_layout, or av_get_default_channel_layout(nb_channels)
                                     enum AVSampleFormat out_sample_format, // the output sample format, see FFmpeg documentation for formats
                                     int out_sample_rate,                   // the output sample rate EX 48000 Hz
                                     int64_t in_channel_layout,             // the input channel layout
                                     enum AVSampleFormat in_sample_format,  // the input sample format
                                     int in_sample_rate) :                  // int input sample rate
        m_swr_ctx{nullptr}, m_frame{nullptr},
        m_out_channel_layout{out_channel_layout}, m_out_sample_format{out_sample_format}, m_out_sample_rate{out_sample_rate},
        m_in_channel_layout{in_channel_layout}, m_in_sample_format{in_sample_format}, m_in_sample_rate{in_sample_rate}
    {}




    /* Frame_Resampler Destructor
     * @desc Frees m_swr_ctx if allocated, and unreferences and frees m_frame if allocated
     */
    Frame_Resampler::~Frame_Resampler()
    {
        if(m_swr_ctx)
        {
            swr_free(&m_swr_ctx);
        }

        if(m_frame)
        {
            av_frame_unref(m_frame);
            av_frame_free(&m_frame);
        }
    }




    /* Frame_Resampler::init() function
     * @desc - Initializes the Frame_Resampler class instance for resampling
     * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     * @note - if any options are changed this function must be called for them to take affect
     */
    Return_Status Frame_Resampler::init()
    {
        if(m_swr_ctx)
        {
            swr_free(&m_swr_ctx);
        }

        if(m_frame)
        {
            av_frame_unref(m_frame);
            av_frame_free(&m_frame);
        }

        int error = 0;
        m_swr_ctx = swr_alloc_set_opts(m_swr_ctx,            // SwrContext* to allocate
                                       m_out_channel_layout, // output channel layout
                                       m_out_sample_format,  // output sample format, see FFmpeg documentation AVSampleFormat for all formats
                                       m_out_sample_rate,    // output sample rate EX: 41000 Hz
                                       m_in_channel_layout,  // input channel layout
                                       m_in_sample_format,   // input sample format
                                       m_in_sample_rate,     // input sample rate
                                       0,                    // logging level offset
                                       nullptr);             // logging context, nullptr for none

        if(!m_swr_ctx)
        {
            enqueue_error("FFmpeg::Frame_Resampler: Failed to allocate SwrContext");
            return STATUS_FAILURE;
        }

        error = swr_init(m_swr_ctx);
        if(error < 0)
        {
            enqueue_error("FFmpeg::Frame_Resampler: Failed to initialize SwrContext");
            enqueue_error(error);
            return STATUS_FAILURE;
        }


        m_frame = av_frame_alloc();
        if(!m_frame)
        {
            enqueue_error("FFmpeg::Frame_Resampler: Failed to allocate frame");
            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Frame_Resampler::resample_frame() function
     * @desc resamples a decoded audio frame to the set output options
     * @param source_frame, AVFrame* that holds decoded audio data
     * @return valid AVFrame* on success, nullptr on failure
     * @note the returned AVFrame* points to the same data as m_frame, and when this function is called again,
     * @note the previously returned pointer will be invalid.
     */
    AVFrame *Frame_Resampler::resample_frame(AVFrame *source_frame)
    {
        if(!m_frame || !m_swr_ctx)
        {
            enqueue_error("FFmpeg::Frame_Resampler: Resampler not initialized");
            return nullptr;
        }

        int error = 0;

        av_frame_unref(m_frame);

        m_frame->channel_layout = m_out_channel_layout;
        m_frame->format = m_out_sample_format;
        m_frame->sample_rate = m_out_sample_rate;

        error = swr_convert_frame(m_swr_ctx, m_frame, source_frame);
        if(error < 0)
        {
            enqueue_error("FFmpeg::Frame_Resampler: Failed to convert frame");
            enqueue_error(error);
            return nullptr;
        }

        return m_frame;
    }




    /* Frame_Resampler::reset_options() function
     * @desc - reset all the option member variables
     * @param out_channel_layout, the output channel layout
     * @param out_sample_format, the output sample format
     * @param out_sample_rate, the output sample rate
     * @param in_channel_layout, the input channel layout
     * @param in_sample_format, the input sample format
     * @param in_sample_rate, the input sample rate
     * @note - after calling this function Frame_Resampler::init() must be called for changes to take affect
     */
    void Frame_Resampler::reset_options(int64_t out_channel_layout,            // the output channel layout
                                        enum AVSampleFormat out_sample_format, // the output sample format, see FFmpeg AVSampleFormat documentation for all formats
                                        int out_sample_rate,                   // output sample rate EX: 44000 Hz
                                        int64_t in_channel_layout,             // input channel layout
                                        enum AVSampleFormat in_sample_format,  // input sample format
                                        int in_sample_rate)                    // input sample rate
    {
        m_out_channel_layout = out_channel_layout;
        m_out_sample_format = out_sample_format;
        m_out_sample_rate = out_sample_rate;

        m_in_channel_layout = in_channel_layout;
        m_in_sample_format = in_sample_format;
        m_in_sample_rate = in_sample_rate;

    }


    // BIT OF INFORMATION //
    // The following 13 functions return references to member variables.
    // If you would like to change an option of a Frame_Resampler class instance,
    // call the apropriate function and assign the value you would like.
    // This works becuase they return references to the actual member variable in the used Frame_Resampler class instance
    // After you have set your options using these functions you must call Frame_Resampler::init() function again for the options to take affect
    // ALSO these functions are simple, so there wont be any above function comments
    // BIT OF INFORMATION END //

    int64_t &Frame_Resampler::out_channel_layout() { return m_out_channel_layout; }
    const int64_t &Frame_Resampler::out_channel_layout() const { return m_out_channel_layout; }

    enum AVSampleFormat &Frame_Resampler::out_sample_format() { return m_out_sample_format; }
    const enum AVSampleFormat &Frame_Resampler::out_sample_format() const { return m_out_sample_format; }

    int &Frame_Resampler::out_sample_rate() { return m_out_sample_rate; }
    const int &Frame_Resampler::out_sample_rate() const { return m_out_sample_rate; }

    int64_t &Frame_Resampler::in_channel_layout() { return m_in_channel_layout; }
    const int64_t &Frame_Resampler::in_channel_layout() const { return m_in_channel_layout; }

    enum AVSampleFormat &Frame_Resampler::in_sample_format() { return m_in_sample_format; }
    const enum AVSampleFormat &Frame_Resampler::in_sample_format() const { return m_in_sample_format; }

    int &Frame_Resampler::in_sample_rate() { return m_in_sample_rate; }
    const int &Frame_Resampler::in_sample_rate() const { return m_in_sample_rate; }

    // Cannot change m_swr_ctx :D
    const struct SwrContext *Frame_Resampler::swr_context() const { return m_swr_ctx; }




    /* Frame_Resampler::poll_error() function
     * @desc polls an error message from m_errors and returns it
     * @return std::string error message, the string will be empty if there are no messages.
     */
    std::string Frame_Resampler::poll_error()
    {
        if(!m_errors.empty())
        {
            std::string return_val = m_errors.front();
            m_errors.pop();
            return return_val;
        }

        return std::string{};
    }




    // Private Functions //




    /* Frame_Resampler::enqueue_error() function
     * @desc enqueues an std::string error onto m_errors
     * @param error, the std::string error message to enqueue onto m_errors
     */
    void Frame_Resampler::enqueue_error(const std::string &error)
    {
        m_errors.push(error);
    }




    /* Frame_Resampler::enqueue_error() function
     * @desc enqueues an ffmpeg error message for the given error code onto m_errors
     * @param error_code - The error code to translate to an error message
     */
    void Frame_Resampler::enqueue_error(int error_code)
    {
        char buff[256];
        int error = av_strerror(error_code, buff, sizeof(buff));

        std::string error_msg;

        if(error < 0)
        {
            error_msg = "FFmepg::Frame_Resampler: Unkown Error Code";
        }

        else
        {
            error_msg = buff;
        }

        m_errors.push(error_msg);
    }
}




/* poll_errors function
 * @desc - polls all errors from a FFmpeg::Frame_Resampler class instance and prints them
 * @param resampler - the FFmpeg::Frame_Resampler class instance to poll errors from
 */
void poll_errors(FFmpeg::Frame_Resampler &resampler)
{
    std::string error{resampler.poll_error()};

    while(!error.empty())
    {
        std::cerr << error << std::endl;
        error = resampler.poll_error();
    }
}
