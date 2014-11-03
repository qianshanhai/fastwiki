# Makefile

ifeq ($(LINUX_MINGW), 1)
cpp_prefix=i686-w64-mingw32-
endif

ifeq ($(LINUX_MINGW_64), 1)
cpp_prefix=x86_64-w64-mingw32-
endif

ifeq ($(android), yes) 
ANDK=/work/android/android-ndk
cpp = arm-linux-androideabi-g++ -O2
ar = arm-linux-androideabi-ar
inc = -I$(ANDK)/platforms/android-13/arch-arm/usr/include
cppflag = -DFW_NJI
else
ifeq ($(DEBUG), 1)
cpp = $(cpp_prefix)g++ -g -DDEBUG -Wall -fPIC
cc = $(cpp_prefix)gcc -g -DDEBUG -Wall -fPIC
else
cpp = $(cpp_prefix)g++ -O2 -Wall -fPIC
cc = $(cpp_prefix)gcc -O2 -Wall -fPIC
endif
ar = $(cpp_prefix)ar
inc = -I.
endif

arflag = rv
cppflag += -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE

ifeq ($(LINUX_MINGW), 1)
cppflag += -DWIN32 -D_WIN32
endif

objs = $(patsubst %.cpp,%.o, $(wildcard *.cpp))

ifneq ($(android), yes) 
#objs_c = $(patsubst %.c,%.o, $(wildcard *.c))
endif

ifeq ($(android), yes) 
libbase = libabase.a
else
libbase = libbase.a
endif

inc += -I../include -I../minilzo
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

