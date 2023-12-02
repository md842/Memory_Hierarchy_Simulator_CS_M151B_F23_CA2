CC = g++
CPPFLAGS = -Wall -O2 -pipe -fno-plt -fPIC

default: memory_driver

memory_driver:
	$(CC) $(CPPFLAGS) *.cpp -o memory_driver

.PHONY: clean
clean:
	rm -f memory_driver
