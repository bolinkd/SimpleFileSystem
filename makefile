all:
	gcc -Wall -DPART1 parts.c -o diskinfo
	gcc -Wall -DPART2 parts.c -o disklist
	gcc -Wall -DPART3 parts.c -o diskget
	gcc -Wall -DPART4 parts.c -o diskput
clean:
	-rm diskinfo disklist diskget diskput