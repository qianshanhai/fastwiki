root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-full-index test-fidx find-fidx unload-fidx

%:%.cpp
	$(cpp) $(cppflag) $(inc) -o $@ $^ $(libs)
	@echo "  CC    $@"

all: $(program)

clean:
	rm -f $(program)
