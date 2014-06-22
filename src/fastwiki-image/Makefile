root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))
libs += -lcurl

program = fastwiki-image

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-image: fastwiki-image.o image_parse.o image_tmp.o 
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
