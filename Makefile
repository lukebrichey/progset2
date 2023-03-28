CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -I/usr/include/eigen3 -lpthread -lstdc++ -lpthread -lm

all: strassen testfiles

strassen: strassen.cpp
	$(CC) $(CFLAGS) -o strassen strassen.cpp $(LIBS)

testfiles: testmaker.cpp
	g++ -o testmaker testmaker.cpp -std=c++17  

debug:
	$(CC) $(CFLAGS) -g -o strassen strassen.cpp $(LIBS)
	
clean:
	rm -f strassen testmaker
