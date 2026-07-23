LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := main

# Pure local name. The copy step above ensures it's right next to this file.
LOCAL_SRC_FILES := agbsyscall.c

LOCAL_CFLAGS    := -fPIC
LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game
LOCAL_SHARED_LIBRARIES       := SDL2
LOCAL_LDLIBS                 := -llog -landroid -Wl,-z,notext

include $(BUILD_SHARED_LIBRARY)
