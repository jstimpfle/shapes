# Makefile for use with emscripten

# NOTE: you need to download emscripten SDK and adapt path to compiler!
CC = /tmp/emsdk/upstream/emscripten/emcc

CFLAGS := -std=c99 -Wall
CFLAGS += -Iinclude

LDFLAGS =
# It seems that these dependencies do not need to be explicitly linked
#LDFLAGS += -lm -lGL -lGLU -lglfw
LDFLAGS += -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2   # Target WebGL2 (which is roughly OpenGL ES 3)

CFILES = \
src/data.c \
src/gfxrender-opengl.c \
src/logging.c \
src/main.c \
src/memoryalloc.c \
src/shapes.c \
src/shapesrender.c \
src/window-glfw-emscripten.c \
src/window.c

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: shapes.html

clean:
	rm -rf shapes $(OBJECTS)

shapes.html: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

BUILD/src/%.o: src/%.c BUILD/src
	$(CC) -c $(CFLAGS) $< -o $@

BUILD/src:
	mkdir -p BUILD/src
