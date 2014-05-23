LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libstagefright_mtvnode.so:obj/lib/libstagefright_mtvnode.so
LOCAL_SRC_FILES += libstagefright_mtvnode.so:system/lib/libstagefright_mtvnode.so
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libstagefright_mtvnode_intermediates/export_includes
include $(WMT_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
		MtvController.cpp \
		MtvExtractor.cpp \
		MtvSource.cpp
       
LOCAL_C_INCLUDES := \
        $(TOP)/frameworks/av/include/media/stagefright \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/av/media/libstagefright/MtvPluginSource/core\
       $(TOP)/external/MobileTVServer/main/stagefright        
LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libutils \
        libandroid_runtime\
        libstagefright_mtvnode

LOCAL_MODULE := libstagefright_mtvpluginsource

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := eng

include $(BUILD_STATIC_LIBRARY)
