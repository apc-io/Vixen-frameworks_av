#define LOG_NDEBUG 0
#define LOG_TAG "MtvExtractor"
#include <ctype.h>

#include "MtvExtractor.h"
#include "MtvStagefrightTypes.h"
#include "MtvLog.h"

namespace android {
	
MtvExtractor::MtvExtractor(const sp<MtvController> &controller)
: mController(controller),
mMtvMetaData (new MetaData())
{
	
}

MtvExtractor::~MtvExtractor()
{
	LOGD("MtvExtractor::~MtvExtractor()");
	//for sp we can't delete it
	
	//delete mMtvMetaData;
	//delete mVideoSource;
	//delete mAudioSource;
}

status_t MtvExtractor::init()
{
	return createSource();
	
	//TODO: mController get parameters
}

status_t MtvExtractor::createSource()
{
	//TODO: get if the source is A/V or only A
	
	mVideoSource = new MtvSource(mController, MTV_SOURCE_VIDEO);
    mAudioSource = new MtvSource(mController, MTV_SOURCE_AUDIO);
    
    return OK;
}

size_t MtvExtractor::countTracks()
{
	LOGD("MtvExtractor::countTracks()");
	
	//you can return 1 to leave video only for debug
	return 2;
	//return 1;
}

sp<MediaSource> MtvExtractor::getTrack(size_t index)
{
	
	//here can't return NULL which will lead to data abort in CHECK()
	
	//0 for video, 1 for audio
	if(index == MTV_SOURCE_VIDEO)
	{
		return mVideoSource;
	}
	
	return mAudioSource;
}

sp<MetaData> MtvExtractor::getTrackMetaData(size_t index, uint32_t flags)
{
	//here can't return NULL which will lead to data abort in CHECK()
	
	if(index == MTV_SOURCE_VIDEO)
	{
		return mVideoSource->getFormat();
	}
	else if(index == MTV_SOURCE_AUDIO)
	{
		return mAudioSource->getFormat();
	}
	
	return NULL;
}

sp<MetaData> MtvExtractor::getMetaData()
{
	return mMtvMetaData;
}
	
} //namespace android
