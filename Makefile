all: myls

myls: myls.c 
	gcc -o myls myls.c

clean:
	rm -f myls *.o