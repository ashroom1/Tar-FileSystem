all:
	gcc -Wall TarRead.c `pkg-config fuse3 --cflags --libs` -o tarmount -lm
