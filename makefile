
FFMPEG_SRC_DIR = src/ffmpeg/
FFMPEG_INCLUDE_DIR = include/ffmpeg/

PORTAUDIO_SRC_DIR = src/portaudio/
PORTAUDIO_INCLUDE_DIR = include/portaudio/

SDL_SRC_DIR = src/sdl/
SDL_INCLUDE_DIR = include/sdl/

CXX = g++
CXXFLAGS = -Wextra -Wall -Wpedantic -Iinclude/

OBJECT_FILES = decoder.o resampler.o color_converter.o playback.o

all: $(OBJECT_FILES)

decoder.o: $(FFMPEG_SRC_DIR)decoder.cpp $(FFMPEG_INCLUDE_DIR)decoder.h
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)decoder.cpp

resampler.o: $(FFMPEG_SRC_DIR)resampler.cpp $(FFMPEG_INCLUDE_DIR)resampler.h
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)resampler.cpp

color_converter.o: $(FFMPEG_SRC_DIR)color_converter.cpp $(FFMPEG_INCLUDE_DIR)color_converter.h
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)color_converter.cpp

playback.o: $(PORTAUDIO_SRC_DIR)playback.cpp $(PORTAUDIO_INCLUDE_DIR)playback.h
	$(CXX) $(CXXFLAGS) -c $(PORTAUDIO_SRC_DIR)playback.cpp

clean:
	rm *.o
