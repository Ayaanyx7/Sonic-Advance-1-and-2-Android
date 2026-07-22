LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := agbsyscall
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libagbsyscall/build/android/libagbsyscall.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_SRC_FILES := stub.c
LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game agbsyscall
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
