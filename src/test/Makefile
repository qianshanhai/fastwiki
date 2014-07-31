root = ..
include $(root)/rules.mk

cppflag += -I../fastwiki-httpd

program = test_zh test_wiki_image

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(program)

test_zh: test_zh.o
	$(cpp) -o $@ $^ $(libs)

test_wiki_image: test_wiki_image.o ../fastwiki-httpd/wiki_config.o
	$(cpp) -o $@ $^ $(libs)

clean:
	rm -f *.o $(program)
