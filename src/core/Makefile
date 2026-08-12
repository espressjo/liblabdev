BUILD_DIR = ../../build
LIBINCL = ../../build/include/labdev/
INCL = -I./ \
	-I/usr/local/include
CPPFLAGS = -std=c++17 \
	-fPIC
CC = g++

SRCS = ethernet.cpp \
       serial.cpp \
       ethernet_devices.cpp \
       devicesManager.cpp \
       labdevices.cpp \
       labdev_log.cpp

OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)


$(LIBINCL):
	@mkdir -p $(LIBINCL)


$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp
	$(CC) $(CPPFLAGS) $(INCL) -c $< -o $@

all: $(OBJS) $(LIBINCL) $(BUILD_DIR)
	@cp *.h $(LIBINCL)
	@echo implement python script for creating library .h file 	

clean:
	@rm -f $(OBJS)

