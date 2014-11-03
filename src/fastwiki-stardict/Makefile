root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-stardict$(file_type)

%.o:%.cpp
	$(cpp) $(optinc) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-stardict$(file_type): fastwiki-stardict.o stardict.o
	$(cpp) -o $@ $^ $(libs) $(optlib)

clean:
	rm -f $(program) $(objs)
