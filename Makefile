all:
	g++ -o gah -lncurses main.cpp
	g++ -o roast roast.cpp
gah:
	g++ -o gah -lncurses main.cpp
roast:
	g++ -o roast roast.cpp
clean:
	rm -f gah roast
