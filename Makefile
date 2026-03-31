# Directories (relative to Makefile)
BUILD_DIR = ./build
INCL_DIR = ./build/include/labdev


# Output
TARGET = ./build/liblabdev.so
LIB = -L./build/ \
      -L/usr/local/lib -luics \
      -lpthread \
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
all: SRC $(TARGET)

# Link object files into shared library
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LIB)

# Clean
clean:
	@cd ./src && make clean
	@rm -f $(TARGET)
make_header:
	$(shell which python) ./build/include/makeheader.py
install: $(make_header)
	@cp ./build/liblabdev.so /usr/local/lib/
	@cp ./build/include/*.h /usr/local/include/
	@cp -r ./build/include/labdev/ /usr/local/include/
