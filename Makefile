.DEFAULT_GOAL := all
.PHONY: test-unit test-func logo build clean all

CC=gcc
STANDARD=-std=c99
OPTIMISE=-O0
DEBUG=-g
INCLUDES=
CFLAGS=$(STANDARD) $(OPTIMISE) $(DEBUG) $(INCLUDES)
LIBPNG=-lpng
LIB=saxbospiral/saxbospiral.h
OS_NAME=
EXE_SUFFIX=.out

saxbospiral/saxbospiral$(OS_NAME).o: saxbospiral/saxbospiral.c $(LIB)
	$(CC) $(CFLAGS) -o saxbospiral/saxbospiral$(OS_NAME).o -c saxbospiral/saxbospiral.c

saxbospiral/serialise$(OS_NAME).o: saxbospiral/serialise.c $(LIB)
	$(CC) $(CFLAGS) -o saxbospiral/serialise$(OS_NAME).o -c saxbospiral/serialise.c

tests.o: $(LIB) tests.c
	$(CC) $(CFLAGS) -o tests$(OS_NAME).o -c tests.c

tests: saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o tests.o
	$(CC) $(CFLAGS) -o tests$(OS_NAME)$(EXE_SUFFIX) saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o tests$(OS_NAME).o

prepare.o: $(LIB) prepare.c
	$(CC) $(CFLAGS) -o prepare$(OS_NAME).o -c prepare.c

prepare: saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o prepare.o
	$(CC) $(CFLAGS) -o prepare$(OS_NAME)$(EXE_SUFFIX) saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o prepare$(OS_NAME).o

generate.o: $(LIB) generate.c
	$(CC) $(CFLAGS) -o generate$(OS_NAME).o -c generate.c

generate: saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o generate.o
	$(CC) $(CFLAGS) -o generate$(OS_NAME)$(EXE_SUFFIX) saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o generate$(OS_NAME).o

render.o: $(LIB) render.c
	$(CC) $(CFLAGS) -o render$(OS_NAME).o -c render.c

render: saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o render.o
	$(CC) $(CFLAGS) -o render$(OS_NAME)$(EXE_SUFFIX) saxbospiral/saxbospiral$(OS_NAME).o saxbospiral/serialise$(OS_NAME).o render$(OS_NAME).o $(LIBPNG)

test-unit: tests
	./tests$(OS_NAME)$(EXE_SUFFIX)

test-func: prepare generate render
	@echo "Running Functional Test"
	@echo -n "saxbospiral `git describe --abbrev=0`" > message.hex
	@./prepare$(OS_NAME)$(EXE_SUFFIX) message.hex message.sxp.hex
	@./generate$(OS_NAME)$(EXE_SUFFIX) message.sxp.hex
	@./render$(OS_NAME)$(EXE_SUFFIX) message.sxp.hex saxbospiral_test.png
	@diff saxbospiral.png saxbospiral_test.png
	@rm saxbospiral_test.png message.hex message.sxp.hex

logo: prepare generate render
	@echo "Generating logo"
	@echo -n "saxbospiral `git describe --abbrev=0`" > saxbospiral.hex
	@./prepare$(OS_NAME)$(EXE_SUFFIX) saxbospiral.hex saxbospiral.sxp
	@./generate$(OS_NAME)$(EXE_SUFFIX) saxbospiral.sxp
	@./render$(OS_NAME)$(EXE_SUFFIX) saxbospiral.sxp saxbospiral.png
	@rm saxbospiral.hex saxbospiral.sxp

build: prepare generate render

clean:
	rm -f *.o *.out *.exe

all: test-unit test-func build
