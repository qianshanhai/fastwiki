root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

libs += -llzma

program = fastwiki-zim

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-zim: fastwiki-zim.o zim.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
