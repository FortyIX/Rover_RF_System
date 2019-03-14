all:
	g++ -lwiringPi -lSDL TX_Demo.cpp cc1100_raspi.cpp -o TX_Demo

run: all
	sudo ./TX_Demo -v -a1 -r3 -i1000 -t5 -c1 -f434 -m100
