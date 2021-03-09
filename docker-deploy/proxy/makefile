cc = g++
cflags = -g -pthread -std=c++1z -Iinclude # -Wall 
deps = include/util.h include/exception.h

main = src/main.cpp
sources = src/proxy.cpp src/parser.cpp src/cache.cpp  # $(wildcard src/*.cpp)
objects = $(patsubst %.cpp,build/%.o,$(notdir ${sources}))

build/%.o: src/%.cpp include/%.h $(deps) | build
	$(cc) $(cflags) -o $@ -c $< 

all: $(objects)
	$(cc) $(cflags) -o proxy $(main) $^

build:
	mkdir $@

cache: build/cache.o

.PHONY: clean
clean:
	rm -r build
