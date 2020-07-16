// personal header files
#include <ffmpeg/color_converter.h>

// library header files
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// C++ Standard Library header files
#include <string>
#include <queue>
#include <iostream>

namespace FFmpeg
{
    /* Color_Converter Class constructor
     * @desc sets and initializes member varialbes
     * @note to initialize a Color_Converter, call the Color_Converter::init() funciton
     */
    Color_Converter::Color_Converter(int src_width,                       // Source width ex: 1280
                                    int src_height,                       // Source height ex: 720
                                    enum AVPixelFormat src_pixel_format,  // Source pixel format, see AVPixelFormat in FFmpeg Documentation for all formats
                                    int dst_width,                        // Destination / Output width
                                    int dst_height,                       // Destination / Output height
                                    enum AVPixelFormat dst_pixel_format) :// Destination / Output pixel format

        m_src_width{src_width}, m_src_height{src_height}, m_src_pixel_format{src_pixel_format},
        m_dst_width{dst_width}, m_dst_height{dst_height}, m_dst_pixel_format{dst_pixel_format},
        m_sws_ctx{nullptr}
    {}




    /* Color_Converter Class deconstructor
     * @desc Frees any allocated member variables
     */
    Color_Converter::~Color_Converter()
    {
        if(m_sws_ctx)
        {
            sws_freeContext(m_sws_ctx);
            m_sws_ctx = nullptr;
        }

        if(m_dst_data.data[0])
        {
            av_freep(&m_dst_data.data[0]);
        }
    }




    /* Color_Converter Initialization function
     * @desc call this function to initialize the color converter
     * @return Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Color_Converter::init()
    {
        int error = 0;

        // check if m_src_pixel_format is supported input
        error = sws_isSupportedInput(m_src_pixel_format);
        if(error == 0)
        {
            std::string src_pix_fmt{av_get_pix_fmt_name(m_src_pixel_format)};
            std::string error_msg{"FFmpeg::Color_Converter: Failed to initialize, pixel format "};
            error_msg.append(src_pix_fmt);
            error_msg.append(" is not supported as conversion input");

            enqueue_error(error_msg);

            return STATUS_FAILURE;
        }

        // check if m_src_pixel_format is supported output
        error = sws_isSupportedOutput(m_dst_pixel_format);
        if(error == 0)
        {
            std::string dst_pix_fmt{av_get_pix_fmt_name(m_src_pixel_format)};
            std::string error_msg{"FFmpeg::Color_Converter: Failed to initialize, pixel format "};
            error_msg.append(dst_pix_fmt);
            error_msg.append(" is not supported as conversion output");

            enqueue_error(error_msg);

            return STATUS_FAILURE;
        }
        
        if(m_sws_ctx)
        {
            sws_freeContext(m_sws_ctx);
            m_sws_ctx = nullptr;
        }
        
        m_sws_ctx = sws_getContext(m_src_width,        // source width ex: 1920
                                   m_src_height,       // source height ex: 1080
                                   m_src_pixel_format, // source pixel format, see AVPixelFormat in FFmpeg documentation for all formats
                                   m_dst_width,        // destination / output width
                                   m_dst_height,       // destination / output height
                                   m_dst_pixel_format, // destination / ouput pixel format
                                   0,                  // flags
                                   nullptr,            // SwsFilter* source, nullptr for none
                                   nullptr,            // SwsFilter* destination, nullptr for none
                                   nullptr);           // extra parameters, nullptr for none
        if(!m_sws_ctx)
        {
            enqueue_error("FFmpeg::Color_Converter: Failed to allocate SwsContext");
            return STATUS_FAILURE;
        }

        error = sws_init_context(m_sws_ctx, nullptr, nullptr);
        if(error < 0)
        {
            enqueue_error("FFmpeg::Color_Converter: Failed to initialize SwsContext");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        // check if space for destination image data has been allocated
        if(!m_dst_data.data[0])
        {
            error = av_image_alloc(m_dst_data.data,      // Destination data pointers
                                   m_dst_data.linesize,  // Destination data linesize
                                   m_dst_width,          // Destination image width
                                   m_dst_height,         // Destination image height
                                   m_dst_pixel_format,   // Destination image pixel format
                                   IMAGE_ALIGNMENT);     // Bit alignment, default 16
            if(error < 0)
            {
                enqueue_error("FFmpeg::Color_Converter: Failed to allocate destination image data");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }

        return STATUS_SUCCESS;
    }




    /* Color_Converter convert function
     * @desc converts the given pixel data into m_dst_pixel_format pixel data
     * @param src_data - the source pixel data, pixel data to be converted
     * @param src_linesize - the source data linesize
     * @return - on success the returned Color_Data.lizesize[0] will be > 0,
     * @return - on failure the returned Color_Data.linesize[0] will be -1.
     * @note - if the passed pixel data is not in the same pixel format as m_src_pixel_format, errors will occur
     */
    Color_Data &Color_Converter::convert(const uint8_t *const src_data[], const int src_linesize[])
    {
        if(!m_sws_ctx)
        {
            enqueue_error("FFmpeg::Color_Converter: Failed to convert, not initialized");

            m_dst_data.linesize[0] = -1;
            return m_dst_data;
        }

        if(!m_dst_data.data[0])
        {
            enqueue_error("FFmpeg::Color_Converter: Failed to convert, space for destination data not allocated");

            m_dst_data.linesize[0] = -1;
            return m_dst_data;
        }

        int error = 0;
        
        error = sws_scale(m_sws_ctx,            // SwsContext to scale with
                          src_data,             // Source pixel data
                          src_linesize,         // Source linesize
                          0,                    // Position to start at, 0 = beginning
                          m_src_height,         // Source image height
                          m_dst_data.data,      // Destination pixel data, will be filled
                          m_dst_data.linesize); // Destination linesize,   will be filled
        if(error < 0)
        {
            enqueue_error("FFmpeg::Color_Converter: Failed to convert");
            enqueue_error(error);

            m_dst_data.linesize[0] = -1;
            return m_dst_data;
        }

        return m_dst_data;
    }




    /* Color_Converter reset function
     * @desc - this is essentially a reconstructor, except that it will free any existing contexts and data if any
     * @param src_width - the source width
     * @param src_height - the source height
     * @param src_pixel_format - the source pixel format
     * @param dst_width - the destination / output width
     * @param dst_height - the destination / ouput height
     * @param dst_pixel_format - the destination / output pixel format
     * @note - after this is called, Color_Converter::init() must be called again
     */
    void Color_Converter::reset(int src_width,                        // Source width ex: 1280
                                int src_height,                       // Source height ex: 720
                                enum AVPixelFormat src_pixel_format,  // Source pixel format, see AVPixelFormat in FFmpeg Documentation for all formats
                                int dst_width,                        // Destination / Output width
                                int dst_height,                       // Destination / Output height
                                enum AVPixelFormat dst_pixel_format)  // Destination / Output pixel format
    {
        m_src_width = src_width;
        m_src_height = src_height;
        m_src_pixel_format = src_pixel_format;
        
        m_dst_width = dst_width;
        m_dst_height = dst_height;
        m_dst_pixel_format = dst_pixel_format;

        if(m_sws_ctx)
        {
            sws_freeContext(m_sws_ctx);
            m_sws_ctx = nullptr;
        }
       
        if(m_dst_data.data[0])
        {
            av_freep(&m_dst_data.data[0]);
        }
    }




    // A BIT OF INFORMATION //
    // The following functions are getters and setters used to get and set member variables
    // NOTE: getters are not prefixed with get
    // NOTE: setters are prefixed with set
    // If any varialbes are changed using setters, Color_Coverter::init() must be called (again) for changes to be applied
    // INFORMATION BIT OVER //

    // There will be no comments for these functions as they are simple and mostly explained above //

    int Color_Converter::source_width() const { return m_src_width; }
    void Color_Converter::set_source_width(int width) { m_src_width = width; }

    int Color_Converter::source_height() const { return m_src_height; }
    void Color_Converter::set_source_height(int height) { m_src_height = height; }

    enum AVPixelFormat Color_Converter::source_pixel_format() const { return m_src_pixel_format; }
    void Color_Converter::set_source_pixel_format(enum AVPixelFormat pixel_format) { m_src_pixel_format = pixel_format; }

    int Color_Converter::destination_width() const { return m_dst_width; }
    void Color_Converter::set_destination_width(int width) { m_dst_width = width; }

    int Color_Converter::destination_height() const { return m_dst_height; }
    void Color_Converter::set_destination_height(int height) { m_dst_height = height; }

    enum AVPixelFormat Color_Converter::destination_pixel_format() const { return m_dst_pixel_format; }
    void Color_Converter::set_destination_pixel_format(enum AVPixelFormat pixel_format) { m_dst_pixel_format = pixel_format; }

    const struct SwsContext * Color_Converter::sws_context() const { return m_sws_ctx; }

    /* Color_Converter poll_error function
     * @desc - poll an std::string error message from the m_errors queue, poll and error message
     * @return - an std::string containing an error message, if no messages are in m_errors, an empty string is returned
     */
    std::string Color_Converter::poll_error()
    {
        if(!m_errors.empty())
        {
            std::string error;
            error = m_errors.front();
            m_errors.pop();

            return error;
        }

        return std::string{};
    }




    // Private Functions //




    /* Color_Converter enqueue error function
     * @desc - enqueue a string error messange onto the error queue m_errors
     * @param error_msg - the message to enqueue
     */
    void Color_Converter::enqueue_error(const std::string &error_msg)
    {
        m_errors.push(error_msg);
    }

    /* Color_Converter enqueue error function
     * @desc - enqueue a error message given an error code
     * @param error_code - the error code to derive a message from
     */
    void Color_Converter::enqueue_error(int error_code)
    {
        int error = 0;

        char buff[256];
        error = av_strerror(error_code, buff, sizeof(buff));

        std::string message{};

        if(error < 0)
        {
            message = "FFmpeg::Color_Converter: Unknown Error Code";    
        }

        else
        {
            message = buff;
        }

        m_errors.push(message);
    }
}




/* poll_errors function
 * @desc - poll all the errors from a FFmpeg::Color_Converter class instance
 * @param converter - the FFmpeg::Color_Converter class instance to poll errors from
 */
void poll_errors(FFmpeg::Color_Converter &converter)
{
    std::string error = converter.poll_error();

    while(!error.empty())
    {
        std::cerr << error << std::endl;
        error = converter.poll_error();
    }
}
