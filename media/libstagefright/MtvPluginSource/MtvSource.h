
#ifndef MTV_SOURCE_H_

#define MTV_SOURCE_H_

#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <utils/threads.h>

#include "MtvController.h"
#include "CoreMtvSource.h"


namespace android {

class MtvSource : public MediaSource {
public:
    // Caller retains ownership of both "dataSource" and "sampleTable".
    MtvSource(const sp<MtvController> &controller, const int type);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);
            
    status_t init();
    status_t uinit();
    
    //int notifyListener_l(int msg, int ext1, int ext2);

    void FindAVCDimensions2(
        const sp<ABuffer> &seqParamSet, int32_t *width, int32_t *height);

protected:
    virtual ~MtvSource();
    
private:
    CoreMtvSource* mCoreMtvSource;

    Mutex mLock;

    MediaBufferGroup *mGroup;

    MediaBuffer *mBuffer;

    int mType;	//for video is 0, audio is 1
    sp<MtvController> mController;
    sp<MetaData> mFormat;
};

}  // namespace android

#endif  // MTV_SOURCE_H_
