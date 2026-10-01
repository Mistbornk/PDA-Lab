GXX = g++
GXXFLAG = -std=c++17 -I../include -Iinclude
FLAG = -Wall -O3 -static

SRC = main.cpp parser.cpp legalizer.cpp
EXE = Legalizer

all::$(EXE)

$(EXE): $(SRC) ../include/pda/io.hpp $(wildcard include/*.hpp inc/*.hpp)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $(SRC)

clean:
	rm -rf $(EXE)
