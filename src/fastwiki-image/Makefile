root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-image$(file_type)

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-image$(file_type): fastwiki-image.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)
