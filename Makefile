# ─────────────────────────────────────────────────────────────
#  Makefile for DOOM3D
#  Supports: Windows (MinGW / MSYS2) and Linux
# ─────────────────────────────────────────────────────────────

TARGET    = doom3d

CC        = gcc
CFLAGS    = -std=c99 -Wall -Wextra -O2 -Iinclude
SRCDIR    = src
OBJDIR    = obj

# All .c files in src/
SRCS      = $(wildcard $(SRCDIR)/*.c)
OBJS      = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# ── Detect OS ────────────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    # Windows / MinGW
    LIBS    = -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lm
    CFLAGS += -DWIN32 -D_WIN32
    TARGET := $(TARGET).exe
else
    # Linux
    LIBS    = $(shell sdl2-config --libs) -lGL -lGLU -lm
    CFLAGS += $(shell sdl2-config --cflags)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
