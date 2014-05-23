/*
*
* Copyright (C) 2011 Siano Mobile Silicon Ltd. All rights reserved 
*                                                                       				
* PROPRIETARY RIGHTS of Siano Mobile Silicon are involved in the 
* subject matter of this material.  All manufacturing, reproduction, 
* use, and sales rights pertaining to this subject matter are governed  
* by the license agreement.  The recipient of this software implicitly   
* accepts the terms of the license.                                     		   
*
*/

#ifndef MTV_CONTROLLER_H_

#define MTV_CONTROLLER_H_

#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <media/stagefright/DataSource.h>

#include "stagefright_mtvparser_datainputthread.h"

namespace android {

class MtvController : public DataSource
{
public:
    MtvController();
    ~MtvController();
	
    virtual status_t initCheck() const;
    virtual ssize_t readAt(off64_t offset, void *data, size_t size);//not used
    
    //virtual ssize_t readAt(off_t offset, void *data, size_t size);//not used

    // May return ERROR_UNSUPPORTED.
    virtual status_t getSize(off_t *size);

    virtual uint32_t flags();
	
    //connect to mtvserver
    status_t connect();
    status_t disconnect();
	
    //get data
    bool getVideoBuffer(uint8_t *pData, int *psize, uint32_t *pts);
    bool getAudioBuffer(uint8_t *pData, int *psize, uint32_t *pts);
	
    ssize_t getMediaData(size_t index, void *data, size_t size);//read A/V data
	
    bool isAVDataEmpty();
    bool isAudioBufferedNFrame(int frameNum); //is there frameNum frames in Audio Buffer. 1 frame ~= 25 items.
	
    DataInputThd* getDataInputThd();

    bool isClientClosed();

    unsigned int getPlayingAudioPTS();
    void setPlayedAudioPTS(unsigned int pts);
    unsigned int getPlayedAudioPTS();
    unsigned int reportAudioPTS();

    void(*pNotifyListenerCallback)(void* privData, int pts);
    void * mPrivData;

private:
    DataInputThd datainputthd;
    bool mIsConnected;
};

}// namespace android

#endif  // MTV_CONTROLLER_H_
