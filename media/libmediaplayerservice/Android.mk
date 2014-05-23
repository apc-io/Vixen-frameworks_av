LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    libdl                       \
    libgui                      \
    libmedia                    \
    libmedia_native             \
    libsonivox                  \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libvorbisidec               \
    libwmtapi                   \
    libwmtenv
    
LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \
    $(TOP)/frameworks/av/media/libstagefright/MtvPluginSource       \
    $(TOP)/frameworks/av/media/libstagefright/MtvPluginSource/core


LOCAL_MODULE:= libmediaplayerservice
LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS 
#stevexu LOCAL_CFLAGS += -DCFG_WMT_WPLAYER

WMT_WPLAYER := disable

ifeq ($(WMT_WPLAYER),enable)
LOCAL_C_INCLUDES += \
    $(TOP)/device/via/common/wplayer/lib                            \
    $(TOP)/device/via/common/ffmpeg                                 \
    $(TOP)/device/via/common/wmtpe                                  \
#   $(TOP)/device/via/common/wmt_api/include

LOCAL_SHARED_LIBRARIES += \
    libwplayer \
#   libwmtapi

endif

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
