INCLUDE_FLAGS = -Iinclude/
OBJECT_OUTPUT_DIR = object-files/
TOTAL_OBJECTS = decoder.o frame.o sdl.o semaphore.o main.o

FFMPEG_INCLUDE_DIR = include/ffmpeg/
FFMPEG_SRC_DIR = src/ffmpeg/

SDL_INCLUDE_DIR = include/sdl/
SDL_SRC_DIR = src/sdl/

UTILITY_INCLUDE_DIR = include/utility/
UTILITY_SRC_DIR = src/utility/

PLAYER_SRC_DIR = src/player/

CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic $(INCLUDE_FLAGS) -g
OUTPUT_FLAGS = -o $(OBJECT_OUTPUT_DIR)
LIBS = -lavformat -lavcodec -lavutil -lSDL2 -lpthread

LXPlayer: $(TOTAL_OBJECTS)
	$(CXX) $(OBJECT_OUTPUT_DIR)*.o $(LIBS) -o LXPlayer

decoder.o: $(FFMPEG_INCLUDE_DIR)decoder.h $(FFMPEG_SRC_DIR)decoder.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)decoder.cpp $(OUTPUT_FLAGS)decoder.o

frame.o: $(FFMPEG_INCLUDE_DIR)frame.h $(FFMPEG_SRC_DIR)frame.cpp
	$(CXX) $(CXXFLAGS) -c $(FFMPEG_SRC_DIR)frame.cpp $(OUTPUT_FLAGS)frame.o

sdl.o: $(SDL_INCLUDE_DIR)sdl.h $(SDL_SRC_DIR)sdl.cpp
	$(CXX) $(CXXFLAGS) -c $(SDL_SRC_DIR)sdl.cpp $(OUTPUT_FLAGS)sdl.o

semaphore.o: $(UTILITY_INCLUDE_DIR)semaphore.h $(UTILITY_SRC_DIR)semaphore.cpp
	$(CXX) $(CXXFLAGS) -c $(UTILITY_SRC_DIR)semaphore.cpp $(OUTPUT_FLAGS)semaphore.o

main.o: $(FFMPEG_INCLUDE_DIR)decoder.h $(FFMPEG_INCLUDE_DIR)frame.h $(SDL_INCLUDE_DIR)sdl.h $(UTILITY_INCLUDE_DIR)semaphore.h $(PLAYER_SRC_DIR)main.cpp
	$(CXX) $(CXXFLAGS) -c $(PLAYER_SRC_DIR)main.cpp $(OUTPUT_FLAGS)main.o

clean:
	rm $(OBJECT_OUTPUT_DIR)*.o
