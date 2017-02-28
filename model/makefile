CC = g++
CFLAGS = -lm -pthread -Ofast -march=native -Wall -funroll-loops -Wno-unused-result -lgsl -lm -lgslcblas
LFLAGS = -lgsl -lm -lgslcblas
INCLUDES = -I/usr/include -I/home/mengqu2/software/eigen-3.2.5
LIBS = -L/usr/local/lib


embed : ransampl.o linelib.o main.o
	$(CC) $(CFLAGS) -o embed ransampl.o linelib.o main.o $(INCLUDES) $(LIBS) $(LFLAGS)

ransampl.o : ransampl.c
	$(CC) $(CFLAGS) -c ransampl.c $(INCLUDES) $(LIBS) $(LFLAGS)

linelib.o : linelib.cpp ransampl.h
	$(CC) $(CFLAGS) -c linelib.cpp $(INCLUDES) $(LIBS) $(LFLAGS)

main.o : main.cpp linelib.o
	$(CC) $(CFLAGS) -c main.cpp $(INCLUDES) $(LIBS) $(LFLAGS)

clean :
	rm -rf *.o embed
