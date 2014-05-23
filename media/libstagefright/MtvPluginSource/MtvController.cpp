#define LOG_NDEBUG 0
#define LOG_TAG "MtvController"

#include <sys/stat.h>
#include <unistd.h>

#include "MtvController.h"
#include "MtvLog.h"

namespace android {
	
MtvController::MtvController()
{
	LOGD("MtvController::MtvController()");
}

MtvController::~MtvController() 
{
	LOGD("MtvController::~MtvController()");
    disconnect();
}

status_t MtvController::initCheck() const
{
	LOGD("MtvController::initCheck()");
	return OK;
}

ssize_t MtvController::readAt(off64_t offset, void *data, size_t size)
{
	LOGD("MtvController::readAt()");
	return 0;
}

bool MtvController::getVideoBuffer(uint8_t *pData, int *psize, uint32_t *pts)
{
	//LOGD("MtvController::getVideoBuffer()");
	return datainputthd.getVideoBuffer(pData, psize, pts);
}

bool MtvController::getAudioBuffer(uint8_t *pData, int *psize, uint32_t *pts)
{
	//LOGD("MtvController::getAudioBuffer()");
	return datainputthd.getAudioBuffer(pData, psize, pts);
}

ssize_t MtvController::getMediaData(size_t index, void *data, size_t size)
{
	LOGD("MtvController::getMediaData");
	if(index == 1) //for video
	{
		
	}
	else //for audio
	{
		
	}
	
	return 0;
}

status_t MtvController::getSize(off_t *size)
{
	LOGD("MtvController::getSize()");
	return OK;
}

uint32_t MtvController::flags()
{
	return 0;
}

status_t MtvController::connect()
{
	//Connect at perpare stage
	LOGD("MtvController::connect");
	DataInputThd::startReceive(datainputthd);
	mIsConnected = true;
	return OK;
}

status_t MtvController::disconnect()
{
	LOGD("MtvController::disconnect");
	if(mIsConnected)
	{
		DataInputThd::stopReceive(datainputthd);
	}
	
	return OK;
}

bool MtvController::isAVDataEmpty()
{
	return datainputthd.isAVBufferEmpty();
}

bool MtvController::isAudioBufferedNFrame(int frameNum)
{
	return datainputthd.isAudioBufferedNFrame(frameNum);
}

DataInputThd* MtvController::getDataInputThd()
{
	//LOGD("MTV test DataInputThd");
	datainputthd.isAVBufferEmpty();
	return &datainputthd;
}

bool MtvController::isClientClosed()
{
	//LOGV("MtvController::isClientClosed");
	return datainputthd.isIntentToStop();
}

unsigned int MtvController::getPlayingAudioPTS()
{
	return datainputthd.getCurAudioPTS();
}

void MtvController::setPlayedAudioPTS(unsigned int pts)
{
	datainputthd.setPreAudioPTS(pts);
}

unsigned int MtvController::getPlayedAudioPTS()
{
	return datainputthd.getPreAudioPTS();
}

unsigned int MtvController::reportAudioPTS()
{
	unsigned int playingPTS = getPlayingAudioPTS();
	unsigned int playedPTS = getPlayedAudioPTS();
	
	if(playingPTS != playedPTS)
	{
		setPlayedAudioPTS(playingPTS);
		return playingPTS;
	}
	return 0;
}
	
} //namespace android
