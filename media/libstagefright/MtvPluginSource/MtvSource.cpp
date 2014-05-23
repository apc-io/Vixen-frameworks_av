#define LOG_NDEBUG 0
#define LOG_TAG "MtvSource"

#include <utils/List.h>
#include <media/MediaPlayerInterface.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ABitReader.h>
#include <include/avc_utils.h>

#include "MtvSource.h"
#include "MtvStagefrightTypes.h"
#include "MtvLog.h"

#define MTV_MAX_MEDIA_BUFFER_SIZE	(640 * 1024)

namespace android {

static int copyCoreMediaBuffer(MediaBuffer *dst, CoreMediaBuffer *src)
{
	CHECK(dst != NULL);
	CHECK(src != NULL);
	
	dst->reset();
	
	memcpy(dst->data(), src->data(), src->range_offset()+src->range_length());
	dst->set_range(src->range_offset(), src->range_length());
	
	dst->meta_data()->clear();
	dst->meta_data()->setInt64(kKeyTime, src->getTime());//for time check
	
	return 0;
}

MtvSource::MtvSource(const sp<MtvController> &controller, const int type)
		: mBuffer(NULL)
{
	mFormat = NULL;
	mType = type;
	mController = controller;
	mGroup = NULL;

	LOGV("MtvSource::MtvSource enter");
	mCoreMtvSource = new CoreMtvSource(mController->getDataInputThd(), type);
}

MtvSource::~MtvSource()
{
	delete mCoreMtvSource;
}

status_t MtvSource::init()
{
	LOGV("MtvSource::init type %d", mType);
	//Mutex::Autolock autoLock(mLock);
	
	mGroup = new MediaBufferGroup;
	//add two buffer, a general one, the other is for last video buffer
	mGroup->add_buffer(new MediaBuffer(MTV_MAX_MEDIA_BUFFER_SIZE));

	mCoreMtvSource->init();
	
	return OK;
}

status_t MtvSource::uinit()
{
	LOGV("MtvSource::uinit %d", mType);
	//Mutex::Autolock autoLock(mLock);
	if (mBuffer != NULL) {
        mBuffer->release();
        mBuffer = NULL;
    }
	
    delete mGroup;
    mGroup = NULL;
    
    mCoreMtvSource->uinit();
    
    return OK;
}

status_t MtvSource::start(MetaData *params)
{
	LOGV("MtvSource::start %d", mType);
	Mutex::Autolock autoLock(mLock);
	
	init();
	return 0;
}

status_t MtvSource::stop()
{
	LOGV("MtvSource::stop %d", mType);
	//called when player's reset is called
	Mutex::Autolock autoLock(mLock);
	uinit();
	return 0;
}

sp<MetaData> MtvSource::getFormat()
{
	//LOGV("MtvSource::getFormat() %d", mType);
	if(mFormat == NULL)
	{
		LOGV("MtvSource: getFormat new a MetaData");
		mFormat = new MetaData();
		//for first time call, fullfil metadata
		if(mType == MTV_SOURCE_VIDEO)
		{
			int32_t width = 0;
            int32_t height = 0;
			uint8_t buf[1024] = {0x0};
			uint32_t len = mCoreMtvSource->getFormat(MTV_SOURCE_VIDEO, buf);
			if(len == 0xFFFC || len == 0xFFFD) //MPEG1/2 video
			{
				//mFormat->setCString(kKeyMIMEType, "video/mpeg2");
				mFormat->setCString(kKeyMIMEType, "video/mp2");
				
				uint32_t esdsSize = mCoreMtvSource->u32_at(buf);
				mFormat->setData(kKeyESDS, kTypeESDS, buf + 4, esdsSize);
				
				width = (buf[4 + 25 + 4] << 4) | buf[4 + 25 + 5] >> 4;
                height = ((buf[4 + 25 + 5] & 0x0f) << 8) | buf[4 + 25 + 6];
				LOGD("MtvSource::MPEGV: width x height: (%d x %d)", width, height);
				mFormat->setInt32(kKeyWidth, width);
			    mFormat->setInt32(kKeyHeight, height);
			}
			else if(len == 0xFFFE) //MPEG4 video
			{
                // TODO: MPEG4 Video will be supported later.
				mFormat->setCString(kKeyMIMEType, "video/mp4v-es");
				
				uint32_t esdsSize = mCoreMtvSource->u32_at(buf);
				mFormat->setData(kKeyESDS, kTypeESDS, buf + 4, esdsSize);
				
				LOGD("MtvSource::MPEG4V: width x height: (%d x %d)", width, height);
				mFormat->setInt32(kKeyWidth, width);
			    mFormat->setInt32(kKeyHeight, height);
			}
			else if(len > 0) //H264
			{
			    mFormat->setCString(kKeyMIMEType, "video/avc");
			    mFormat->setData(kKeyAVCC, 0, buf, len);
				
			    uint32_t spsSize = (uint32_t)((buf[6] << 8) + buf[7]);
			    //LOGD("sps size: %d", spsSize);
			    sp<ABuffer> seqParamSet = new ABuffer(spsSize); //SPS
			    memcpy(seqParamSet->data(), &buf[8], spsSize);
				
			    FindAVCDimensions2(seqParamSet, &width, &height);
                if(width > 1920 || width < 320)
                {
					LOGE("MtvSource::H264: width: %d not correct, set to 1920", width);
					width = 1920;
				}
				if(height > 1080 || height < 180)
				{
					LOGE("MtvSource::H264: height: %d not correct, set to 1080", height);
					height = 1080;
				}
				LOGD("MtvSource::H264: width x height: (%d x %d)", width, height);
			    mFormat->setInt32(kKeyWidth, width);
			    mFormat->setInt32(kKeyHeight, height);
			}
			else //default
			{
			    mFormat->setCString(kKeyMIMEType, "video/avc");
			    mFormat->setInt32(kKeyWidth, 1920);
			    mFormat->setInt32(kKeyHeight, 1080); //(320, 240)
			}
			LOGD("MtvSource::set SMS DTV flag");
			//mFormat->setInt32(kKeyIsSmsDtv, 1); // set SMS DTV flag
		}
		else if(mType == MTV_SOURCE_AUDIO)
		{
			uint8_t buf[256] = {0x0}; 
            int32_t samplingFreqTable[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

            uint32_t len = mCoreMtvSource->getFormat(MTV_SOURCE_AUDIO, buf);
            if(len == 0xFFFF) //mpeg audio
            {
				uint8_t layer = buf[0];
				//switch(layer)
				//{
				//	case 1: { mFormat->setCString(kKeyMIMEType, "audio/mpeg-L1"); break; }
				//	case 2: { mFormat->setCString(kKeyMIMEType, "audio/mpeg-L2"); break; }
				//	case 3: { mFormat->setCString(kKeyMIMEType, "audio/mpeg"); break; }
				//	default: { LOGE("incorrect Audio MPEG Layer: %d", layer); }
				//}
				//bill modify				
				mFormat->setCString(kKeyMIMEType, "audio/mpeg"); 

				int32_t sampleRate = mCoreMtvSource->u32_at(&buf[1]);
				int32_t channelCount = mCoreMtvSource->u32_at(&buf[5]);
				LOGD("MtvSource::MPEGA: layer:%d, channel configuration:%d, sampling frequency:%d", layer, channelCount, sampleRate);
				mFormat->setInt32(kKeyChannelCount, channelCount);
			    mFormat->setInt32(kKeySampleRate, sampleRate);
			}
			else if(len > 0) //aac audio
			{
				mFormat->setCString(kKeyMIMEType, "audio/mp4a-latm");
				
                mFormat->setData(kKeyESDS, kTypeESDS, buf, len);

                int32_t smpIndex = buf[22] & 0x7;
                smpIndex <<= 1;
                smpIndex |= (buf[23] >> 7) & 0x1;
                LOGD("sampling rate index: %d", smpIndex);
                //if(smpIndex >= 6) { smpIndex -= 3; }
                int32_t channelCount = (buf[23] >> 3) & 0x7;
                LOGD("MtvSource::AAC: channel configuration: %d, sampling frequency: %d", channelCount, samplingFreqTable[smpIndex/2]);
                //LOGD("MtvSource::AAC: channel configuration: %d, sampling frequency: %d", channelCount, samplingFreqTable[smpIndex]);
                mFormat->setInt32(kKeyChannelCount, channelCount);
			    mFormat->setInt32(kKeySampleRate, samplingFreqTable[smpIndex/2]);
			    //mFormat->setInt32(kKeySampleRate, samplingFreqTable[smpIndex]);
			}
			else //default
			{
				mFormat->setCString(kKeyMIMEType, "audio/mp4a-latm");
				
                buf[0] = 0x03; buf[1] = 2 + 20;
                buf[2] = 0xff; buf[3] = 0xff; // ES_ID
                buf[4] = 0x00; // streamDependenceFlag, URL_Flag, OCRstreamFlag
                
                buf[5] = 0x04; buf[6] = 2 + 15;
                buf[7] = 0; //0x40: Audio ISO/IEC 14496-3
                buf[8] = 0; buf[9] = 0; buf[10] = 0; buf[11] = 0;
                buf[12] = 0; buf[13] = 0; buf[14] = 0; buf[15] = 0;
                buf[16] = 0; buf[17] = 0; buf[18] = 0; buf[19] = 0;
                
                buf[20] = 0x05; buf[21] = 2;
                
                // AudioSpecificInfo follows

                // oooo offf fccc c000
                // o - audioObjectType
                // f - samplingFreqIndex
                // c - channelConfig
                buf[22] = 2 << 3;
                buf[22] |= (6 >> 1) & 0x7;
                buf[23] = (6 << 7) & 0x80;
                buf[23] |= (1 << 3) & 0x78;
                mFormat->setData(kKeyESDS, kTypeESDS, buf, 22 + 2);
                
			    mFormat->setInt32(kKeyChannelCount, 1);
			    mFormat->setInt32(kKeySampleRate, 48000);
			}
		}
		else
		{
			LOGE("unknow track");
		}
	}
	return mFormat;
}

// Determine video dimensions from the sequence parameterset.
void MtvSource::FindAVCDimensions2(
        const sp<ABuffer> &seqParamSet, int32_t *width, int32_t *height) {
    ABitReader br(seqParamSet->data() + 1, seqParamSet->size() - 1);

    unsigned profile_idc = br.getBits(8);
    br.skipBits(16);
    parseUE(&br);  // seq_parameter_set_id

    unsigned chroma_format_idc = 1;  // 4:2:0 chroma format

    if (profile_idc == 100 || profile_idc == 110
            || profile_idc == 122 || profile_idc == 244
            || profile_idc == 44 || profile_idc == 83 || profile_idc == 86) {
        chroma_format_idc = parseUE(&br);
        if (chroma_format_idc == 3) {
            br.skipBits(1);  // residual_colour_transform_flag
        }
        parseUE(&br);  // bit_depth_luma_minus8
        parseUE(&br);  // bit_depth_chroma_minus8
        br.skipBits(1);  // qpprime_y_zero_transform_bypass_flag
        //CHECK_EQ(br.getBits(1), 0u);  // seq_scaling_matrix_present_flag
        LOGD("CHECK_EQ--CHECK_EQ--CHECK_EQ: %dvs0", br.getBits(1));
    }

    parseUE(&br);  // log2_max_frame_num_minus4
    unsigned pic_order_cnt_type = parseUE(&br);

    if (pic_order_cnt_type == 0) {
        parseUE(&br);  // log2_max_pic_order_cnt_lsb_minus4
    } else if (pic_order_cnt_type == 1) {
        // offset_for_non_ref_pic, offset_for_top_to_bottom_field and
        // offset_for_ref_frame are technically se(v), but since we are
        // just skipping over them the midpoint does not matter.

        br.getBits(1);  // delta_pic_order_always_zero_flag
        parseUE(&br);  // offset_for_non_ref_pic
        parseUE(&br);  // offset_for_top_to_bottom_field

        unsigned num_ref_frames_in_pic_order_cnt_cycle = parseUE(&br);
        for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            parseUE(&br);  // offset_for_ref_frame
        }
    }

    parseUE(&br);  // num_ref_frames
    br.getBits(1);  // gaps_in_frame_num_value_allowed_flag

    unsigned pic_width_in_mbs_minus1 = parseUE(&br);
    unsigned pic_height_in_map_units_minus1 = parseUE(&br);
    unsigned frame_mbs_only_flag = br.getBits(1);

    *width = pic_width_in_mbs_minus1 * 16 + 16;

    *height = (2 - frame_mbs_only_flag)
        * (pic_height_in_map_units_minus1 * 16 + 16);

    if (!frame_mbs_only_flag) {
        br.getBits(1);  // mb_adaptive_frame_field_flag
    }

    br.getBits(1);  // direct_8x8_inference_flag

    if (br.getBits(1)) {  // frame_cropping_flag
        unsigned frame_crop_left_offset = parseUE(&br);
        unsigned frame_crop_right_offset = parseUE(&br);
        unsigned frame_crop_top_offset = parseUE(&br);
        unsigned frame_crop_bottom_offset = parseUE(&br);

        unsigned cropUnitX, cropUnitY;
        if (chroma_format_idc == 0  /* monochrome */) {
            cropUnitX = 1;
            cropUnitY = 2 - frame_mbs_only_flag;
        } else {
            unsigned subWidthC = (chroma_format_idc == 3) ? 1 : 2;
            unsigned subHeightC = (chroma_format_idc == 1) ? 2 : 1;

            cropUnitX = subWidthC;
            cropUnitY = subHeightC * (2 - frame_mbs_only_flag);
        }

        LOGV("frame_crop = (%u, %u, %u, %u), cropUnitX = %u, cropUnitY = %u",
             frame_crop_left_offset, frame_crop_right_offset,
             frame_crop_top_offset, frame_crop_bottom_offset,
             cropUnitX, cropUnitY);

        *width -=
            (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
        *height -=
            (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;
    }
}

status_t MtvSource::read(
            MediaBuffer **buffer, const ReadOptions *options)
{
	//Don't return error. if return value
	
	//LOGV("MtvSource::read type is %d", mType);
	Mutex::Autolock autoLock(mLock);
	
	*buffer = NULL;
	
	int err;
	
	if (mBuffer == NULL) {
		
		//LOGV("Try to get buffer from BufferGroup");
        err = mGroup->acquire_buffer(&mBuffer);

        if (err != OK) {
            CHECK(mBuffer == NULL);
            return err;
        }
        
        mBuffer->set_range(0,0);
    }
    
    CoreMediaBuffer *rst = NULL;
    
    err = mCoreMtvSource->read(&rst);
    
    if(err == OK)
    {
		copyCoreMediaBuffer(mBuffer, rst);
		rst->release();
		rst = NULL;
		
		//LOGV("normal clone the mBuffer");
		MediaBuffer *clone = mBuffer->clone();
		CHECK(clone != NULL);
		
		CHECK(mBuffer != NULL);
		mBuffer->release();
		mBuffer = NULL;
		
		*buffer = clone;
		err = OK;
		
		if(mType == 1) //audio
		{
		    int pts = (int)mController->reportAudioPTS();
		    if(pts != 0)
		    {
			    if(mController->pNotifyListenerCallback != NULL)
			    {
				    mController->pNotifyListenerCallback(mController->mPrivData, pts);
			    }
	        }
	    }		
	}
    else
    {
		LOGE("MtvSource::read: CoreMtvSource->read return Error");
		err = ERROR_IO;
	}
		
	return err;
    
}
	
} //namespace android
