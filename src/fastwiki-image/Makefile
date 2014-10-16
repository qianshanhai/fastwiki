root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-image

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-image: fastwiki-image.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
