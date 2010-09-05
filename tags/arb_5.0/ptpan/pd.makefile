# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

pd:     probe_debug.o ../PROBE_COM/client.a
        CC -g -o $@ probe_debug.o ../PROBE_COM/client.a

probe_debug.o: probe_debug.cxx ../INCLUDE/PT_com.h
        CC -c -g -o $@ probe_debug.cxx -I../INCLUDE
