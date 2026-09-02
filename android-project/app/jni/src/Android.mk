LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libagbsyscall
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libagbsyscall/build/android/libagbsyscall.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main

SDL_PATH := SDL

LOCAL_CFLAGS    := -fPIC -fpie -fno-common -DWIDESCREEN_HACK=1

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../include \
                    $(LOCAL_PATH)/../../../../libagbsyscall

LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game agbsyscall
LOCAL_SHARED_LIBRARIES       := SDL2
LOCAL_LDLIBS                 := -llog -landroid \
                                -Wl,--allow-multiple-definition \
                                -Wl,-z,muldefs \
                                -Wl,--no-fatal-warnings
include $(BUILD_SHARED_LIBRARY)
