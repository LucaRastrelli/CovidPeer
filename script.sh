#!/bin/sh
echo "Progetto Reti Informatiche A.A. 2020-2021"
echo "Luca Rastrelli 564654"
make -f makefile
gnome-terminal -- ./peer 4343
gnome-terminal -- ./peer 4444
gnome-terminal -- ./peer 4443
gnome-terminal -- ./peer 5000
gnome-terminal -- ./peer 4768
./ds 4242