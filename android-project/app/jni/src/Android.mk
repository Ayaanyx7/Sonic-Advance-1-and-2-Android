LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_SHARED_LIBRARIES := SDL2

# Go up 4 steps to map out the repository engine root directories
ENGINE_ROOT := $(LOCAL_PATH)/../../../../

# ⚡ REMOVED compilation targets to data code arrays to enforce asset-free legal shells
LOCAL_SRC_FILES := \
    $(wildcard $(ENGINE_ROOT)/src/*.c) \
    $(wildcard $(ENGINE_ROOT)/src/platform/sdl/*.c) \
    $(wildcard $(ENGINE_ROOT)/src/game/shared/*.c) \
    $(wildcard $(ENGINE_ROOT)/src/game/sa1/*.c)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../SDL2/include \
                    $(ENGINE_ROOT)/include \
                    $(ENGINE_ROOT)/src/platform/sdl

# ⚡ Rock-solid, non-breaking stable optimization parameter framework
LOCAL_CFLAGS := -DANDROID -D__ANDROID__ -DTARGET_SA1 -O2 -g

include $(BUILD_SHARED_LIBRARY)
