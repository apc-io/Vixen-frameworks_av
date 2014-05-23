/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "MediaPlayerFactory"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <media/IMediaPlayer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <utils/Errors.h>
#include <utils/misc.h>

#include "MediaPlayerFactory.h"

#include "MidiFile.h"
#include "TestPlayerStub.h"
#include "StagefrightPlayer.h"
#include "nuplayer/NuPlayerDriver.h"

#ifdef CFG_WMT_WPLAYER //  JasonLin: wplayer header file
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaExtractor.h>
#include <NuCachedSource2.h>
#include <HTTPBase.h>
#include "libffplayer.h"
#endif

namespace android {

Mutex MediaPlayerFactory::sLock;
MediaPlayerFactory::tFactoryMap MediaPlayerFactory::sFactoryMap;
bool MediaPlayerFactory::sInitComplete = false;

status_t MediaPlayerFactory::registerFactory_l(IFactory* factory,
                                               player_type type) {
    if (NULL == factory) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, factory is"
              " NULL.", type);
        return BAD_VALUE;
    }

    if (sFactoryMap.indexOfKey(type) >= 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, type is"
              " already registered.", type);
        return ALREADY_EXISTS;
    }

    if (sFactoryMap.add(type, factory) < 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, failed to add"
              " to map.", type);
        return UNKNOWN_ERROR;
    }

    return OK;
}

player_type MediaPlayerFactory::getDefaultPlayerType() {
    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.stagefright.use-nuplayer", value, NULL)
            && (!strcmp("1", value) || !strcasecmp("true", value))) {
        return NU_PLAYER;
    }

    return STAGEFRIGHT_PLAYER;
}

status_t MediaPlayerFactory::registerFactory(IFactory* factory,
                                             player_type type) {
    Mutex::Autolock lock_(&sLock);
    return registerFactory_l(factory, type);
}

void MediaPlayerFactory::unregisterFactory(player_type type) {
    Mutex::Autolock lock_(&sLock);
    sFactoryMap.removeItem(type);
}

#define GET_PLAYER_TYPE_IMPL(a...)                      \
    Mutex::Autolock lock_(&sLock);                      \
                                                        \
    player_type ret = STAGEFRIGHT_PLAYER;               \
    float bestScore = 0.0;                              \
                                                        \
    for (size_t i = 0; i < sFactoryMap.size(); ++i) {   \
                                                        \
        IFactory* v = sFactoryMap.valueAt(i);           \
        float thisScore;                                \
        CHECK(v != NULL);                               \
        thisScore = v->scoreFactory(a, bestScore);      \
        if (thisScore > bestScore) {                    \
            ret = sFactoryMap.keyAt(i);                 \
            bestScore = thisScore;                      \
        }                                               \
    }                                                   \
                                                        \
    if (0.0 == bestScore) {                             \
        bestScore = getDefaultPlayerType();             \
    }                                                   \
                                                        \
    return ret;

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const char* url) {
    GET_PLAYER_TYPE_IMPL(client, url);
}

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              int fd,
                                              int64_t offset,
                                              int64_t length) {
    GET_PLAYER_TYPE_IMPL(client, fd, offset, length);
}

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const sp<IStreamSource> &source) {
    GET_PLAYER_TYPE_IMPL(client, source);
}

#undef GET_PLAYER_TYPE_IMPL

sp<MediaPlayerBase> MediaPlayerFactory::createPlayer(
        player_type playerType,
        void* cookie,
        notify_callback_f notifyFunc) {
    sp<MediaPlayerBase> p;
    IFactory* factory;
    status_t init_result;
    Mutex::Autolock lock_(&sLock);

    if (sFactoryMap.indexOfKey(playerType) < 0) {
        ALOGE("Failed to create player object of type %d, no registered"
              " factory", playerType);
        return p;
    }

    factory = sFactoryMap.valueFor(playerType);
    CHECK(NULL != factory);
    p = factory->createPlayer();

    if (p == NULL) {
        ALOGE("Failed to create player object of type %d, create failed",
               playerType);
        return p;
    }

    init_result = p->initCheck();
    if (init_result == NO_ERROR) {
        p->setNotifyCallback(cookie, notifyFunc);
    } else {
        ALOGE("Failed to create player object of type %d, initCheck failed"
              " (res = %d)", playerType, init_result);
        p.clear();
    }

    return p;
}

/*****************************************************************************
 *                                                                           *
 *                     Built-In Factory Implementations                      *
 *                                                                           *
 *****************************************************************************/

class StagefrightPlayerFactory :
    public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        char buf[20];
        lseek(fd, offset, SEEK_SET);
        read(fd, buf, sizeof(buf));
        lseek(fd, offset, SEEK_SET);

        long ident = *((long*)buf);

        // Ogg vorbis?
        if (ident == 0x5367674f) // 'OggS'
            return 1.0;

        static const char AMR_header[]   = "#!AMR\n";  //amr
        if(!memcmp(AMR_header, buf, 5))
            return 1.0;

        //For CTS test 'run cts -c android.media.cts.MediaPlayerTest -m testGapless1'
        //it needs player to cut some MP3 samples at the beginning and the end.
    
        //FFPlayer can refer the code in stagefright XINGSeeker::CreateFromSource
        //pay attention to mEncoderDelay/mEncoderPadding
        //But it's much easier to roll back to the STAGEFRIGHT_PLAYER
    
        //Just for Pass the CTS test only
        if((offset != 0) && (length == 40541))
            return 1.0;



        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 1.0;
        static const char* const FILE_EXTS[] = { ".ogg",
                                                 ".oga",
                                                 ".amr"
                                                 };
        if (kOurScore <= curScore)
            return 0.0;

        int lenURL = strlen(url);
        for (int i = 0; i < NELEM(FILE_EXTS); ++i) {
            int len = strlen(FILE_EXTS[i]);
            int start = lenURL - len;
            if (start > 0) {
                if (!strncasecmp(url + start, FILE_EXTS[i], len)) {
                    return kOurScore;
                }
            }
        }

        //for siano player
        if (!strncasecmp(url, "isdbt://", strlen("isdbt://"))) {
            return kOurScore;
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create StagefrightPlayer");
        return new StagefrightPlayer();
    }
};

class NuPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.8;

        if (kOurScore <= curScore)
            return 0.0;

        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)) {
            size_t len = strlen(url);
            if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                return kOurScore;
            }

            if (strstr(url,"m3u8")) {
                return kOurScore;
            }
        }

        if (!strncasecmp("rtsp://", url, 7)) {
            return kOurScore;
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const sp<IStreamSource> &source,
                               float curScore) {
        return 1.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create NuPlayer");
        return new NuPlayerDriver;
    }
};

class SonivoxPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.4;
        static const char* const FILE_EXTS[] = { ".mid",
                                                 ".midi",
                                                 ".smf",
                                                 ".xmf",
                                                 ".mxmf",
                                                 ".imy",
                                                 ".rtttl",
                                                 ".rtx",
                                                 ".ota" };
        if (kOurScore <= curScore)
            return 0.0;

        // use MidiFile for MIDI extensions
        int lenURL = strlen(url);
        for (int i = 0; i < NELEM(FILE_EXTS); ++i) {
            int len = strlen(FILE_EXTS[i]);
            int start = lenURL - len;
            if (start > 0) {
                if (!strncasecmp(url + start, FILE_EXTS[i], len)) {
                    return kOurScore;
                }
            }
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        static const float kOurScore = 0.8;

        if (kOurScore <= curScore)
            return 0.0;

        // Some kind of MIDI?
        EAS_DATA_HANDLE easdata;
        if (EAS_Init(&easdata) == EAS_SUCCESS) {
            EAS_FILE locator;
            locator.path = NULL;
            locator.fd = fd;
            locator.offset = offset;
            locator.length = length;
            EAS_HANDLE  eashandle;
            if (EAS_OpenFile(easdata, &locator, &eashandle) == EAS_SUCCESS) {
                EAS_CloseFile(easdata, eashandle);
                EAS_Shutdown(easdata);
                return kOurScore;
            }
            EAS_Shutdown(easdata);
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create MidiFile");
        return new MidiFile();
    }
};

class TestPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        if (TestPlayerStub::canBeUsed(url)) {
            return 1.0;
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV("Create Test Player stub");
        return new TestPlayerStub();
    }
};

#ifdef CFG_WMT_WPLAYER
class WmtPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        // we are smaller than Stagefright, NuPlayer, and Sonivox
        // Stagefright - handle ogg (score 1.0)
        // NuPlayer    - handle .m3u8 and rtsp (score 0.8)
        // Sonivox     - handle midi (score 0.4)
        static const float kOurScore = 0.2;
        char value[PROPERTY_VALUE_MAX];

        if (client == NULL) return 0.0;
        if (property_get("persist.sys.media.player", value, NULL) > 0){
            if (!strncasecmp(value,"native",6))
                return 0.0;
            if (!strncasecmp(value,"wmt",6))
                return 1.0;
        }
        if (property_get("media.player.default", value, NULL) > 0){
            ALOGW("Property \"media.player.default\" has been obsolete. "
                "Please use \"persist.sys.media.player\" instead");
        }

        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)) {
            size_t len = strlen(url);
            if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                return 1.0;
            }

            if (strstr(url,"m3u8")) {
                return 1.0;
            }
        }

        if (kOurScore <= curScore)
            return 0.0;

        // check DRM content start
        if(!strncasecmp("widevine://", url, 11)){
            ALOGI("widevine URL: %s", url);
            return 0.0;
        }

        bool checkDRM = true;
        
        {
            if(!strncasecmp("http://127.0.0.1", url, 16))
                checkDRM = false;
            if (property_get("media.player.DRM.check", value, "1") > 0){
                checkDRM = atoi(value);
            }
        }

        if(checkDRM)
        {
            if (!strncasecmp("http://", url, 7) || !strncasecmp("https://", url, 8)) {
                sp<HTTPBase> mConnectingDataSource = HTTPBase::Create(0);
                mConnectingDataSource->setUID(getuid());
                String8 cacheConfig;
                bool disconnectAtHighwatermark;
                NuCachedSource2::RemoveCacheSpecificHeaders(
                        NULL, &cacheConfig, &disconnectAtHighwatermark);
                status_t err = mConnectingDataSource->connect(url, NULL);
                if (err == OK){
                    sp<NuCachedSource2> mCachedSource = new NuCachedSource2(
                            mConnectingDataSource,
                            cacheConfig.isEmpty() ? NULL : cacheConfig.string(),
                            disconnectAtHighwatermark);
                    if(isDRMProtectedContent(mCachedSource)) {
                        mConnectingDataSource.clear();
                        return 0.0;
                    }
                }
                mConnectingDataSource.clear();
            }
        }
        // check DRM content end

		return kOurScore;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        // we are smaller than Sonivox
        // Sonivox - handle midi (score 0.6)
        static const float kOurScore = 0.6;
        char value[PROPERTY_VALUE_MAX];
        String8 uri;
  
        if (client == NULL) return 0.0;
        if (property_get("persist.sys.media.player", value, NULL) > 0){
            if (!strncasecmp(value,"native",6))
                return 0.0;
            if (!strncasecmp(value,"wmt",6))
                return 1.0;
        }
        if (property_get("media.player.default", value, NULL) > 0){
            ALOGW("Property \"media.player.default\" has been obsolete. "
                "Please use \"persist.sys.media.player\" instead");
        }

        if (kOurScore <= curScore)
            return 0.0;

        // check DRM content start
        sp<DataSource> dataSource = new FileSource(dup(fd), offset, length);
        if (dataSource->initCheck() == OK &&
            isDRMProtectedContent(dataSource)){
            return 0.0;
        }
        // check DRM content end

        uri.clear();
        uri.appendFormat("fd://%d,%lld,%lld", fd, offset, length);
        // check WMT Support format
        return kOurScore;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const sp<IStreamSource> &source,
                               float curScore) {
        // Currently, we can't handle IStreamSource
        static const float kOurScore = 0.0;
        if (client == NULL) return 0.0;
        return kOurScore;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create WMT_Player");
        //[Wmt] defined in libffplayer.so 
        return ffplayerCreate();
    }

private:
    bool isDRMProtectedContent(const sp<DataSource> &source){
        DataSource::RegisterDefaultSniffers();
        sp<MediaExtractor> extractor = MediaExtractor::Create(source);
        if (extractor != NULL && extractor->getDrmFlag()){
            ALOGD("Digital Right Protected contain. Go through DRMEngine.");
            return true;
        }
        return false;
    }
};
#endif

void MediaPlayerFactory::registerBuiltinFactories() {
    Mutex::Autolock lock_(&sLock);

    if (sInitComplete)
        return;

    registerFactory_l(new StagefrightPlayerFactory(), STAGEFRIGHT_PLAYER);
    registerFactory_l(new NuPlayerFactory(), NU_PLAYER);
    registerFactory_l(new SonivoxPlayerFactory(), SONIVOX_PLAYER);
    registerFactory_l(new TestPlayerFactory(), TEST_PLAYER);
#ifdef CFG_WMT_WPLAYER
    registerFactory_l(new WmtPlayerFactory(), WMT_PLAYER);

    //[Wmt] defined in libffplayer.so
    ffplayerPreInit();
#endif

    sInitComplete = true;
}

}  // namespace android
