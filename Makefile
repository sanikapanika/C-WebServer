webserver: main.o
	g++ main.o -o webserver

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm *.o

full-clean:
	rm -rf *.o server webserver