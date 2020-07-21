// local headers 
#include <portaudio/playback.h>

// library headers
extern "C"
{
#include <portaudio.h>
}

// C++ standard library headers
#include <string>
#include <queue>
#include <iostream>

namespace PortAudio
{
    /* Playback Constructor
     * @desc - Takes and sets all needed varialbes
     * @params - can be found below
     * @note - if you aren't using callbacks set the callbacks
     * to nullptr and use the Playback::write() function
     */
    Playback::Playback(PaHostApiIndex host_api_index,             // Index of host api to use
             PaDeviceIndex device_index,                          // Index of device do use
             int channels,                                        // Number of output channels
             PaSampleFormat sample_format,                        // Sample format, EX paInt16
             PaTime suggested_latency,                            // Suggested latency in seconds, PaTime is a double, so 1.0 would be a second, -1.0 for auto set
             void* host_api_specific_stream_info,                 // host api specific stream info, nullptr for none
             int sample_rate,                                     // Sample rate of audio data, EX: 48000 Hz
             PaStreamCallback* stream_callback,                   // Stream callback function, nullptr if not using callbacks
             PaStreamFinishedCallback* stream_finished_callback,  // Stream finished callback function, nullptr if not using callbacks
             void *callback_user_data) :                          // callback user data, client / user supplied pointer which will be passed to the callback function, nullptr if not using callbacks

        m_host_api_index{host_api_index}, m_device_index{device_index}, m_channels{channels},
        m_sample_format{sample_format}, m_suggested_latency{suggested_latency}, m_host_api_specific_stream_info{host_api_specific_stream_info},
        m_sample_rate{sample_rate}, m_stream_callback{stream_callback}, m_stream_finished_callback{stream_finished_callback}, 
        m_callback_user_data{callback_user_data}, m_stream{nullptr}
    {}


   

    /* Playback deconstructor
     * @desc - closes an audio stream if one is open
     */
    Playback::~Playback()
    {
        if(m_stream)
        {
            PaError error = 0;
            error = Pa_CloseStream(m_stream);

            if(error < 0)
            {
                std::cerr << "PortAuido::Playback: Error when deconstructing, " << Pa_GetErrorText(error) << std::endl;
            }
        }
    }




    /* Playback initialization function
     * @desc - initializes the Playback class instance for audio playback
     * @return - Return_Status::SUCCESS on success, Return_Status::FAILURE on failure
     * @note - if any options / variables were changed using reset_options() or the other individual functions
     * this function must be called (again) for the changes to take affect
     */
    Return_Status Playback::init()
    {
        PaError error = 0;

        // close stream if open
        if(m_stream)
        {
            error = Pa_CloseStream(m_stream);

            if(error < 0)
            {
                enqueue_error("PortAuido::Playback: Failed to initialize, failed to close existing stream");
                enqueue_error(error);
                return STATUS_FAILURE;
            }
        }


        // auto set suggested latency with device information
        if(m_suggested_latency == -1.0)
        {
            const PaDeviceInfo *device_info = Pa_GetDeviceInfo(m_device_index);

            if(!device_info)
            {
                enqueue_error("PortAudio::Playback: Failed to get device info, invalid device index");
                return STATUS_FAILURE;
            }

            m_suggested_latency = device_info->defaultHighInputLatency;
        }
        
        // create Stream paramets
        PaStreamParameters stream_params;
        stream_params.device = m_device_index;                // set device to use 
        stream_params.channelCount = m_channels;              // set number of channels
        stream_params.sampleFormat = m_sample_format;          // set sample format
        stream_params.suggestedLatency = m_suggested_latency; // set suggested latency
        stream_params.hostApiSpecificStreamInfo = m_host_api_specific_stream_info; // set host specific stream info

        // check if provided format / options is supported
        error = Pa_IsFormatSupported(nullptr, &stream_params, static_cast<double>(m_sample_rate));
        if(error != paNoError)
        {
            enqueue_error("PortAudio::Playback: Provided options results in non supported format for selected device");
            enqueue_error(error);
            return STATUS_FAILURE;
        }

        // open the stream
        error = Pa_OpenStream(&m_stream,                          // pointer to where the stream
                              nullptr,                            // input parameters, not needed, this class does output only
                              &stream_params,                     // output parameters
                              static_cast<double>(m_sample_rate), // sample rate
                              paFramesPerBufferUnspecified,       // frames per buffer, unspecified for varying amounts or unkown
                              paNoFlag,                           // flags, no flags will be passed
                              m_stream_callback,                  // stream callback function
                              m_callback_user_data);              // callback user data, user data pointer that will be passed to the callback function
        
        // check if stream opened successfully
        if(error != paNoError)
        {
            m_stream = nullptr;

            enqueue_error("PortAudio::Playback: Failed to open audio output stream");
            enqueue_error(error);

            return STATUS_FAILURE;
        }

        // if stream finished callback was supplied, set it
        if(m_stream_finished_callback)
        {
            error = Pa_SetStreamFinishedCallback(m_stream, m_stream_finished_callback);
            if(error != paNoError)
            {
                enqueue_error("PortAudio::Playback: Failed to set stream finished callback");
                enqueue_error(error);

                return STATUS_FAILURE;
            }
        }

        return STATUS_SUCCESS;
    }




    /* Playback start_stream() function
     * @desc - starts the audio stream, must be called before any write() calls are made
     * @return - Return_Status::SUCCESS on success, Return_Status::FAILURE on failure
     * @note - when using callbacks this still needs to be called after init()
     */
    Return_Status Playback::start_stream()
    {
        PaError error = 0;

        error = Pa_StartStream(m_stream);
        if(error != paNoError)
        {
            enqueue_error("PortAudio::Playback: Failed to start audio stream");
            enqueue_error(error);

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Playback stop_stream() function
     * @desc - stops an audio stream
     * @return - Return_Status::SUCCESS on success, Return_Status::FAILURE on failure
     * @note - Note needed to call before class instance is deconstructed or options are reset, but reccommended
     */
    Return_Status Playback::stop_stream()
    {
        PaError error = 0;

        error = Pa_StopStream(m_stream);
        if(error != paNoError)
        {
            enqueue_error("PortAudio::Playback: Failed to stop audio stream");
            enqueue_error(error);

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Playback write() function
     * @desc - used to write audio data to the audio stream
     * @prarm data - the audio data to be written
     * @param number_samples - the number of samples contained within @param data
     * @return - Return_Status::SUCCESS on success, Return_Status::FAILURE on failure
     * @note - do not use this function if you are using callbacks to play audio
     */
    Return_Status Playback::write(const void *data, unsigned long number_samples)
    {
        if(!m_stream)
        {
            enqueue_error("PortAudio::Playback: Failed to write audio data, class instance has not been initialized");

            return STATUS_FAILURE;
        }

        PaError error = 0;

        // check if the stream is stopped
        error = Pa_IsStreamStopped(m_stream);
        if(error == 1)
        {
            enqueue_error("PortAudio::Playback: Failed to write audio data, stream has not been started");

            return STATUS_FAILURE;
        }
        
        // some error occurred
        else if(error < 0)
        {
            enqueue_error("PortAudio::Playback: Failed to detect stream status");
            enqueue_error(error);

            return STATUS_FAILURE;
        }

        error = Pa_WriteStream(m_stream, data, number_samples);
        if(error != paNoError)
        {
            enqueue_error("PortAudio::Playback: Failed to write data to audio stream");
            enqueue_error(error);

            return STATUS_FAILURE;
        }

        return STATUS_SUCCESS;
    }




    /* Playback reset_options() function
     * @desc - this function is essentialy a reconstructor, it resets all options
     * @note - in order for new options to apply Playback::init() must be called again
     */
    void Playback::reset_options(PaHostApiIndex host_api_index,                       // Index of host api to use
                                 PaDeviceIndex device_index,                          // Index of device do use
                                 int channels,                                        // Number of output channels
                                 PaSampleFormat sample_format,                        // Sample format, EX paInt16
                                 PaTime suggested_latency,                            // Suggested latency in seconds, PaTime is a double, so 1.0 would be a second, -1.0 for auto set
                                 void* host_api_specific_stream_info,                 // host api specific stream info, nullptr for none
                                 int sample_rate,                                     // Sample rate of audio data, EX: 48000 Hz
                                 PaStreamCallback* stream_callback,                   // Stream callback function, nullptr if not using callbacks
                                 PaStreamFinishedCallback* stream_finished_callback,  // Stream finished callback function, nullptr if not using callbacks
                                 void *callback_user_data)                            // callback user data, client / user supplied pointer which will be passed to the callback function, nullptr if not using callbacks
    {
        m_host_api_index = host_api_index;
        m_device_index = device_index;
        m_channels = channels;
        m_sample_format = sample_format;
        m_suggested_latency = suggested_latency;
        m_host_api_specific_stream_info = host_api_specific_stream_info;
        m_sample_rate = sample_rate;
        m_stream_callback = stream_callback;
        m_stream_finished_callback = stream_finished_callback;
        m_callback_user_data = callback_user_data;
    }



    // BIT OF INFORMATION END //
    // The following functions are getters and setters and do not have comments above them, due to their simplicity
    // Comments might be added in the future if they get more complex
    // NOTE: getters do not start with get
    // NOTE: if you set options using setters, Playback::init() must be called (again) for them to be applied
   



    PaHostApiIndex Playback::host_api_index() const { return m_host_api_index; }
    void Playback::set_host_api_index(PaHostApiIndex new_index) { m_host_api_index = new_index; }

    PaDeviceIndex Playback::device_index() const { return m_device_index; }
    void Playback::set_device_index(PaDeviceIndex new_index) { m_device_index = new_index; }

    int Playback::channels() const { return m_channels; }
    void Playback::set_channels(int channels) { m_channels = channels; }

    PaSampleFormat Playback::sample_format() const { return m_sample_format; }
    void Playback::set_sample_format(PaSampleFormat sample_format) { m_sample_format = sample_format; }

    PaTime Playback::suggested_latency() const { return m_suggested_latency; }
    void Playback::set_suggested_latency(PaTime suggested_latency) { m_suggested_latency = suggested_latency; }

    void *Playback::host_api_specific_stream_info() { return m_host_api_specific_stream_info; }
    const void *Playback::host_api_specific_stream_info() const { return m_host_api_specific_stream_info; }
    void Playback::set_host_api_specific_stream_info(void* host_api_specific_stream_info) { m_host_api_specific_stream_info = host_api_specific_stream_info; }

    PaStreamCallback *Playback::stream_callback() { return m_stream_callback; }
    const PaStreamCallback *Playback::stream_callback() const { return m_stream_callback; }
    void Playback::set_stream_callback(PaStreamCallback *stream_callback) { m_stream_callback = stream_callback; }

    PaStreamFinishedCallback *Playback::stream_finished_callback() { return m_stream_finished_callback; }
    const PaStreamFinishedCallback *Playback::stream_finished_callback() const { return m_stream_finished_callback; }
    void Playback::set_stream_finished_callback(PaStreamFinishedCallback *new_callback) { m_stream_finished_callback = new_callback; }

    void *Playback::callback_user_data() { return m_callback_user_data; }
    const void *Playback::callback_user_data() const { return m_callback_user_data; }
    void Playback::set_callback_user_data(void* callback_user_data) { m_callback_user_data = callback_user_data; }




    /* Playback::output_latency() function
     * @desc - get the latency of the playback stream
     * @return - a positive PaTime on success negative on failure
     */
    PaTime Playback::output_latency()
    {
        if(!m_stream)
        {
            enqueue_error("PortAudio::Playback: Playback not initialized");
            return -1.0;
        }

        const PaStreamInfo *stream_info = Pa_GetStreamInfo(m_stream);

        if(!stream_info)
        {
            enqueue_error("Failed to get stream info");
            return -1.0;
        }

        return stream_info->outputLatency;
    }


    
    /* Playback poll_error function
     * @desc - poll a std::string error message off to m_errors
     * @return - a std::string error message, if m_errors is empty the returned string is empty
     */
    std::string Playback::poll_error()
    {
        if(!m_errors.empty())
        {
            std::string error = m_errors.front();
            m_errors.pop();
            return error;
        }

        return std::string{};
    }




    // Private Functions //




    /* Playback enqueue_error function
     * @desc - enqueues a string error message onto m_errors
     */
    void Playback::enqueue_error(const std::string &message)
    {
        m_errors.push(message);
    }




    /* Playback enqueue_error function
     * @desc - enqueues a string error message for the given error code onto m_errors
     */
    void Playback::enqueue_error(PaError error_code)
    {
        std::string error = Pa_GetErrorText(error_code);
        m_errors.push(error);
    }
}




/* poll_errors function
 * @desc - polls all the errors from a PortAudio::Playback class instance and prints them
 */
void poll_errors(PortAudio::Playback &playback)
{
    std::string error = playback.poll_error();

    while(!error.empty())
    {
        std::cerr << error << std::endl;
        error = playback.poll_error();
    }
}
