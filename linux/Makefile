CFLAGS := -c -std=c99 -Wall
CFLAGS += -Iinclude
CFLAGS += $(shell pkg-config --cflags gl)
CFLAGS += $(shell pkg-config --cflags glu)
CFLAGS += $(shell pkg-config --cflags glfw3)

LDFLAGS := -lm
LDFLAGS += $(shell pkg-config --libs gl)
LDFLAGS += $(shell pkg-config --libs glu)
LDFLAGS += $(shell pkg-config --libs glfw3)

CFILES = \
src/data.c \
src/gfxrender-opengl.c \
src/logging.c \
src/main.c \
src/memoryalloc.c \
src/shapes.c \
src/shapesrender.c \
src/window-glfw.c \
src/window.c

OBJECTS = $(CFILES:.c=.o)

all: shapes

clean:
	rm -rf shapes $(OBJECTS)

shapes: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@
