cc = gcc
cflag = -O2 -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DPERL_USE_SAFE_PUTENV \
	  -DNO_MATHOMS -DStrtol=strtol 
inc = -I../include
libs = -L../lib -lperl -lm

program = $(patsubst %.c,%, $(wildcard *.c))

%:%.c
	@$(cc) $(cflag) $(inc) -o $@ $< $(libs)
	@echo "  CC    $@"

all: $(program)

clean:
	rm -f $(program)
