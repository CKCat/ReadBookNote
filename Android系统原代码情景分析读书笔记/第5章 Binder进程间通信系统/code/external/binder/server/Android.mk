LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils libutils libbinder
LOCAL_SRC_FILES := ../common/IFregService.cpp FregServer.cpp
LOCAL_MODULE := FregServer
include $(BUILD_EXECUTABLE)