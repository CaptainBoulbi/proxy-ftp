CC=gcc
FLAG=-Wall -Wextra -ggdb

proxy: proxy.o simpleSocketAPI.o
	${CC} $(FLAG) $^ -o $@ 
proxy.o: proxy.c
	${CC} $(FLAG) -c $^ -I.
simpleSocketAPI.o: simpleSocketAPI.c
	${CC} $(FLAG) -c $^ 

run: proxy
	./proxy

clean:
	rm -f *.o proxy

.PHONY: run clean
