CC       = gcc
CXX      = g++
CFLAGS   = -Iinclude -Wall -O2
CXXFLAGS = -Iinclude -Wall -O2 -std=c++14

# Todos los .c excepto chorus.c (reemplazado por .cpp)
C_SRC   = $(filter-out src/chorus.c, $(wildcard src/*.c))
CPP_SRC = src/chorus.cpp

C_OBJ   = $(C_SRC:.c=.o)
CPP_OBJ = $(CPP_SRC:.cpp=.o)

OUT = audio_engine

all: $(C_OBJ) $(CPP_OBJ)
	$(CXX) -o $(OUT) $^ -lm -lasound -lsndfile

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OUT) src/*.o