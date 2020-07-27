#include <ffmpeg/scale.h>

extern "C"
{
#include <libswscale/swscale.h>
}

namespace FFmpeg
{

    // Contsructor
    Scale::Scale() : m_sws_context{nullptr}
    {}

    // Destructor
    Scale::~Scale()
    {
        if(m_sws_context)
        {
            sws_freeContext(m_sws_context);
        }
    }

    // Overloaded SwsContext* cast operator
    Scale::operator struct SwsContext*() { return m_sws_context; }

    // overloaded -> operator
    struct SwsContext* Scale::operator ->() { return m_sws_context; }

    // overloaded = operator
    struct SwsContext* Scale::operator =(struct SwsContext *new_context) { return m_sws_context = new_context; }

    // getters //
    struct SwsContext* Scale::swscontext() { return m_sws_context; }
    const struct SwsContext* Scale::swscontext() const { return m_sws_context; }
}
