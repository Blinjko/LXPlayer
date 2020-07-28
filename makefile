INCLUDE_FLAGS = -Iinclude/
TOTAL_OBJECTS = decoder.o frame.o sdl.o portaudio.o semaphore.o scale.o resample.o utility.o main.o

FFMPEG_INCLUDE_DIR = include/ffmpeg/
FFMPEG_SRC_DIR = src/ffmpeg/

SDL_INCLUDE_DIR = include/sdl/
SDL_SRC_DIR = src/sdl/

PORTAUDIO_INCLUDE_DIR = include/portaudio/
PORTAUDIO_SRC_DIR = src/portaudio/

UTILITY_INCLUDE_DIR = include/utility/
UTILITY_SRC_DIR = src/utility/

PLAYER_SRC_DIR = src/player/

CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic $(INCLUDE_FLAGS) -g
OUTPUT_FLAGS = -o $(OBJECT_OUTPUT_DIR)
LIBS = -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2 -lportaudio -lpthread

LXPlayer: $(TOTAL_OBJECTS)
	$(CXX) $(TOTAL_OBJECTS) $(LIBS) -o LXPlayer

# sdl.o is just needed for utility.o, SDL is not actually used anywhere in AudioPlayer
AudioPlayer: $(PLAYER_SRC_DIR)audio-player.cpp decoder.o frame.o portaudio.o resample.o utility.o sdl.o
	$(CXX) $(CXXFLAGS) $(PLAYER_SRC_DIR)audio-player.cpp decoder.o frame.o portaudio.o resample.o utility.o sdl.o -o AudioPlayer $(LIBS)

decoder.o: $(FFMPEG_INCLUDE_DIR)decoder.h $(FFMPEG_SRC_DIR)decoder.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)decoder.cpp 

frame.o: $(FFMPEG_INCLUDE_DIR)frame.h $(FFMPEG_SRC_DIR)frame.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)frame.cpp 

scale.o: $(FFMPEG_INCLUDE_DIR)scale.h $(FFMPEG_SRC_DIR)scale.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)scale.cpp 

resample.o: $(FFMPEG_INCLUDE_DIR)resample.h $(FFMPEG_SRC_DIR)resample.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)resample.cpp 

sdl.o: $(SDL_INCLUDE_DIR)sdl.h $(SDL_SRC_DIR)sdl.cpp
	$(CXX) $(CXXFLAGS) -c $(SDL_SRC_DIR)sdl.cpp 

portaudio.o: $(PORTAUDIO_INCLUDE_DIR)portaudio.h $(PORTAUDIO_SRC_DIR)portaudio.cpp
	$(CXX) $(CXXFLAGS) -c $(PORTAUDIO_SRC_DIR)portaudio.cpp

semaphore.o: $(UTILITY_INCLUDE_DIR)semaphore.h $(UTILITY_SRC_DIR)semaphore.cpp
	$(CXX) $(CXXFLAGS) -c $(UTILITY_SRC_DIR)semaphore.cpp 

utility.o: $(UTILITY_INCLUDE_DIR)utility.h $(UTILITY_SRC_DIR)utility.cpp
	$(CXX) $(CXXFLAGS) -c $(UTILITY_SRC_DIR)utility.cpp 

main.o: $(FFMPEG_INCLUDE_DIR)decoder.h $(FFMPEG_INCLUDE_DIR)frame.h $(SDL_INCLUDE_DIR)sdl.h $(UTILITY_INCLUDE_DIR)semaphore.h $(PLAYER_SRC_DIR)main.cpp
	$(CXX) $(CXXFLAGS) -c $(PLAYER_SRC_DIR)main.cpp 

clean:
	rm *.o
