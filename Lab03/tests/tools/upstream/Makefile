# Makefile for compiling main.cpp with g++

CXX = g++
CXXFLAGS = -g -Wall -static -std=c++11
TARGET = Evaluator
SOURCES = ./evaluator/evaluator.cpp
LIBS = -lpthread

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)


run1:
	./$(TARGET) ./testcase/testcase1_16900.lg ./testcase/testcase1_16900.opt ./testcase/testcase1_16900_post.lg 2>&1 | tee run1.log

run2:
	./$(TARGET) ./testcase/testcase2_100.lg ./testcase/testcase2_100.opt ./testcase/testcase2_100_post.lg

run3:
	./$(TARGET) ./testcase/testcase1_ALL0_5000.lg ./testcase/testcase1_ALL0_5000.opt ./testcase/testcase1_ALL0_5000_post.lg

success:
	./$(TARGET) ./testcase/success_testcase/debug.lg ./testcase/success_testcase/debug.opt ./testcase/success_testcase/debug.out

overlap:
	./$(TARGET) ./testcase/fail_testcase/debug.lg ./testcase/fail_testcase/debug.opt ./testcase/fail_testcase/debug.out

debug2:
	./$(TARGET) ./testcase/student/testcase_my0.lg ./testcase/student/testcase_my0.opt ./testcase/student/testcase_my0_post.lg

clean:
	rm -f $(TARGET)
