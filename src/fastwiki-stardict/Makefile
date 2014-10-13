root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-stardict 

%.o:%.cpp
	$(cpp) $(optinc) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-stardict: fastwiki-stardict.o stardict.o
	$(cpp) -o $@ $^ $(libs) $(optlib)

clean:
	rm -f $(program) $(objs)
