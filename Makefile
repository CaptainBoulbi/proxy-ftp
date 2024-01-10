CC=gcc
FLAG=-Wall -Wextra

proxy: proxy.o simpleSocketAPI.o
	${CC} $(FLAG) $^ -o $@ 
proxy.o: proxy.c simpleSocketAPI.h
	${CC} $(FLAG) -c $^ 
simpleSocketAPI.o: simpleSocketAPI.c
	${CC} $(FLAG) -c $^ 

run: proxy
	./proxy

clean:
	rm -f *.o proxy

.PHONY: run clean
