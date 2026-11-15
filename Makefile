all: banker

banker: banker.c
	gcc -Wall -Wextra -g -o banker banker.c

clean:
	rm -f banker *.o