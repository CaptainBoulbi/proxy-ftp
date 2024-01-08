CC=gcc
FLAG=-Wall -Wextra

proxy: proxy.o simpleSocketAPI.o
	${CC} $(FLAG) $^ -o $@ 
proxy.o: proxy.c simpleSocketAPI.h
	${CC} $(FLAG) -c $^ 
simpleSocketAPI.o: simpleSocketAPI.c
	${CC} $(FLAG) -c $^ 

.PHONY: clean
clean:
	rm -f *.o proxy
