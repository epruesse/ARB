# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

nd:	names_debug.o
	acc -g -o $@ $? ../NAMES_COM/client.a

names_debug.o: names_debug.c ../INCLUDE/names_client.h
	acc -c -g -o $@ $*.c -I../INCLUDE
