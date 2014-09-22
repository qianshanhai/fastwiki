root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

program = fastwiki-httpd

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

fastwiki-httpd: fastwiki-httpd.o httpd-index.o 
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f $(program) $(objs)

fastwiki-httpd.h: fastwiki-httpd.html
	perl fastwiki-httpd.pl $^
