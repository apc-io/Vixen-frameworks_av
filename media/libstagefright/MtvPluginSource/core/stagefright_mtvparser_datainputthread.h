#ifndef STAGEFRIGHT_MTV_PARSER_DATAINPUTTHREAD_H_INCLUDED
#define STAGEFRIGHT_MTV_PARSER_DATAINPUTTHREAD_H_INCLUDED

#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/List.h>
#include <pthread.h>
#include <sys/time.h>

#include <poll.h>

#include "stagefright_mtvbufmgr.h"

enum MediaPlayerEngineIndication {
    UNKNOWN_IDICATION,
    REQUEST_SHAREDBUFFER_FD,
    GOT_SHAREDBUFFER_FD,
    MEDIA_SERVER_ALIVE
};

class DataInputThd
{
public:
    //static DataInputThd * instance();

    DataInputThd();
    ~DataInputThd();
    
    bool getVideoBuffer(uint8_t *pData, int *psize, uint32_t *pts);
    bool getAudioBuffer(uint8_t *pData, int *psize, uint32_t *pts);

    static void startReceive(DataInputThd& recvThd);
    static void stopReceive(DataInputThd& recvThd);
  
    int iVidSock, iAudSock, iCtrlSock;
    int iCtrlNewSock1, iCtrlNewSock2, iCtrlNewSock;
    bool mCtrlConnected;
    unsigned char* revbuffer; 	
    StageFrightMtvBufMgr *mMtvBufMgr;

    // using 'pipe' to clear 'select' block when need to exit thread.
    int mPipe[2];


    // For logging
    //PVLogger* iLogger;	   

    static DataInputThd* gInstance;
    static DataInputThd* getInstance();

    bool mIsAudioDataReceived;
    bool mIsVideoDataReceived;
  	
    bool isAVBufferEmpty();
    bool isAudioBufferedNFrame(int frameNum); //is there frameNum frames in Audio Buffer. 1 frame ~= 25 items.
	
    int sentCommandtoMtv(int code);

    bool mIntentStop;

    void setIntentToStop(bool value);
    bool isIntentToStop();
    
    bool mGetFormat;
    
    void setGetFormat(bool value);
    bool isGetFormat();
    bool checkParaFrame(unsigned char* buffer);
    
    bool mNeedSpsPps;
    
    void setNeedSpsPps(bool value);
    bool isNeedSpsPps();
    bool checkSpsPps(unsigned char* buffer);

    unsigned int mCurAudioPTS;
    
    void setCurAudioPTS(unsigned int pts);
    unsigned int getCurAudioPTS();
    
    unsigned int mPreAudioPTS;
    
    void setPreAudioPTS(unsigned int pts);
    unsigned int getPreAudioPTS();
    
    struct timeval mLastOpencoreFetch;
    
    static int mFdCommand;
    static bool mCmdThreadStarted;
    static void* recvCommandThread(void* data);
    static int onCommandFd(int fd);
    static int sendIndication(int fd, MediaPlayerEngineIndication indication, const char* cookie);
    static int doSendIndication(int fd, MediaPlayerEngineIndication indication, const char* cookie);
    
    int mRecvCmdSocket;
    int startRecvCommandThread();
    int stopRecvCommandThread();
    
    
private:
    int init();
    int uninit();
    bool initSocket();	
    void CloseSocket();	
    void waitforEmpty();	
    static void*  readRtp_threadfunc(void* arg);
    static void*  readSocketRtp_threadfunc(void* arg);	
    char getStamp(unsigned char *buffer);

private:
    static pthread_t tid;
    
    int iAudioCount;

    bool bAudioFlag, bVideoFlag;
    bool bIsFirst;
    bool bPlay;	

    sockaddr_in iAudAddr;
    sockaddr_in iVidAddr;
    sockaddr_in iCtrlAddr;

    pthread_cond_t stopRevCond; //stop receive condition

private:
    bool openSM;
    //MtvOriShareBuffer* pSM;

    int openShareMemory(int sharedFd);
    int initShareMemory();
    int clearShareMemory();

    static void* readShareMemory_threadfunc(void* arg);
    //int readShareBuffer(unsigned char* sharemem);
  	
};

#endif //STAGEFRIGHT_MTV_PARSER_DATAINPUTTHREAD_H_INCLUDED

