all: Myserv

Myserv: main.c
	gcc -W -Wall -o Myserv main.c -lpthread

clean:
	rm Myserv