LOCAL_PATH := $(call my-dir)

SDL_IMAGE_PATH := $(LOCAL_PATH)/../SDL_image

# Build SDL2_image as an Android module.
include $(SDL_IMAGE_PATH)/Android.mk


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

LOCAL_CFLAGS := -fPIC -fpie -fno-common -DWIDESCREEN_HACK=1

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../include \
                    $(LOCAL_PATH)/../../../../libagbsyscall \
                    $(SDL_IMAGE_PATH)/include

LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game agbsyscall

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image

LOCAL_LDLIBS := -llog -landroid \
                -Wl,--allow-multiple-definition \
                -Wl,-z,muldefs \
                -Wl,--no-fatal-warnings

include $(BUILD_SHARED_LIBRARY)
