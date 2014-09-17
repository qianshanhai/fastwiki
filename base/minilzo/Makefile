cc = gcc
ar = ar
arflag = rv
cflag = -fPIC -Wall -fomit-frame-pointer

SOURCES = minilzo.c

LIBLZO = libfwminilzo.a

%.o:%.c
	@$(cc) $(cflag) -o $@ -c $<
	@echo "  CC    $@"

all: $(LIBLZO)

$(LIBLZO): minilzo.o
	@$(ar) $(arflag) $(LIBLZO) $^ > /dev/null 2>&1 && echo "  AR    $@"

testmini: testmini.c $(LIBLZO)
	$(cc) -I. -o $@ $< -L. -lminilzo

clean:
	@rm -f testmini *.o $(LIBLZO)
	@echo clean minilzo done.

