LIBZ= -pthread
CXX= g++
SRCZ= src/main.cpp src/dbStaticDriver.cpp src/driver.cpp
HDRZ=
INC= -I src/
TRGT= lobo
UBSAN=
OPT=
DEBUG?=false

ifeq ($(OS),Windows_NT)
#TRGT= lobo.exe
$(error It will not work on Windows)
endif

ifeq ($(DEBUG), true)
OPT += -g -DMSG_SEPARATOR=\'\;\' -DDEBUG_LEVEL=Debug
UBSAN = -fsanitize=address -fsanitize=leak -fsanitize=undefined
else ifeq ($(DEBUG), some)
UBSAN =
OPT += -O3 -DDEBUG_LEVEL=Debug
else
UBSAN =
OPT += -O3 -DDEBUG_LEVEL=Info
endif

ifeq ($(DISABLE_1PULL), true)
OPT += -DOLD_PULL_DISABLED=1
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