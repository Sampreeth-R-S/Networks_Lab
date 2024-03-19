debug_init:
	gcc -g -fsanitize=address -pthread -o init init.c -DDEBUG
	./init

init:
	gcc -pthread -o init init.c
	./init

debug_lib:
	gcc -c -o msocket.o msocket.c -DDEBUG -g -fsanitize=address
	ar rcs libmsocket.a msocket.o

libmsocket.a:
	gcc -c -o msocket.o msocket.c
	ar rcs libmsocket.a msocket.o

debug_user: debug_lib
	gcc -g -fsanitize=address -pthread -o user1 user1.c -L. -lmsocket -DDEBUG
	gcc -g -fsanitize=address -pthread -o user2 user2.c -L. -lmsocket -DDEBUG
	xterm ./user1 &
	xterm ./user2 &

user: libmsocket.a
	gcc -pthread -o user1 user1.c -L. -lmsocket
	gcc -pthread -o user2 user2.c -L. -lmsocket
	xterm ./user1 &
	xterm ./user2 &

clean:
	rm -f *.o *.a init user1 user2 