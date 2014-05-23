
#ifndef MTV_LOG_H_
#define MTV_LOG_H_

#include <utils/Log.h>

#ifndef LOGD
#ifdef ALOGD
#define LOGD ALOGD
#else
#define LOGD
#endif //#ifdef ALOG
#endif //#ifndef LOGD

#ifndef LOGE
#ifdef ALOGE
#define LOGE ALOGE
#else
#define LOGE
#endif //#ifdef ALOGE
#endif //#ifndef LOGE

#ifndef LOGI
#ifdef ALOGI
#define LOGI ALOGI
#else
#define LOGI
#endif //#ifdef ALOGI
#endif //#ifndef LOGI

#ifndef LOGV
#ifdef ALOGV
#define LOGV ALOGV
#else
#define LOGV
#endif //#ifdef ALOGV
#endif //#ifndef LOGV

#ifndef LOGW
#ifdef ALOGW
#define LOGW ALOGW
#else
#define LOGW
#endif //#ifdef ALOGW
#endif //#ifndef LOGW

#ifndef CHECK
#define CHECK
#endif

#ifndef CHECK_EQ
#define CHECK_EQ
#endif

#endif // MTV_LOG_H_
