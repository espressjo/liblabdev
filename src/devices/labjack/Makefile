
CC = g++
CPPFLAGS += -std=c++17 
LIBS = /usr/local/lib -lLabJackM -llabdev
INCL = /usr/local/include

OBJS = labjack.o

%.o: %.cpp
	$(CC) $(CPPFLAGS) $(INCL) -c $< -o $@

all: $(OBJS)
	$(CC) $(CPPFLAGS) $(INCL) $(LIBS) $(OBJS) -o $(TARGET)
clean:
	@rm -f $(OBJS) $(TARGET)
