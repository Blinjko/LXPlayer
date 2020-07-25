#pragma once

extern "C"
{
#include <SDL2/SDL.h>
}

namespace SDL
{
    // Initializer struct
    // Description: A RAII wrapper around the SDL_Init() and SDL_Quit() calls
    struct Initializer
    {
        Initializer(uint32_t);
        ~Initializer();
    };

    // Window class
    // Description: A RAII wrapper around SDL_Window*, usable with SDL functions
    class Window
    {
        public:
            Window();

            Window(const Window&) = delete;

            ~Window();

            operator SDL_Window*();

            SDL_Window *operator =(SDL_Window*);


            SDL_Window* window();
            const SDL_Window* window() const;

        private:
            SDL_Window *m_window;

    };

    // Renderer class
    // Description: A RAII wrapper around SDL_Renderer*, usable with SDL functions
    class Renderer
    {
        public:
            Renderer();

            Renderer(const Renderer&) = delete;

            ~Renderer();

            operator SDL_Renderer*();

            SDL_Renderer *operator =(SDL_Renderer*);


            SDL_Renderer* renderer();
            const SDL_Renderer* renderer() const;

        private:
            SDL_Renderer *m_renderer;

    };

    // Texture class
    // Description: A RAII wrapper around SDL_Texture*, usable with SDL functions
    class Texture
    {
        public:
            Texture();

            Texture(const Texture&) = delete;

            ~Texture();

            operator SDL_Texture*();

            SDL_Texture *operator =(SDL_Texture*);


            SDL_Texture* texture();
            const SDL_Texture* texture() const;

        private:
            SDL_Texture *m_texture;

    };
}
