# Description #
LXPlayer is a command line audio / video player.   
AudioPlayer is a command line audio player.  
This is just a fun side project I did, so this will not be recieving any updates or fixes.

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
There are two programs to build, **LXPlayer**, and **AudioPlayer**. To build the programs use **make** with the name of the program you would like to build. 

# Usage #
## LXPlayer ##
A general usage example would be ```LXPlayer file1.mp4 file2.mkv ...```, this would play file1 and file2 in the specified order.  
Other options:  
1. ```--shuffle``` Shuffles the files passed to LXPlayer
2. ```--audio-only``` Just plays audio, no video
3. ```--video-only``` Just plays video, no audio
4. ```--help``` Displays a help message  

If video is being played, the video & audio can be paused / unpaused by pressing **space**, the player can be exited with **q**, the current video can be skipped with **n**, and to go-to the previous video press **p**.  
If just audio is being played, then the program will read commands from stdin, the commands are:  
1. ```pause```
2. ```play```
3. ```next```, skips to the next file
4. ```prev```, skips to the previous file
5. ```exit```

## AudioPlayer ##
A general usage example: ```AudioPlayer song1.wav song2.au song3.ogg ...```, this would play the files in the specified order.  
Other options:  
1. ```--shuffle``` Shuffles the given files

While audio is playing the program will read commands from stdin, the commands are:  
1. ```pause```
2. ```play```
3. ```next```, skips to the next file
4. ```prev```, skips to the previous file
5. ```exit```

# Supported Formats #
Almost every format that FFmpeg can decode is supported.

# Libraries Used #
[SDL2](https://libsdl.org/)  
[FFmpeg](https://ffmpeg.org/)  
[PortAudio](http://portaudio.com/)
