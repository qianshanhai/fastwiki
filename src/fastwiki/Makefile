root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki$(file_type)

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

$(program): fastwiki.o wiki_parse.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
