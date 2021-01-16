#makefile DS - COVID 
#Luca Rastrelli 564654
#
all: ds peer
#
ds: ds.o
	gcc -o ds ds.o
#
peer: peer.o
	gcc -o peer peer.o
#
ds.o: ds.c
	gcc -W -c ds.c
#
peer.o: peer.c
	gcc -W -c peer.c
