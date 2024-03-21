init: init.c
	gcc -o init -pthread init.c
	./init
lib: msocket.c
	gcc -c -o msocket.o msocket.c
	ar rcs libmsocket.a msocket.o
user1: user1.c lib
	gcc -o user1 -pthread user1.c -L. -lmsocket
user2: user2.c lib
	gcc -o user2 -pthread user2.c -L. -lmsocket
user3: user3.c lib
	gcc -o user3 -pthread user3.c -L. -lmsocket
user4: user4.c lib
	gcc -o user4 -pthread user4.c -L. -lmsocket

run: user1 user2
	./user1 &
	./user2

run2: user3 user4
	./user3 &
	./user4
clean:
	rm -f *.o *.a init user1 user2 *.out