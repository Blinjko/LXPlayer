# Notice #
I have decided to remake this application, because the original design was just bad. I made things more complicated than needed, causing excess code to be written
and overall bad performance. The new code is simpler and more perfomant that the old code. The code is also very messy, and lacking comments, I will fix this
by cleaning up code and adding comments where needed. The new code I have written is closely knit together, and purpose built just for this application, but I think
it is still very simple and easy to understand for anyone who wants to read it.

# Description #
A Audio / Video player that uses all software decoding, and the following libraries, FFmpeg, SDL2, PortAudio. 

# Current State #
I have copied the new code and added comments where needed, the main.cpp source file is still messy and I have plans to clean it up. The LXPlayer "works", it plays a video file, no audio yet, just video. It can be interrupted with Ctrl + c
to kill the video player. There is no pause / fullscreen support yet, however there are plans to add it. 

# To Do #
1. Remove Utility code from main.cpp and put it into utility source files
2. Create an image rescaler class
3. Create an Audio resampler class
4. Implement Audio playback
5. Create a thread that listens for input, so video can be fullscreened, paused and played
6. Add a conversion table between AVPixelFormat and SDL PixelFormatEnum to determine wheather image rescaling is necessary or not

# Libraries Used #
[SDL2](https://libsdl.org/)  
[FFmpeg](https://ffmpeg.org/)  
[PortAudio](http://portaudio.com/)
