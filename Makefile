all:
	gcc -c LinkedList.c
	gcc LinkedList.o Account.c -o Account
clean:
	rm Account
	rm LinkedList.o
