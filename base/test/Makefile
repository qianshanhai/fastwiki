cpp = g++
cppflag = -g
inc = -I../include
libs = -L../lib -lbase -L../minilzo -lfwminilzo

program = $(patsubst %.cpp,%, $(wildcard *.cpp))

%:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ $< $(libs)
	@echo "  CC    $@"

all: $(program)

clean:
	rm -f $(program)
