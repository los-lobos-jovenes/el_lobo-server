LIBZ= -pthread
CXX= g++
SRCZ= src/main.cpp
HDRZ=
INC= -I src/
TRGT= lobo
UBSAN= -fsanitize=address -fsanitize=leak -fsanitize=undefined
OPT= -g
DEBUG?=true

ifeq ($(OS),Windows_NT)
#TRGT= lobo.exe
$(error It will not work on Windows)
endif

ifeq ($(DEBUG), false)
UBSAN= 
OPT= -O3
else
OPT += -DMSG_SEPARATOR=\'\;\'
endif

all: $(SRCZ) $(HDRZ)
	$(CXX) $(SRCZ) -o $(TRGT) -std=c++17 -Wall -Wextra -Wpedantic $(OPT) $(UBSAN) $(INC) $(LIBZ)

run: all
	./$(TRGT)

clean:
	rm -rf $(TRGT)

rebuild: clean all

.PHONY: all clean rebuild run
.SUFFIXES: