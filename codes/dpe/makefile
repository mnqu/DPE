CC = g++
CFLAGS = -lm -pthread -O2 -march=native -Wall -funroll-loops -Wno-unused-result -lgsl -lm -lgslcblas
LFLAGS = -lgsl -lm -lgslcblas
INCLUDES = -I/usr/include -I../../eigen-3.2.5
LIBS = -L/usr/local/lib


dpe : linelib.o main.o
	$(CC) $(CFLAGS) -o dpe linelib.o main.o $(INCLUDES) $(LIBS) $(LFLAGS)

linelib.o : linelib.cpp
	$(CC) $(CFLAGS) -c linelib.cpp $(INCLUDES) $(LIBS) $(LFLAGS)

main.o : main.cpp linelib.o
	$(CC) $(CFLAGS) -c main.cpp $(INCLUDES) $(LIBS) $(LFLAGS)

clean :
	rm -rf *.o dpe
