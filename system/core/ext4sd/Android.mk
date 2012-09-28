LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= sdcard.c
LOCAL_MODULE:= ext4sd
LOCAL_MODULE_TAGS:= optional

LOCAL_SHARED_LIBRARIES := libc

include $(BUILD_EXECUTABLE)
