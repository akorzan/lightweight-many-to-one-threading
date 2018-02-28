all:
	gcc -g -O3 -o main main.c lwt_asm.S
unop:
	gcc -g -o main main.c lwt_asm.S
test:
	gcc -g -O3 -o test test_main.c lwt_asm.S
assembly:
	gcc -S -O3 -o main.S main.c
clean:
	rm main test
