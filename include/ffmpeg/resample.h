#pragma once

extern "C"
{
#include <libswresample/swresample.h>
}

namespace FFmpeg
{
    // wrapper around SwrContext
    class Resample
    {
        public:
            Resample();
            Resample(const Resample&) = delete;

            ~Resample();

            operator struct SwrContext*();
            struct SwrContext* operator=(struct SwrContext*);
            struct SwrContext* operator->();

            struct SwrContext* swrcontext();
            const struct SwrContext* swrcontext() const;

        private:
            struct SwrContext *m_swr_context;
    };
}
