ifeq ($(LINUX_MINGW), 1)
cpp_prefix=i686-w64-mingw32-
endif

ifeq ($(LINUX_MINGW_64), 1)
cpp_prefix=x86_64-w64-mingw32-
endif

cc = $(cpp_prefix)gcc
ar = $(cpp_prefix)ar
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

