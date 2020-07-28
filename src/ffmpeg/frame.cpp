#include <ffmpeg/frame.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

namespace FFmpeg
{
    // Frame Class Start //

    // Constructor
    Frame::Frame() : m_frame{nullptr}
    {}

    // Destructor
    Frame::~Frame()
    {
        if(m_frame)
        {
            av_frame_free(&m_frame);
        }
    }

    // overloaded cast to AVFrame*
    Frame::operator AVFrame*() { return m_frame; }

    // overloaded -> operator
    AVFrame *Frame::operator->() { return m_frame; }

    // overloaded = operator
    AVFrame *Frame::operator=(AVFrame *frame) { return m_frame = frame; }

    // allocate function
    // Description: Allocates an avframe to hold an image with the given parameters
    // Parameter: pixel_format - the AVPixelFormat of the image to allocate
    // Parameter: width - width of the image to allocate
    // Parameter: height - height of the image to allocate
    // Returns: -1111 if failed to allocate the AVFrame, otherwise FFmpeg error code, a value >= 0 on success
    // If called more than once on the same frame memory will be leaked
    int Frame::allocate(enum AVPixelFormat pixel_format, int width, int height)
    {
        m_frame = av_frame_alloc();
        if(!m_frame)
        {
            return -1111;
        }

        int error{0};

        m_frame->format = static_cast<int>(pixel_format);
        m_frame->width = width;
        m_frame->height = height;

        error = av_frame_get_buffer(m_frame, 16);

        return error;
    }

    // Copies the src_frame into m_frame
    // Note: the allocate function must be called before any copies take place
    int Frame::copy(const AVFrame *src_frame)
    {
        int error{0};

        m_frame->pts = src_frame->pts;

        error = av_frame_copy(m_frame, src_frame);

        return error;
    }

    // getters //
    AVFrame *Frame::frame() { return m_frame; }
    const AVFrame *Frame::frame() const { return m_frame; }

    // Frame Class End //

    // Frame Array Start //

    // Constructor
    Frame_Array::Frame_Array(int size) : m_size{size}
    {
        m_array = new Frame[size]{};
    }

    // Desctructor
    Frame_Array::~Frame_Array()
    {
        delete[] m_array;
    }

    // operator [] overload
    // NOTE: no bounds checking is done
    Frame &Frame_Array::operator[](int index)
    {
        return m_array[index];
    }

    // operator[] const version
    const Frame &Frame_Array::operator[](int index) const
    {
        return m_array[index];
    }

    // getters //
    int Frame_Array::size() const { return m_size; }
    int Frame_Array::length() const { return m_size; }

    // Frame Array End //
}
