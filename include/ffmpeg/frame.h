#pragma once

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

namespace FFmpeg
{
    /* Frame Class
     * Description: This is a RAII wrapper around an AVFrame*. When the deconstructor is called the frame gets freed if allocated
     */
    class Frame
    {
        public:
            Frame();
            Frame(const Frame&) = delete;

            ~Frame();

            operator AVFrame*();
            AVFrame *operator->();
            AVFrame *operator=(AVFrame*);


            int allocate(enum AVPixelFormat, int, int);

            int copy(const AVFrame*);

            AVFrame *frame();
            const AVFrame *frame() const;

        private:
            AVFrame *m_frame;
    };

    /* Frame Array class
     * Description: This is just a class for an array of Frames(see above). Had to use this because C++ doesn't have a fixed size array, where
     * the size is known at runtime, and I didn't want to use std::vector for other reasons
     */
    class Frame_Array
    {
        public:
            Frame_Array(int);
            Frame_Array(const Frame&) = delete;

            ~Frame_Array();

            Frame &operator[](int);
            const Frame &operator[](int) const;


            int size() const;
            int length() const;

        private:
            Frame *m_array;
            int m_size;
    };
}
