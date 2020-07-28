#include <ffmpeg/resample.h>

extern "C"
{
#include <libswresample/swresample.h>
}

namespace FFmpeg
{
    // Resample constructor
    Resample::Resample() : m_swr_context{nullptr}
    {}

    
    // Resample Destructor
    // Frees m_swr_context if allocated
    Resample::~Resample()
    {
        if(m_swr_context)
        {
            swr_close(m_swr_context);
            swr_free(&m_swr_context);
            m_swr_context = nullptr;
        }
    }

    // overloaded cast operator
    Resample::operator struct SwrContext*() { return m_swr_context; }

    // overloaded = operator
    struct SwrContext* Resample::operator=(struct SwrContext *swr_context) 
    {
        return m_swr_context = swr_context;
    }

    // overloaded -> operator
    struct SwrContext* Resample::operator->() { return m_swr_context; }

    // getters //
    struct SwrContext* Resample::swrcontext() { return m_swr_context; }
    const struct SwrContext* Resample::swrcontext() const { return m_swr_context; }
}
