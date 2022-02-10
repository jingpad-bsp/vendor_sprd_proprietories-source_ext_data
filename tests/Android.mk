include $(CLEAR_VARS)
LOCAL_SRC_FILES := tests/test_ext_data.c

LOCAL_MODULE := test_ext_data
LOCAL_CFLAGS := -O2 -g -W -Wall -D__ANDROID__ -DNO_SCRIPT
LOCAL_SHARED_LIBRARIES := libnetutils liblog libcutils

#LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
