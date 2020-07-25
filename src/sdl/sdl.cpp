#include <sdl/sdl.h>

extern "C"
{
#include <SDL2/SDL.h>
}

#include <iostream>
#include <cstdlib>

namespace SDL
{
    // Initializer start //

    // Constructor, will exit if errors occur
    Initializer::Initializer(uint32_t flags)
    {
        int error{SDL_Init(flags)};

        if(error < 0)
        {
            std::cerr << "Failed to initialize SDL, Error Message: " << SDL_GetError() << std::endl;
            std::exit(1);
        }
    }


    // Destructor
    Initializer::~Initializer()
    {
        SDL_Quit();
    }

    // Initializer end //


    // Window Start //

    // Constructor //
    Window::Window() : m_window{nullptr}
    {}

    // Destructor
    Window::~Window()
    {
        if(m_window)
        {
            SDL_DestroyWindow(m_window);
        }
    }

    // overloaded SDL_Window* cast operator
    Window::operator SDL_Window*() { return m_window; }

    // overloaded = operator
    SDL_Window *Window::operator =(SDL_Window *window) { return m_window = window; }

    // Getters //
    SDL_Window* Window::window() { return m_window; }
    const SDL_Window* Window::window() const { return m_window; }

    // Window End //


    // Renderer Start //

    // Constructor
    Renderer::Renderer() : m_renderer{nullptr}
    {}

    // Destructor
    Renderer::~Renderer()
    {
        if(m_renderer)
        {
            SDL_DestroyRenderer(m_renderer);
        }
    }

    // overloaded SDL_Renderer* cast operator
    Renderer::operator SDL_Renderer*() { return m_renderer; }
    
    // overloaded = operator
    SDL_Renderer *Renderer::operator =(SDL_Renderer *renderer) { return m_renderer = renderer; }


    // Getters //
    SDL_Renderer* Renderer::renderer() { return m_renderer; }
    const SDL_Renderer* Renderer::renderer() const { return m_renderer; }

    // Renderer End //


    // Texture Start //

    // Constructor //
    Texture::Texture() : m_texture{nullptr}
    {}

    // Destructor
    Texture::~Texture()
    {
        if(m_texture)
        {
            SDL_DestroyTexture(m_texture);
        }
    }

    // overloaded SDL_Texture* cast operator
    Texture::operator SDL_Texture*() { return m_texture; }

    // overloaded = operator
    SDL_Texture *Texture::operator =(SDL_Texture *texture) { return m_texture = texture; }

    // Getters //
    SDL_Texture* Texture::texture() { return m_texture; }
    const SDL_Texture* Texture::texture() const { return m_texture; }

    // Texture End //
}
