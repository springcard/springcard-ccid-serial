#
# Springcard ccid-serial SDK - Makefile for Win32, with MinGW+GCC as toolchain
# ----------------------------------------------------------------------------
#
# Requirements:
# - GCC (of course)
# - GNU utilities (make, mkdir...)
# - MinGW to run all the above
#
# If you don't already have a working MinGW+GCC setup on your com:
# - https://code.visualstudio.com/docs/cpp/config-mingw
# - https://earthly.dev/blog/vscode-make/
#
# To build the sample, just open a shell in the directory containing the Makefile,
# and enter 'make'
#
# You can run the generated program using 'bin\ccid-serial.exe'
# Use 'bin\ccid-serial.exe -h' to read the usage message.
#
# If you have Visual Studio 2022 installed, consider using win32_vs2022 instead!
#

# Directory where all the source files are
SOURCE_DIR:=../../src
# Directory for objects
OBJECT_DIR:=./obj
# Directory for the program
OUTPUT_DIR:=./bin

# Name of the program
PROGRAM:=$(OUTPUT_DIR)/ccid-serial.exe

# We use GCC for compiling and linking
CC:=gcc
# All warnings enabled, stop on warning
CFLAGS:=-Wall -Wextra -Werror
# The source will be looking for <project.h> that is in the current directory
CINCL:=-I.

# Select all the sources
SOURCES:=\
	$(wildcard $(SOURCE_DIR)/ccid/*.c) \
	$(wildcard $(SOURCE_DIR)/scard/*.c) \
	$(wildcard $(SOURCE_DIR)/hal/win32/*.c) \
	$(wildcard $(SOURCE_DIR)/sample/*.c) \
	$(wildcard $(SOURCE_DIR)/sample/pc/*.c)

# Make objects from sources
OBJECTS:=$(patsubst %c,%o,$(SOURCES))
OBJECTS:=$(subst $(SOURCE_DIR),$(OBJECT_DIR),$(OBJECTS))

# Build the program
all: $(PROGRAM)

# Rule to link the program
$(PROGRAM): $(OBJECTS) | $(OUTPUT_DIR)
	$(CC) -o $@ $^

# Rule to compile an object from a source file
$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.c | $(OBJECT_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CINCL) -c -o $@ $<

# Make sure we have the output directory
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Make sure we have the object directory
$(OBJECT_DIR):
	mkdir -p $(OBJECT_DIR)

# Clean the objects and the program
.PHONY: clean
clean: 
	rm $(OBJECTS) $(PROGRAM)