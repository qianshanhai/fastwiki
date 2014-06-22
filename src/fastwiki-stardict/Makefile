root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-stardict

%.o:%.cpp
	@$(cpp) $(perlinc) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-stardict: fastwiki-stardict.o fastwiki-stardict-perl.o
	$(cpp) -o $@ $^ $(libs) $(perllib)

clean:
	rm -f $(program) $(objs)
