# Directories (relative to Makefile)
BUILD_DIR = ./build
INCL_DIR = ./build/include/labdev


# Output
TARGET = ./build/liblabdev.so
LIB = -L./build/ \
      -L/usr/local/lib -luics \
      -lpthread \
      -lsoem \
      -lprometheus-cpp-core \
      -lprometheus-cpp-pull
# Compiler
CC = g++
CXXFLAGS = -fPIC -std=c++17 -shared -I$(INCL_DIR)

SRC:
	@cd ./src && make all


# All object files in the build directory
OBJS = $(wildcard $(BUILD_DIR)/*.o)

# Default target
# The link must run in a *second* make invocation: $(OBJS) is a wildcard over
# build/, and if it were expanded while this makefile is first read (before SRC
# has compiled anything) it would expand to nothing -- and make's directory
# cache would keep returning nothing for the rest of the run, linking an empty
# .so. Re-entering make re-globs build/ once the objects are there.
all: SRC
	@$(MAKE) --no-print-directory $(TARGET)

# Link object files into shared library
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LIB)

# Clean
clean:
	@cd ./src && make clean
	@rm -f $(TARGET)
	@rm -rf ./build/*
install:
	@cp ./build/liblabdev.so /usr/local/lib/
	@cp ./build/include/*.h /usr/local/include/
	@cp -r ./build/include/labdev/ /usr/local/include/
