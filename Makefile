CC = g++
CFLAGS = -Wall -Wextra -O2
TARGET = vmm

all: $(TARGET)

$(TARGET): vmm.cpp
	$(CC) $(CFLAGS) vmm.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
