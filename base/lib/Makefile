# Makefile

ifeq ($(android), yes) 
ANDK=/work/android/android-ndk
cpp = arm-linux-androideabi-g++ -O2
ar = arm-linux-androideabi-ar
inc = -I$(ANDK)/platforms/android-13/arch-arm/usr/include
else
ifeq ($(DEBUG), 1)
cpp = g++ -g -DDEBUG -Wall -fPIC
cc = gcc -g -DDEBUG -Wall -fPIC
else
cpp = g++ -O2 -Wall -fPIC
cc = gcc -O2 -Wall -fPIC
endif
ar = ar
inc = -I.
endif

arflag = rv
cppflag = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

ifneq ($(android), yes) 
#objs_c = $(patsubst %.c,%.o, $(wildcard *.c))
endif

ifeq ($(android), yes) 
libbase = libabase.a
else
libbase = libbase.a
endif

inc += -I../include
#cppflag += -fPIC

.SUFFIXES: .o .c .cpp

%.o:%.cpp
	$(cpp) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

%.o:%.c
	$(cc) $(cppflag) $(inc) -o $@ -c $<
	@echo "  CC    $@"

all: $(libbase)

$(libbase): $(objs) $(objs_c)
	$(ar) $(arflag) $@ $^

clean:
	@rm -f $(objs) $(objs_c) $(libbase)
	@echo clean done.

