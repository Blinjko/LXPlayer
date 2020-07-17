// Local header files
#include <sdl/renderer.h>
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
    /* Renderer Class constructor
     * @desc - takes all variables needed to create an SDL_Renderer and SDL_Texture
     * @params, see comments below
     */
    Renderer::Renderer(Window &window,                // A SDL::Window class reference, Window must be initialized before being passed to constructor
                       int driver_index,              // Index of the renderering driver to initialize the renderer with, -1 for first supporting one, see SDL2 wiki for more info
                       uint32_t renderer_flags,       // Flags to pass to the SDL_CreateRenderer() function, see SDL2 wiki for more info
                       uint32_t texture_pixel_format, // The SDL_PixelFormatEnum / pixel format to create the SDL_Texture with
                       int texture_access,            // The access to create the SDL_Texture with, see SDL wiki SDL_CreateTexture() for more info
                       int texture_width,             // Width in pixels to create the texture with
                       int texture_height) :          // height in pixels to create the texture with

        // set / initialize all variables
        m_window{window}, m_renderer{nullptr}, m_driver_index{driver_index}, m_renderer_flags{renderer_flags},
        m_texture{nullptr}, m_texture_pixel_format{texture_pixel_format}, m_texture_access{texture_access}, 
        m_texture_width{texture_width}, m_texture_height{texture_height}
    {}




    /* Renderer Class deconstructor
     * @desc - Destroys / deletes / frees the allocated SDL_Renderer and SDL_Texture if allocated / initialized
     * @note - SDL::Window will still be valid after a Renderer class instance has been deconstructed
     */
    Renderer::~Renderer()
    {
        if(m_texture)
        {
            SDL_DestroyTexture(m_texture);
        }

        if(m_renderer)
        {
            SDL_DestroyRenderer(m_renderer);
        }
    }




    /* Renderer class init() function
     * @desc - initializes the Renderer class instance, allocates SLD_Renderer, and SDL_Texture,
     * and prepares the class for rendering.
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     * @note - must be called after Renderer::reset() function for new values to take affect.
     * @note - if called more than once it will deallocate the previous SDL_Renderer and SDL_Texture, 
     * and reallocate new ones with the current values of the member variables
     */
    Return_Status Renderer::init()
    {
        if(m_texture)
        {
            SDL_DestroyTexture(m_texture);
        }

        if(m_renderer)
        {
            SDL_DestroyRenderer(m_renderer);
        }

        // create a SDL_Renderer
        m_renderer = SDL_CreateRenderer(m_window.window(),  // the SDL_Window to use
                                        m_driver_index,     // Driver index, see SDL2 wiki for more info
                                        m_renderer_flags);  // flags to pass to the renderer, see SDL2 wiki for more info
        // check for failed allocation
        if(!m_renderer)
        {
            enqueue_error("SDL::Renderer: Failed to create SDL_Renderer");
            enqueue_error(SDL_GetError());

            return STATUS_FAILURE;
        }

        // create a SDL_Texture
        m_texture = SDL_CreateTexture(m_renderer,             // the SDL_Renderer to initialize this texture with
                                      m_texture_pixel_format, // the SDL_PixelFormatEnum to initialize this texture with, see SDL2 wiki for more info
                                      m_texture_access,       // the SDL_TextureAccess to initialize this texture with, see SDL2 wiki for more info
                                      m_texture_width,        // the width in pixels to initialize this texture with
                                      m_texture_height);      // the height in pixels to initialize this texture with
        // check for failed allocation
        if(!m_texture)
        {
            enqueue_error("SDL::Renderer: Failed to create SDL_Texture");
            enqueue_error(SDL_GetError());

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Renderer::clear() function
     * @desc - clears the current rendering target, this is a wrapper around the SDL_RenderClear() function
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Renderer::clear()
    {
        int error = 0;
        
        error = SDL_RenderClear(m_renderer);
        if(error < 0)
        {
            enqueue_error("SDL::Renderer: Failed to clear renderer");
            enqueue_error(SDL_GetError());

            return STATUS_SUCCESS;
        }

        return STATUS_SUCCESS;
    }




    /* Renderer::update_texture() function
     * @desc - this function is a wrapper around the SDL_UpdateTexture function, see SDL2 wiki for more info
     * @param rect - Quoting SDL2 wiki "an SDL_Rect structure representing the area to update, or nullptr to update the entire texture"
     * @param pixel_data - the new pixel data to update the texture with
     * @param pitch - Quoting SDL2 wiki "the number of bytes in a row of pixel data, includeing padding between lines", also called linesize
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Renderer::update_texture(const SDL_Rect *rect, const void *pixel_data, int pitch)
    {
        if(!m_texture)
        {
            enqueue_error("SDL::Renderer: Failed to update texture, class not initialized");

            return STATUS_FAILURE;
        }

        int error = 0;
        
        // update the texture
        error = SDL_UpdateTexture(m_texture,  // the texture to update
                                  rect,       // see comment above function for what this does, or see SDL2 wiki
                                  pixel_data, // the pixel data to update / fill the texture with
                                  pitch);     // the pitch / linesize of the pixel data
        // check for errors
        if(error < 0)
        {
            enqueue_error("SDL::Renderer: Failed to update texture");
            enqueue_error(SDL_GetError());

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Renderer::copy() function
     * @desc - a wrapper around the SDL_RenderCopy function, copies a portion of the texture into the current rendering target
     * @param src_rect - Quoting SDL2 wiki "the source SDL_Rect structure or nullptr for the entire texture"
     * @param dst_rect - Quoting SDL2 wiki "the destination SDL_Rect structure or nullptr for the enture target; the texture will be stretched to fill the given rectangle"
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Renderer::copy(const SDL_Rect *src_rect, const SDL_Rect *dst_rect)
    {
        if(!m_renderer || !m_texture)
        {
            enqueue_error("SDL::Renderer: Failed to copy, class not initialized");

            return STATUS_FAILURE;
        }

        int error = 0;

        error = SDL_RenderCopy(m_renderer, m_texture, src_rect, dst_rect);

        if(error < 0)
        {
            enqueue_error("SDL::Renderer: Failed to copy");
            enqueue_error(SDL_GetError());

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Renderer::present() function
     * @desc - a wrapper around SDL_RenderPreset, updates the screen with any rendering performed since the previous call
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Renderer::present()
    {
        if(!m_renderer)
        {
            enqueue_error("SDL::Renderer: Failed to present, class not initialized");

            return STATUS_FAILURE;
        }

        SDL_RenderPresent(m_renderer);

        return STATUS_SUCCESS;
    }




    /* Renderer::render() function
     * @desc - combines functions Renderer::update_texture() and Renderer::copy() functions
     * @params - see the functions states above for more info
     * @return - Return_Status::STATUS_SUCCESS on success, Return_Status::STATUS_FAILURE on failure
     */
    Return_Status Renderer::render(const SDL_Rect *texture_rect, const void *pixel_data, int pitch, const SDL_Rect *src_rect, const SDL_Rect *dst_rect)
    {
        Return_Status status = STATUS_SUCCESS;

        status = update_texture(texture_rect, pixel_data, pitch);
        if(status == STATUS_FAILURE)
        {
            return status;
        }

        status = copy(src_rect, dst_rect);
        if(status == STATUS_FAILURE)
        {
            return status;
        }

        return status;
    }




    /* Renderer::reset() function
     * @desc - resets all member variables to passed values, a reconstructor
     * @note - after this is called, Renderer::init() must be called to changes to take affect
     */
    void Renderer::reset(Window &window,              // A SDL::Window class reference, Window must be initialized before being passed to constructor
                       int driver_index,              // Index of the renderering driver to initialize the renderer with, -1 for first supporting one, see SDL2 wiki for more info
                       uint32_t renderer_flags,       // Flags to pass to the SDL_CreateRenderer() function, see SDL2 wiki for more info
                       uint32_t texture_pixel_format, // The SDL_PixelFormatEnum / pixel format to create the SDL_Texture with
                       int texture_access,            // The access to create the SDL_Texture with, see SDL wiki SDL_CreateTexture() for more info
                       int texture_width,             // Width in pixels to create the texture with
                       int texture_height)            // height in pixels to create the texture with
    {
        m_window = window;
        m_driver_index = driver_index;
        m_renderer_flags = renderer_flags;
        m_texture_pixel_format = texture_pixel_format;
        m_texture_access = texture_access;
        m_texture_width = texture_width;
        m_texture_height = texture_height;
    }




    /* Renderer::poll_error() function
     * @desc - polls a std::string error messages off m_errors and returns it
     * @return - a std::string error message, if there are no errors the returned string will be empty
     */
    std::string Renderer::poll_error()
    {
        if(!m_errors.empty())
        {
            std::string error = m_errors.front();
            m_errors.pop();

            return error;
        }

        return std::string{};
    }

   

    // BIT OF INFORMATION //
    // The following functions are getters and setters
    // NOTE: getters are not prefixed with get
    // NOTE: setters are prefixed with set
    // NOTE: after setting a varialbe a user must call Renderer::init() (again) for it to take affect
    // ALSO: The getters and setters don't have comments above them because they are relatively simple
    // and self explaining
    // BIT OF INFORMATION END //




    Window &Renderer::window() { return m_window; }
    const Window &Renderer::window() const { return m_window; }
    void Renderer::set_window(Window &window) { m_window = window; }

    SDL_Renderer *Renderer::renderer() { return m_renderer; }
    const SDL_Renderer *Renderer::renderer() const { return m_renderer; }

    int Renderer::driver_index() const { return m_driver_index; }
    void Renderer::set_driver_index(int driver_index) { m_driver_index = driver_index; }

    uint32_t Renderer::renderer_flags() const { return m_renderer_flags; }
    void Renderer::set_renderer_flags(uint32_t renderer_flags) { m_renderer_flags = renderer_flags; }

    SDL_Texture *Renderer::texture() { return m_texture; }
    const SDL_Texture *Renderer::texture() const { return m_texture; }

    uint32_t Renderer::texture_pixel_format() const { return m_texture_pixel_format; }
    void Renderer::set_texture_pixel_format(uint32_t texture_pixel_format) { m_texture_pixel_format = texture_pixel_format; }

    int Renderer::texture_access() const { return m_texture_access; }
    void Renderer::set_texture_access(int texture_access) { m_texture_access = texture_access; }

    int Renderer::texture_width() const { return m_texture_width; }
    void Renderer::set_texture_width(int texture_width) { m_texture_width = texture_width; }

    int Renderer::texture_height() const { return m_texture_height; }
    void Renderer::set_texture_height(int texture_height) { m_texture_height = texture_height; }
    // Private Functions //




    /* Renderer::enqueue_error() function
     * @desc - enqueues an std::string error onto the error queue m_errors
     * @param error_msg - the std::string error message to enqueue
     */
    void Renderer::enqueue_error(const std::string &error_msg)
    {
        m_errors.push(error_msg);
    }
}




/* poll_errors function
 * @desc - polls all the errors out of an SDL::Renderer class instance and prints them
 * @param renderer - the SDL::Renderer class instance to poll errors from
 */
void poll_errors(SDL::Renderer &renderer)
{
    std::string error = renderer.poll_error();

    while(!error.empty())
    {
        std::cerr << error << std::endl;

        error = renderer.poll_error();
    }
}
