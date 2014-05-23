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

#ifndef MTV_EXTRACTOR_H_

#define MTV_EXTRACTOR_H_

#include <media/stagefright/MediaExtractor.h>
#include <utils/Vector.h>

#include "MtvSource.h"

namespace android {

struct AMessage;
class DataSource;
class SampleTable;
class String8;

class MtvExtractor : public MediaExtractor {
public:
    // Extractor assumes ownership of "source".
    MtvExtractor(const sp<MtvController> &controller);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();
    
    status_t init();

protected:
    virtual ~MtvExtractor();

private:

    status_t createSource();

    sp<MtvController> mController;
    bool mHaveMetadata;

    sp<MetaData> mMtvMetaData;
    //sp<MetaData> mVideoMetaData;
    //sp<MetaData> mAudioMetaData;
    
    sp<MtvSource> mVideoSource;
    sp<MtvSource> mAudioSource;
};

}  // namespace android

#endif  // MTV_EXTRACTOR_H_
