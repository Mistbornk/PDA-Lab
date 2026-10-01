GXX = g++
GXXFLAG = -std=c++17 -I../include -Iinclude
FLAG = -Wall -O3

SRC = main.cpp $(wildcard src/*.cpp)
EXE = Lab2

all::$(EXE)

$(EXE): $(SRC) ../include/pda/io.hpp $(wildcard include/*.hpp inc/*.hpp)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $(SRC)

clean:
	rm -rf $(EXE)
