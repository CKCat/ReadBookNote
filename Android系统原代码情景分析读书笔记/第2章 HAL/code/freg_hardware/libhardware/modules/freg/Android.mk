LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
# out/target/product/generic/system/lib/hw
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := freg.cpp
# 模块对应的文件名称定义为freg.default，编译成功后得到了一个freg.default.so文件
LOCAL_MODULE := freg.default
include $(BUILD_SHARED_LIBRARY)
