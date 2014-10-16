LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := fastwiki
LOCAL_SRC_FILES := \
	wiki_bookmark.cpp \
	wiki_config.cpp \
	wiki_common.cpp \
	wiki_history.cpp \
	wiki_favorite.cpp \
	wiki_scan_file.cpp \
	wiki_socket.cpp \
	wiki_local.cpp \
	wiki_data.cpp \
	wiki_index.cpp \
	wiki_full_index.cpp \
	wiki_zh.cpp \
	wiki_math.cpp \
	wiki_image.cpp \
	wiki_manage.cpp \
	wiki_audio.cpp \
	fastwiki.cpp 

ifeq ($(DEBUG), 1)
debug_flag = -DDEBUG -DFW_DEBUG
else
debug_flag = 
endif

LOCAL_CFLAGS += $(debug_flag) -DFW_NJI -DHAVE_PTHREADS -DHAVE_ANDROID_OS=1 \
				-DO_BINARY=0 -D_VERSION="\"$(shell cat VERSION | perl -pne chomp)\""

LOCAL_LDLIBS    := -lOpenSLES -lz -lstdc++ -L../../base/lib/ -labase
LOCAL_C_INCLUDES := . ../../base/include ../../src/lib

include $(BUILD_SHARED_LIBRARY)

