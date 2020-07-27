#pragma once

extern "C"
{
#include <libswscale/swscale.h>
}

namespace FFmpeg
{
    /* The Scale Class
     * Description: An RAII wrapper around struct SwsContext*
     */
    class Scale
    {
        public:
            Scale();
            Scale(const Scale&) = delete;

            ~Scale();

            operator struct SwsContext*();
            struct SwsContext* operator ->();
            struct SwsContext* operator =(struct SwsContext*);

            struct SwsContext* swscontext();
            const struct SwsContext* swscontext() const;

        private:
            struct SwsContext *m_sws_context;
    };
}
