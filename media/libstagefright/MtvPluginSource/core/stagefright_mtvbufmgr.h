#ifndef STAGEFRIGHT_MTV_BUFMGR_H
#define STAGEFRIGHT_MTV_BUFMGR_H

#include <stdint.h>
#include <sys/types.h>


#define QMAX (4000 * 10)

#define MAX_MEDIA_BUFF_SIZE (640 * 1024)

//H264 NAL to /mnt/sdcard/mediaH264File.h264
//#define H264_NAL_TO_FILE

#ifdef H264_NAL_TO_FILE
#define H264_NAL_BUFFER_SIZE    (16*1024*1024)
#endif


typedef uint8_t Item;


typedef struct node
{
 Item *pitem;
 int size;
 unsigned int pts;
 struct node *next;
} Node;

typedef struct queue
{
 Node *front;
 Node *rear;
 int items;
 pthread_mutex_t Queue_mutex;
 pthread_cond_t notempty;
 pthread_cond_t notfull;
} Queue;

//--------------------------------------------------------------------------------------------------
//                                     GLOBAL VARIABLES
//--------------------------------------------------------------------------------------------------


//Queue mBufQueue;

/*
// Inline function to lock global mutex 
static inline void
MTV_LOCK(void)
{
    int retval = pthread_mutex_lock(&mtv_mutex);
    if (retval != 0)
        return;
}   

// Inline function to unlock the global mutex 
static inline void
MTV_UNLOCK(void)
{
    int retval = pthread_mutex_unlock(&mtv_mutex);
    if (retval != 0)
        return;
}
*/

//using namespace android;

//typedef int32_t status_t;

class StageFrightMtvBufMgr // : public BnMediaPlayer
{
    public:
        StageFrightMtvBufMgr();
        ~StageFrightMtvBufMgr();

        int InputRTPDataToAudioQueue(uint8_t *pData, int size, uint32_t pts);
        int InputRTPDataToVideoQueue(uint8_t *pData, int size, uint32_t pts);		
	bool GetRTPDataFromAudioQueue(uint8_t *pData, int *psize, uint32_t *pts);
	bool GetRTPDataFromVideoQueue(uint8_t *pData, int *psize, uint32_t *pts);
	void ClearAVQueue();
	bool isAudioQueueEmpty();
	bool isVideoQueueEmpty();
	int getVideoQueueLength();
	int getAudioQueueLength();

    private:
        bool GetRTPDataFromQueue(uint8_t *pData, int *psize, uint32_t *pts, Queue * mQueue);		
        void MTV_LOCK(pthread_mutex_t *mutex);		
        void MTV_UNLOCK(pthread_mutex_t *mutex);		
        bool copyToNode(Item *item, Node * pn, int size);
	bool copyToItem(Node * pn, Item *pi, int *psize);
	void QueueIni(Queue * mQueue);
	void QueueDeIni(Queue * mQueue);
	bool isEmptyQueue(Queue * mQueue);
        bool isFullQueue(Queue * mQueue);
        int itemCount(Queue * mQueue);
	bool enQueue(Item *item, int size, uint32_t pts, Queue * mQueue);
	bool deQueue(Item *pitem, int *psize, uint32_t *pts, Queue * mQueue);
	void emptyQueue(Queue * mQueue);
	int getQueueLength(Queue * mQueue);
	 
	Queue mAudioBufQueue;
 	Queue mVideoBufQueue;
	 
	//For logging
        //PVLogger* iLogger;	
	 
	//pthread_mutex_t mMtv_mutex;

#ifdef H264_NAL_TO_FILE
        FILE *mH264File;
        unsigned char* mH264Buffer;
        unsigned int mH264BufferLen;
#endif
};

#endif // STAGEFRIGHT_MTV_BUFMGR_H
