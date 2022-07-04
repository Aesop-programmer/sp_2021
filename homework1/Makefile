.PHONY: clean

all:write_server read_server

write_server: server.c
	gcc server.c -D WRITE_SERVER -o write_server -Wall
read_server: server.c
	gcc server.c -D READ_SERVER -o read_server -Wall

clean:
	rm -f read_server write_server