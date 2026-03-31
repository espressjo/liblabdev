BUILD_DIR = ../../../build
LIBINCL = ../../../build/include/labdev
INCL = -I./ \
	-I../../core/ \
	-I/usr/local/include
CPPFLAGS = -std=c++17 \
	-fPIC
CC = g++

SRCS = lakeshore336.cpp

OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

$(LIBINCL):
	@mkdir -p $(LIBINCL)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp
	$(CC) $(CPPFLAGS) $(INCL) -c $< -o $@

all: $(OBJS) $(LIBINCL)
	@cp *.h $(LIBINCL)/

clean:
	@rm -f $(OBJS)

