// local headers
#include <sdl/window.h>

// SDL2 library headers
extern "C"
{
#include <SDL2/SDL.h>
}

// C++ STD library headers
#include <string>
#include <queue>
#include <iostream>

namespace SDL
{
    /* Window class constructor
     * @desc - takes all variables needed to make an SDL_Window and sets member varialbes
     * @params - see below
     */
    Window::Window(const std::string &title, // title of the window
                   int x_pos,                // X position of the window, see SDL wiki, SDL_CreateWindow() for info
                   int y_pos,                // Y position of the window, see SDL wiki, SDL_CreateWindow() for info
                   int width,                // Width in pixels of the window
                   int height,               // Height in pixels of the window
                   uint32_t flags) :         // flags for SDL_CreateWindow() function see SDL wiki for info

        m_window{nullptr}, m_title{title}, m_x_pos{x_pos}, m_y_pos{y_pos},
        m_width{width}, m_height{height}, m_flags{flags}, m_pixel_format{SDL_PIXELFORMAT_UNKNOWN},
        m_refresh_rate{0}, m_fullscreen{false}
    {}




    /* Window class constructor
     * @desc - takes all variables needed to make an SDL_Window and sets member varialbes
     * @note - this just takes a const char* instead of an std::string
     */
    Window::Window(const char *title,        // title of the window
                   int x_pos,                // X position of the window, see SDL wiki, SDL_CreateWindow() for info
                   int y_pos,                // Y position of the window, see SDL wiki, SDL_CreateWindow() for info
                   int width,                // Width in pixels of the window
                   int height,               // Height in pixels of the window
                   uint32_t flags) :         // flags for SDL_CreateWindow() function see SDL wiki for info
    Window(std::string{title}, x_pos, y_pos, width, height, flags)
    {}




    /* Window class Destructor
     * @desc - closes the SDL_Window if open
     */
    Window::~Window()
    {
        if(m_window)
        {
            SDL_DestroyWindow(m_window);
        }
    }




    /* Window init function
     * @desc - creates a window with the given parameters
     * @return - Return_Status::STATUS_SUCCESS on success, and Return_Status::STATUS_FAILURE on failure
     * @note - will deallocate the previous window if call more then once
     */
    Return_Status Window::init()
    {
        if(m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        if(m_flags & SDL_WINDOW_FULLSCREEN || m_flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
        {
            m_fullscreen = true;
        }

        m_window = SDL_CreateWindow(m_title.c_str(), m_x_pos, m_y_pos, m_width, m_height, m_flags);
        if(!m_window)
        {
            enqueue_error("SDL::Window: Failed to create window");
            enqueue_error(std::string{SDL_GetError()});
            return STATUS_FAILURE;
        }

        int error = 0;

        SDL_DisplayMode mode;

        error = SDL_GetWindowDisplayMode(m_window, &mode);
        if(error < 0)
        {
            enqueue_error("SDL::Window: Failed to get window display mode");
            enqueue_error(std::string{SDL_GetError()});
            error = 0;

            m_pixel_format = SDL_PIXELFORMAT_UNKOWN;
            m_refresh_rate = -1;
        }
        
        else
        {
            m_pixel_format = mode.format;
            m_refresh_rate = mode.refresh_rate;
        }

        return STATUS_SUCCESS;
    }




    /* Window reset function
     * @desc - resets member variables, Window::init() must be called for changes to take affect
     */
    void Window::reset(const std::string &title, int x_pos, int y_pos, int width, int height, uint32_t flags)
    {
        m_title = title;
        m_x_pos = x_pos;
        m_y_pos = y_pos;
        m_width = width;
        m_height = height;
        m_flags = flags;
    }




    // const char* version
    void Window::reset(const char *title, int x_pos, int y_pos, int width, int height, uint32_t flags)
    {
        reset(std::string{title}, x_pos, y_pos, width, height, flags);
    }




    // The following functions are getters, so they dont have any above function comments

    SDL_Window *Window::window() { return m_window; }
    const SDL_Window *Window::window() const { return m_window; }

    int Window::x_position() const { return m_x_pos; }
    int Window::y_position() const { return m_y_pos; }

    uint32_t Window::pixel_format() const { return m_pixel_format; }
    int Window::refresh_rate() const { return m_refresh_rate; }

    bool Window::fullscreen() const { return m_fullscreen; }



    
    /* Window enter_fullscreen function
     * @desc - used to make the window fullscreen
     * @param flags - flags, can be found in SDL wiki for function SDL_SetWindowFullscreen()
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     * @note - will give errors if window is already fullscreened
     */
    Return_Status Window::enter_fullscreen(uint32_t flags)
    {
        if(m_fullscreen)
        {
            enqueue_error("SDL::Window: Window already in fullscreen mode");
            return STATUS_FAILURE;
        }

        int error = 0;
        error = SDL_SetWindowFullscreen(m_window, flags);

        if(error < 0)
        {
            enqueue_error("SDL::Window: Window failed to fullscreen");
            enqueue_error(std::string{SDL_GetError()});
            return STATUS_FAILURE;
        }
        
        m_fullscreen = true;
        return STATUS_SUCCESS;
    }




    /* Window exit_fullscreen function
     * @desc - exits fullscreen mode
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     * @note - will give errors if window is not fullscreened 
     */
    Return_Status Window::exit_fullscreen()
    {
        if(!m_fullscreen)
        {
            enqueue_error("SDL::Window: Window in windowed mode");
            return STATUS_FAILURE;
        }

        int error = 0;
        error = SDL_SetWindowFullscreen(m_window, 0);

        if(error < 0)
        {
            enqueue_error("SDL::Window: Window failed to exit fullscreen");
            enqueue_error(std::string{SDL_GetError()});
            return STATUS_FAILURE;
        }

        m_fullscreen = false;
        return STATUS_SUCCESS;
    }




    /* Window poll error function
     * @desc - polls an std::string error message off m_errors
     * @return - an std::sring error message, will be empty if no there is no error messages
     */
    std::string Window::poll_error()
    {
        if(!m_errors.empty())
        {
            std::string error = m_errors.front();
            m_errors.pop();

            return error;
        }

        return std::string{};
    }




    // Private functions //



    
    /* Window enqueue_error function
     * @desc - enqueues an std::string error message onto m_errors
     */
    void Window::enqueue_error(const std::string& error_message)
    {
        m_errors.push(error_message);
    }
}




/* SDL::Window poll errors function
 * @desc - polls all the errors out of a SDL::Window class instance and prints them
 */
void poll_errors(SDL::Window &window)
{
    std::string error{window.poll_error()};

    while(!error.empty())
    {
        std::cerr << error << std::endl;
        error = window.poll_error();
    }
}
