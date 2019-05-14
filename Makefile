compile: coarse.o
		gcc coarse.c -pthread

clean:
	rm -rf *.o