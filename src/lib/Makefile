root = ..
include $(root)/rules.mk

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

libfastwiki = libfastwiki.a

%.o:%.cpp
	@$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(libfastwiki)

$(libfastwiki): $(objs)
	$(ar) $(arflag) $@ $^

clean:
	rm -f $(libfastwiki) $(objs)

wiki_local_auto.h: msg.txt
	perl local.pl msg.txt

# file ZhConversion.php can found in mediawiki/includes/
wiki_zh_auto.h: ../../utils/mediawiki/ZhConversion.php
	perl convert_zh.pl $^ > $@
