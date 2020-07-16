#pragma once
// SDL2 library headers
extern "C"
{
#include <SDL2/SDL.h>
}

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
    class Window
    {
        public:
            
            Window(const std::string&, int, int, int, int, uint32_t);
            Window(const char*, int, int, int, int, uint32_t);

            Window(const Window&) = delete;

            ~Window();

            Return_Status init();

            void reset(const std::string&, int, int, int, int, uint32_t);
            void reset(const char*, int, int, int, int, uint32_t);

            SDL_Window *window();
            const SDL_Window *window() const;

            int x_position() const;
            int y_position() const;

            uint32_t pixel_format() const;
            int refresh_rate() const;

            bool fullscreen() const;

            Return_Status enter_fullscreen(uint32_t);
            Return_Status exit_fullscreen();

            std::string poll_error();

        private:

            SDL_Window *m_window;
            std::string m_title;

            int m_x_pos;
            int m_y_pos;

            int m_width;
            int m_height;

            uint32_t m_flags;

            uint32_t m_pixel_format;
            int m_refresh_rate;

            bool m_fullscreen;

            std::queue<std::string> m_errors;

            void enqueue_error(const std::string&);
    };
}

void poll_errors(SDL::Window&);
