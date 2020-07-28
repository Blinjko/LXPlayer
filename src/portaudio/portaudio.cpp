#include <portaudio/portaudio.h>
#include <iostream>
#include <cstdlib>

namespace PortAudio
{

    // PortAudio initialzer structure
    Initializer::Initializer()
    {
        PaError error{0};

        error = Pa_Initialize();

        if(error < 0)
        {
            std::cerr << "Failed to initialize PortAudio" << std::endl;
            std::exit(1);
        }
    }


    Initializer::~Initializer()
    {
        PaError error{0};

        error = Pa_Terminate();

        if(error < 0)
        {
            std::cerr << "Failed to terminate PortAudio" << std::endl;
            std::exit(1);
        }
    }


    // Stream_Playback constructor
    Stream_Playback::Stream_Playback() : 
        m_host_api{-1}, m_device{-1}, m_stream{nullptr},
        m_channel_count{0}, m_sample_format{0}, m_suggested_latency{0.0},
        m_sample_rate{0}, m_stream_stopped{true}
    {}


    // Stream_Playback destructor
    Stream_Playback::~Stream_Playback()
    {
        if(m_stream)
        {
            // PortAudio documentation says these functions return PaError types,
            // however it doesn't say they return errors? Wouldn't make much sense why they would, so they are not error checked.
            if(!m_stream_stopped)
            {
                Pa_StopStream(m_stream);
            }

            Pa_CloseStream(m_stream);
        }
    }


    /* set_host_api_index function
     * Description: Set the host api index for portaudio to use, -1 = default host api
     * Parameter host_api - the host api to use, or -1 for default host api
     * Return: a negative PaError on failure, positive PaHostApiIndex on success
     */
    PaError Stream_Playback::set_host_api_index(PaHostApiIndex host_api)
    {
        if(host_api == -1)
        {
            host_api = Pa_GetDefaultHostApi();
            if(host_api < 0)
            {
                return host_api;
            }
        }

        m_host_api = host_api;

        return m_host_api;
    }


    /* set_device_index function
     * Description: Set the device index for portaudio to use, -1 = default device
     * Parameter device - the device to use, or -1 for default host api
     * Return: a negative PaError on failure, positive PaDeviceIndex on success
     */
    PaError Stream_Playback::set_device_index(PaDeviceIndex device)
    {
        if(device == -1)
        {
            device = Pa_GetDefaultOutputDevice();
            if(device < 0)
            {
                return device;
            }
        }

        m_device = device;

        return m_device;
    }


    /* open_stream function
     * Description: opens a stream for audio playback
     * Parameter channel_count - number of channels for audio playback
     * Parameter sample_format - the sample format of the audio
     * Parameter suggested_latency - the suggested latency to use -1 for default
     * Parameter sample_rate - the sample rate of the audio
     * Parameter flags - PortAudio stream flags
     * Return: positive value on success, negative value on failure
     */
    PaError Stream_Playback::open_stream(int channel_count, PaSampleFormat sample_format, PaTime suggested_latency, int sample_rate, PaStreamFlags flags)
    {
        if(suggested_latency == -1)
        {
            // get device info
            const PaDeviceInfo *device_info{Pa_GetDeviceInfo(m_device)};

            if(!device_info)
            {
                return paInvalidDevice;
            }

            // get the suggested latency
            m_suggested_latency = device_info->defaultHighOutputLatency;
        }

        else
        {
            m_suggested_latency = suggested_latency;
        }

        // set member variables
        m_channel_count = channel_count;
        m_sample_format = sample_format;
        m_sample_rate = sample_rate;

        // setup stream parameters struct
        PaStreamParameters stream_parameters;
        stream_parameters.device = m_device;                     // set device to use
        stream_parameters.channelCount = m_channel_count;        // set output channel count
        stream_parameters.sampleFormat = m_sample_format;        // set output sample format
        stream_parameters.suggestedLatency = m_suggested_latency;// set output suggested latency
        stream_parameters.hostApiSpecificStreamInfo = nullptr;   // set host secific apt info to nothing

        PaError error{0};

        // check if the provided parameters are supported
        error = Pa_IsFormatSupported(nullptr, &stream_parameters, static_cast<double>(sample_rate));
        if(error != paFormatIsSupported)
        {
            return error;
        }

        // open the stream
        error = Pa_OpenStream(&m_stream,                        // pointer to where the newly made stream will be stored
                              nullptr,                          // input parameters
                              &stream_parameters,               // output parameters
                              static_cast<double>(sample_rate), // sample rate
                              0,                                // frames per buffer, 0 for unknown / default
                              flags,                            // flags
                              nullptr,                          // callback, not needed
                              nullptr);                         // user data, not needed

        return error;
    }

    
    /* start_stream function
     * Description: starts the playback stream if stopped
     * Return: a negative error if the stream is already started or the stream failed to start, positive value on success
     */
    PaError Stream_Playback::start_stream()
    {
        if(!m_stream_stopped)
        {
            return paStreamIsNotStopped;
        }

        PaError error{0};

        error = Pa_StartStream(m_stream);

        if(error < 0)
        {
            return error;
        }

        m_stream_stopped = false;

        return error;
    }


    /* stop_stream function
     * Description: stops the playback stream if stream is started
     * Return: a negative error if the stream is already stopped, or failure to stop the stream, positive value on success
     */
    PaError Stream_Playback::stop_stream()
    {
        if(m_stream_stopped)
        {
            return paStreamIsStopped;
        }

        PaError error{0};

        error = Pa_StopStream(m_stream);

        if(error < 0)
        {
            return error;
        }

        m_stream_stopped = true;

        return error;

    }


    /* write function
     * Description: writes audio data to the opened stream
     * Parameter: data - pointer to audio data
     * Parameter: number_samples - the number of samples contained in the audio data
     * Return: a positive value on success, negative error on failure
     */
    PaError Stream_Playback::write(const void *data, unsigned long number_samples)
    {
        PaError error{0};

        error = Pa_WriteStream(m_stream, data, number_samples);

        return error;
    }


    /* reset function
     * Description: deallocates any allocated memory, closes streams, and resets all members to default values
     * Return: a negative PaError on failure, positive value on success
     */
    PaError Stream_Playback::reset()
    {
        PaError error{0};

        if(m_stream)
        {
            if(!m_stream_stopped)
            {
                error = Pa_StopStream(m_stream);

                if(error < 0)
                {
                    return error;
                }
            }

            error = Pa_CloseStream(m_stream);
        }

        m_host_api = -1;
        m_device = -1;
        m_stream = nullptr;
        m_channel_count = -1;
        m_sample_format = 0;
        m_suggested_latency = -1.0;
        m_stream_stopped = true;

        return error;
    }


    /* acutal_latency function
     * Description: gets the actual latency of the stream
     * Return: a postive value on success, negative one on failure
     */
    PaTime Stream_Playback::acutal_latency()
    {
        const PaStreamInfo *stream_info{Pa_GetStreamInfo(m_stream)};

        if(!stream_info)
        {
            return -1.0;
        }

        return stream_info->outputLatency;
    }


    // getters //

    bool Stream_Playback::steam_stopped() const { return m_stream_stopped; }
    int Stream_Playback::channel_count() const { return m_channel_count; }
    PaSampleFormat Stream_Playback::sample_format() const { return m_sample_format; }
    PaTime Stream_Playback::suggested_latency() const { return m_suggested_latency; }
    int Stream_Playback::sample_rate() const { return m_sample_rate; }

    PaStream *Stream_Playback::stream() { return m_stream; }
    const PaStream *Stream_Playback::stream() const { return m_stream; }

}
