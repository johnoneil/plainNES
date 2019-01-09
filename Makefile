# Compiler
CC = g++
OPTS = -Wall -g
#-O3
#-pg

# Project name
PROJECT = plainNES

# Directories
BINDIR = bin
OBJDIR = build
SRCDIR = src

# Include Paths
INCLUDE_PATHS = -IC:\dev_libs\SDL2\include\SDL2 -IC:/dev_libs/boost/include/boost-1_69

# Link Paths
LINK_LIBRARIES = -LC:\dev_libs\boost\lib

# LINKER_FLAGS
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lboost_chrono-mgw63-mt-x32-1_69 -lboost_thread-mgw63-mt-x32-1_69

# Files and folders
#SRC_FILES = main.cpp gamepak.cpp cpu.cpp
#SRCS = $(addprefix $(SRCDIR)/,$(SRC_FILES))
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: $(OBJDIR) $(BINDIR) $(BINDIR)/$(PROJECT)

$(BINDIR)/$(PROJECT): $(OBJS)
	$(CC) $(OBJS) $(LINK_LIBRARIES) $(LINKER_FLAGS) $(OPTS) -o $@

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CC) $< -c $(OPTS) $(INCLUDE_PATHS) -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: clean

clean:
	rmdir $(OBJDIR) /S /Q