all:
	gcc -Wno-discarded-qualifiers cshell.c -o cshell
clean:
	rm cshell
