
FFMPEG_SRC = src/ffmpeg/
FFMPEG_INCLUDE = include/ffmpeg/
SDL_SRC = src/sdl/
SDL_INCLUDE = include/sdl/

CXX = g++
CXXFLAGS = -Wextra -Wall -Wpedantic -Iinclude/

all: $(wildcard *.o)
	@echo "Built all"

color_converter.o: $(FFMPEG_SRC)color_converter.cpp $(FFMPEG_INCLUDE)color_converter.h
	g++ -c $(CXXFLAGS) $(FFMPEG_SRC)color_converter.cpp

decoder.o: $(FFMPEG_SRC)decoder.cpp $(FFMPEG_INCLUDE)decoder.h
	g++ -c $(CXXFLAGS) $(FFMPEG_SRC)decoder.cpp

resampler.o: $(FFMPEG_SRC)resampler.cpp $(FFMPEG_INCLUDE)resampler.h
	g++ -c $(CXXFLAGS) $(FFMPEG_SRC)resampler.cpp

clean:
	rm *.o
