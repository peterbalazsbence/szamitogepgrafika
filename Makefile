# ─────────────────────────────────────────────────────────────
#  Makefile for DOOM3D
#  Supports: Windows (MinGW / MSYS2) and Linux
# ─────────────────────────────────────────────────────────────

TARGET  = doom3d

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2
SRCS    = main.c

# ── Detect OS ────────────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    # Windows / MinGW
    LIBS    = -lSDL2main -lSDL2 -lopengl32 -lglu32 -lm
    CFLAGS += -DWIN32 -D_WIN32
    TARGET := $(TARGET).exe
else
    # Linux
    LIBS    = $(shell sdl2-config --libs) -lGL -lGLU -lm
    CFLAGS += $(shell sdl2-config --cflags)
endif

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean