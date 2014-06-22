
ifeq ($(RELEASE), 1)
cpp = g++ -O2 -Wall -s #-static
else
cpp = g++ -Wall -g -DDEBUG
endif

ar = ar
arflag = rv

system = $(shell uname -s)
cppflag = -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE \
		  -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE 

libbase_inc = $(root)/../base/include
libbase_lib = $(root)/../base/lib

ifeq ($(system), Linux)
libs = -lm -lbz2 -lz -lpthread -L$(root)/lib -lfastwiki -L$(libbase_lib) -lbase
cppflag += -DO_BINARY=0
else
libs = -static -lm -lbz2 -lz -L$(MINGW_LIB) -lpthreadGC2 -static -L$(libbase_lib) -lbase -lws2_32 
endif

inc = -I. -D_FASTWIKI_BIN_ -I$(root)/lib -I$(libbase_inc)

ifeq ($(system), Linux)
perlinc = `perl -MExtUtils::Embed -e ccopts`
perllib = `perl -MExtUtils::Embed -e ldopts`
endif

version = $(shell cat VERSION | perl -pechomp)
cppflag += -D_VERSION="\"$(version)\""

