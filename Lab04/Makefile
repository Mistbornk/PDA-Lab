GXX = g++
GXXFLAG = -std=c++17 -I../include -Iinc
FLAG = -Wall -O3

SRC = main.cpp parser.cpp legacy_router.cpp layered_router.cpp
EXE = D2DGRter

all::$(EXE)

$(EXE): $(SRC) ../include/pda/io.hpp $(wildcard include/*.hpp inc/*.hpp)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $(SRC)

clean:
	rm -rf $(EXE)
