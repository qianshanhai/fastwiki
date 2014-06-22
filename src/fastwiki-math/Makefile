root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-math

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-math: fastwiki-math.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
