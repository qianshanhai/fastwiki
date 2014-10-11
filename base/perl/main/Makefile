cc = gcc
cflag = -g -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DPERL_USE_SAFE_PUTENV \
	  -DNO_MATHOMS -DStrtol=strtol 
inc = -I../include

ifeq ($(system), MINGW)
inc += -DPTW32_STATIC_LIB
libs = -static \
		-L../lib -lperl \
	   -L/c/MinGW/lib -L/c/MinGW/mingw32/lib -lpthreadGC2 \
	   -lws2_32 -lm 
# export MINGW_LIB=/c/MinGW/lib
else
libs = -L../lib -lperl -lm
cppflag += -DO_BINARY=0
USE_STARDICT_PERL=1
endif

program = $(patsubst %.c,%, $(wildcard *.c))

%:%.c
	@$(cc) $(cflag) $(inc) -o $@ $< $(libs)
	@echo "  CC    $@"

all: $(program)

clean:
	rm -f $(program)
