CC = g++
CFLAGS = -Wall -Wextra -O2
TARGET = vmm
OBJS = vmm.o process_file.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

vmm.o: vmm.cpp process_file.hpp
	$(CC) $(CFLAGS) -c vmm.cpp -o vmm.o

process_file.o: process_file.cpp process_file.hpp
	$(CC) $(CFLAGS) -c process_file.cpp -o process_file.o

clean:
	rm -f $(TARGET) $(OBJS)
