CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -I/usr/include/eigen3 -lpthread -lstdc++ -lpthread -lm
CXXFLAGS = -Iexternal/eigen

all: strassen testfiles

strassen: strassen.cpp
	$(CC) $(CXXFLAGS) $(CFLAGS) -o strassen strassen.cpp $(LIBS)

testfiles: testmaker.cpp
	g++ -o testmaker testmaker.cpp -std=c++17  

clean:
	rm -f strassen testmaker
