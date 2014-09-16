root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-full-index test-fidx find-fidx

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-full-index: fastwiki-full-index.o
	$(cpp) -o $@ $^ $(libs)

test-fidx: test-fidx.o
	$(cpp) -o $@ $^ $(libs)

find-fidx: find-fidx.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
