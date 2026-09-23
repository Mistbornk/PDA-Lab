GXX = g++
GXXFLAG = -std=c++11
FLAG = -Wall -O3

SRC = main.cpp
EXE = Lab2

all::$(EXE)

$(EXE): $(SRC)
	$(GXX) $(GXXFLAG) $(FLAG) -o $@ $^

clean:
	rm -rf $(EXE)