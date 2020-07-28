# Description #
An audio and video player using SDL, FFMpeg and PortAudio

# Current State #
LXPlayer currently is just a video player, it only plays video, however audio will be added soon. If you would like just audio but no video, there is the 
AudioPlayer program. The source code for AudioPlayer is included in this repository, and can be built by following the instructions under the Building section.

# Dependencies #
1. FFmpeg libavformat
2. FFmpeg libavutil
3. FFmpeg libavcodec
4. FFmpeg libswscale
5. FFmpeg libswresample
6. SDL2 library
7. PortAudio library
8. G++ / C++ compiler
9. GNU make

# Building #
Currently there are two complete programs to build, **LXPlayer**, and **AudioPlayer**. Currently LXPlayer is just video playback, and no audio. AudioPlayer is
only audio playback and no video.
To build **LXPlayer** execute the following ```make LXPlayer```  
To build **AudioPlayer** execute the following ```make AudioPlayer```

# Libraries Used #
[SDL2](https://libsdl.org/)  
[FFmpeg](https://ffmpeg.org/)  
[PortAudio](http://portaudio.com/)
