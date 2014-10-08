root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-audio find-audio

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-audio: fastwiki-audio.o
	$(cpp) -o $@ $^ $(libs)

find-audio: find-audio.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
