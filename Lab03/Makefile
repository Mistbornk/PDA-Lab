GXX = g++
GXXFLAG = -std=c++14 -Iinclude 
FLAG = -Wall -O3 -static

SRC = main.cpp
EXE = Legalizer

all::$(EXE)

$(EXE): $(SRC)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $^

clean:
	rm -rf $(EXE)