GXX = g++
GXXFLAG = -std=c++14 -Iinc 
FLAG = -Wall -O3

SRC = main.cpp
EXE = D2DGRter

all::$(EXE)

$(EXE): $(SRC)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $^

clean:
	rm -rf $(EXE)