root = ..
include $(root)/rules.mk

program = test_zh test_scan_file

%:%.cpp
	$(cpp) $(cppflag) $(inc) -o $@ $^ $(libs)
	@echo "  CC    $@"

all: $(program)

clean:
	rm -f *.o $(program)
