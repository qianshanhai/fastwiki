cc  = gcc
cflag = -O2 -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DPERL_USE_SAFE_PUTENV \
	  -DNO_MATHOMS -DStrtol=strtol 
inc = -I../include -I.

ar = ar
arflag = rv

objs = $(patsubst %.c,%.o, $(wildcard *.c))
libperl = libperl.a

.SUFFIXES: .o .c .cpp

%.o:%.c
	@$(cc) $(cflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(libperl)

libperl.a: $(objs)
	$(ar) $(arflag) $@ $^

clean:
	rm -f $(libperl) $(objs)
