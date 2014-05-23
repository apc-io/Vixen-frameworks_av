
#ifndef CORE_MTV_SOURCE_H_
#define CORE_MTV_SOURCE_H_

#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include <media/stagefright/MediaDefs.h>
#include <utils/threads.h>

#include "stagefright_mtvparser_datainputthread.h"
#include "CoreMediaBuffer.h"
#include "CoreMediaBufferGroup.h"


#define PTS_ADJUST_EARLY_TIME 20 // in ms

#define MTVRCV_H264_NAL_HEAD_LEN 0x3

#define MTVRCV_ISDBT_HEADER_SIZE 8
#define MTVRCV_SOCKET_EXTRA_HEADER_SIZE 8 

#define MTVRCV_SOCKET_VIDEO                  0x1
#define MTVRCV_SOCKET_AUDIO_FRAME            0x2
#define MTVRCV_SOCKET_AUDIO_EXTRA            0x3
#define MTVRCV_SOCKET_AUDIO_FRAME_WITH_PARAM 0x4
#define MTVRCV_SOCKET_AUDIO_MPEG             0x5
#define MTVRCV_SOCKET_VIDEO_MPEG1            0x6
#define MTVRCV_SOCKET_VIDEO_MPEG2            0x7
#define MTVRCV_SOCKET_VIDEO_MPEG4            0x8
#define MTVRCV_SOCKET_VIDEO_STOP             0xa
#define MTVRCV_SOCKET_AUDIO_STOP             0xb


namespace android {

class CoreMtvSource {
public:
    // Caller retains ownership of both "dataSource" and "sampleTable".
    CoreMtvSource(DataInputThd* datainput, const int type);

    status_t read(CoreMediaBuffer **buffer);
    
    status_t init();
    status_t uinit();
    uint32_t getFormat(int mType, uint8_t *mFormatBuf);
    
    uint32_t u32_at(uint8_t *ptr);
    void u32_ta(uint32_t u32, uint8_t *buf);

    ~CoreMtvSource();
    
private:
    void setPTS(uint8_t *pMediaBuf, uint32_t uPTS);
    uint32_t getPTS(uint8_t *pMediaBuf);
    uint32_t getPTS2(uint8_t *pMediaBuf);
    uint32_t NALToH264FRAME(uint8_t* nalStart, uint32_t nalType, uint32_t nalSize, uint8_t* mH264FrameBuf);
    
    void MakeMPEGVideoESDS(uint8_t *esdsData, uint8_t *data, size_t dataSize);
    void EncodeSize14(uint8_t **_ptr, size_t size);
    
    bool GetMPEGAudioFrameSize(
    uint8_t *ptr, int len, unsigned *layer, size_t *frame_size,
    int *out_sampling_rate, int *out_channels,
    int *out_bitrate, int *out_num_samples);

    uint8_t *mFrameBuf;
    CoreMediaBufferGroup *mGroup;

    CoreMediaBuffer *mBuffer;
    bool mIsNewBuffer;

    int mType;	//for video is 0, audio is 1
    DataInputThd* mDataInputThd;

    uint16_t iVideoSeq;
    uint16_t iAudioSeq;
    uint16_t iLastVideoRtpSeq;
    uint16_t iLastAudioRtpSeq;		
    bool bSkipVideoFrame;
    bool bSendSPS;
    bool bSendPPS;
    bool bFindIDRFrame;
    bool mIsLastNalFUA;
	
    uint8_t* mMediaBuf;
    uint32_t currentPTS[2];
    
    uint32_t mAudioLastPTS;
    int32_t mAudioDeltaPTS;
    uint32_t mVideoLastPTS;
    int32_t mVideoDeltaPTS;
    int adjustAudioPTS(uint8_t *pMediaBuf);
    int adjustVideoPTS(uint8_t *pMediaBuf);
    
    uint32_t totalSize;
    bool foundSlice;
};

}  // namespace android

#endif  // CORE_MTV_SOURCE_H_
