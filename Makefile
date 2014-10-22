
all: run

apihook: apihook.o main.o
	sh4-linux-gcc apihook.o main.o -o apihook

apihook.o: apihook.c
	sh4-linux-gcc -c apihook.c -o apihook.o -O2

main.o: main.c apihook.h
	sh4-linux-gcc -c main.c -o main.o -O2

apihook.txt: apihook
	sh4-linux-objdump -D apihook > apihook.txt

run: apihook.txt
	qemu-sh4 -L /stmroot apihook
