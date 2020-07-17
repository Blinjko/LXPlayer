#pragma once

// local headers
#include <sdl/window.h>

// SDL2 Library headers
extern "C"
{
#include <SDL2/SDL.h>
}

// C++ STD library headers
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

namespace SDL
{
    class Renderer
    {
        public:

            Renderer(Window&, int, uint32_t, uint32_t, int, int, int);
            Renderer(const Renderer&) = delete;
            ~Renderer();

            Return_Status init();
            Return_Status clear();
            Return_Status update_texture(const SDL_Rect*, const void*, int);
            Return_Status copy(const SDL_Rect*, const SDL_Rect*);
            Return_Status present();

            Return_Status render(const SDL_Rect*, const void*, int, const SDL_Rect*, const SDL_Rect*);

            void reset(Window&, int, uint32_t, uint32_t, int, int, int);

            Window &window();
            const Window &window() const;
            void set_window(Window&);

            SDL_Renderer *renderer();
            const SDL_Renderer *renderer() const;

            int driver_index() const;
            void set_driver_index(int);

            uint32_t renderer_flags() const;
            void set_renderer_flags(uint32_t);

            SDL_Texture *texture();
            const SDL_Texture *texture() const;

            uint32_t texture_pixel_format() const;
            void set_texture_pixel_format(uint32_t);

            int texture_access() const;
            void set_texture_access(int);

            int texture_width() const;
            void set_texture_width(int);

            int texture_height() const;
            void set_texture_height(int);
            
            std::string poll_error();

        private:

            Window &m_window;
            SDL_Renderer *m_renderer;
            int m_driver_index;
            uint32_t m_renderer_flags;

            SDL_Texture *m_texture;
            uint32_t m_texture_pixel_format;
            int m_texture_access;
            int m_texture_width;
            int m_texture_height;

            std::queue<std::string> m_errors;

            void enqueue_error(const std::string&);
    };
}

void poll_errors(SDL::Renderer&);
