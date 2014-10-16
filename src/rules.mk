
ifeq ($(RELEASE), 1)
cpp = g++ -O2 -Wall -s #-static
else
cpp = g++ -Wall -g -DDEBUG
endif

ar = ar
arflag = rv

system = $(shell uname -s | perl -ne 'chomp;print substr($$_, 0, 5)')
cppflag = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE \
		  -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE 

libbase_inc = $(root)/../base/include
libbase_lib = $(root)/../base/lib

ifeq ($(system), MINGW)
inc = -DPTW32_STATIC_LIB
libs = -static -L$(root)/lib -L$(libbase_lib) -static \
	   -lfastwiki -lbase \
	   -L/c/MinGW/lib -L/c/MinGW/mingw32/lib -lpthreadGC2 \
	   -lws2_32 -lm -lbz2 -lz
# export MINGW_LIB=/c/MinGW/lib
else
libs = -lm -lbz2 -lz -lpthread -L$(root)/lib -lfastwiki -L$(libbase_lib) -lbase
cppflag += -DO_BINARY=0
endif

inc += -I. -D_FASTWIKI_BIN_ -I$(root)/lib -I$(libbase_inc) -I$(root)/../base/lua/src
libs += -L$(root)/../base/minilzo -lfwminilzo -L$(root)/../base/lua/src -llua -ldl

version = $(shell cat VERSION | perl -pechomp)
cppflag += -D_VERSION="\"$(version)\"" -D_AUTHOR="\"Qianshanhai <qianshanhai@gmail.com>\"" \
		   -D_WEBSITE="\"https://fastwiki.me\""

