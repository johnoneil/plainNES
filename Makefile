# Compiler
CC = g++
OPTS = -Wall -g
#-fno-exceptions may remove requirement for libstdc++-6.dll

# Project name
PROJECT = NESEmu

# Directories
BINDIR = bin
OBJDIR = build
SRCDIR = src

# Libraries
LINKER_FLAGS =

# Files and folders
#SRC_FILES = main.cpp gamepak.cpp cpu.cpp
#SRCS = $(addprefix $(SRCDIR)/,$(SRC_FILES))
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: build $(BINDIR)/$(PROJECT)

$(BINDIR)/$(PROJECT): $(OBJS)
	$(CC) $(OPTS) -o $@ $(OBJS)

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CC) $(OPTS) $(LINKER_FLAGS) -c $< -o $@

build:
	mkdir $(OBJDIR)

.PHONY: clean test

clean:
	rmdir $(OBJDIR) /S /Q