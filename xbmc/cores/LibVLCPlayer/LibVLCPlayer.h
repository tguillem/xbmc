/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "FileItem.h"

#include "cores/IPlayer.h"
#include "windowing/WindowingFactory.h"

#include <memory>
#include <atomic>

namespace VLC
{
  class Instance;
  class MediaPlayer;
};

namespace KODI
{
  class CLibVLCPlayer : public IPlayer
  {
  public:
    explicit CLibVLCPlayer(IPlayerCallback& callback);
    ~CLibVLCPlayer() override;

    // implementation of IPlayer
    bool Initialize(TiXmlElement* pConfig) override;
    bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
    //virtual bool QueueNextFile(const CFileItem &file) override { return false; }
    //virtual void OnNothingToQueueNotify() override { } FIXME
    bool CloseFile(bool reopen = false) override;
    bool IsPlaying() const override;
    bool CanPause() override;
    void Pause() override;
    bool HasVideo() const override;
    bool HasAudio() const override;
    //bool HasGame() const override { return false; }
    //virtual bool HasRDS() const override { return false; }
    bool IsPassthrough() const override { return true /* FIXME */;}
    bool CanSeek() override;
    void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
    //virtual bool SeekScene(bool bPlus = true) override { return false; } FIXME
    void SeekPercentage(float fPercent = 0) override;
    float GetCachePercentage() override;
    void SetMute(bool bOnOff) override;
    void SetVolume(float volume) override;
    //virtual void SetDynamicRangeCompression(long drc) override { }
    //virtual bool CanRecord() override { return false; }
    //virtual bool IsRecording() override { return false; }
    //virtual bool Record(bool bOnOff) override { return false; }
    //virtual void SetAVDelay(float fValue = 0.0f) override { return; } FIXME
    //virtual float GetAVDelay() override { return 0.0f; } FIXME
    //virtual void SetSubTitleDelay(float fValue = 0.0f) override { } FIXME
    //virtual float GetSubTitleDelay() override { return 0.0f; } FIXME
    int GetSubtitleCount() override;
    int GetSubtitle() override;
    void GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info) override;
    void SetSubtitle(int iStream) override;
    bool GetSubtitleVisible() override;
    void SetSubtitleVisible(bool bVisible) override;
    //virtual void AddSubtitle(const std::string& strSubPath) override { } FIXME
    //virtual int GetAudioStreamCount() override { return 0; } FIXME
    //virtual int GetAudioStream() override { return -1; } FIXME 
    //virtual void SetAudioStream(int iStream) override { } FIXME
    //virtual void GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info) override { } FIXME
    //virtual int GetVideoStream() const override { return -1; } FIXME
    //virtual int GetVideoStreamCount() const override { return 0; } FIXME
    //virtual void GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo &info) override { } FIXME
    //virtual void SetVideoStream(int iStream) override { } FIXME 
    //virtual TextCacheStruct_t* GetTeletextCache() override { return NULL; } FIXME
    //virtual void LoadPage(int p, int sp, unsigned char* buffer) override { } FIXME
    //virtual std::string GetRadioText(unsigned int line) override { return ""; }
    //virtual int GetChapterCount() override { return 0; } FIXME
    //virtual int GetChapter() override { return -1; } FIXME
    //virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) override { return; } FIXME
    //virtual int64_t GetChapterPos(int chapterIdx = -1) override { return 0; } FIXME
    //virtual int SeekChapter(int iChapter) override { return -1; } FIXME
    //virtual float GetActualFPS() override { return 0.0f; } FIXME
    void SeekTime(int64_t iTime = 0) override;
    bool SeekTimeRelative(int64_t iTime) override;
    //virtual void SetTotalTime(int64_t time) override { } // Only used by Air Tunes Server
    //virtual int GetSourceBitrate() override { return 0; }
    void SetSpeed(float speed) override;
    bool IsCaching() const override;
    //virtual int GetCacheLevel() const override { return -1; } FIXME
    bool IsInMenu() const override;
    bool HasMenu() const override;
    //virtual void DoAudioWork() override { }
    bool OnAction(const CAction &action) override;
    std::string GetPlayerState() override;
    bool SetPlayerState(const std::string& state) override;
    //virtual std::string GetPlayingTitle() override { return ""; } FIXME
    //virtual bool SwitchChannel(const PVR::CPVRChannelPtr &channel) override { return false; }
    //virtual void GetAudioCapabilities(std::vector<int> &audioCaps) override { audioCaps.assign(1,IPC_AUD_ALL); }
    //virtual void GetSubtitleCapabilities(std::vector<int> &subCaps) override { subCaps.assign(1,IPC_SUBS_ALL); }
    void FrameMove() override;
    void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
    void FlushRenderer() override;
    //void SetRenderViewMode(int mode) override { } // Must go through render callback
    //float GetRenderAspectRatio() override { return 1.0f; }
    void TriggerUpdateResolution() override;
    bool IsRenderingVideo() override;
    //bool Supports(EINTERLACEMETHOD method) override { return false; } // Must go through render callback
    //EINTERLACEMETHOD GetDeinterlacingMethodDefault() override { return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE; } // Must go through render callback
    //bool Supports(ESCALINGMETHOD method) override { return false; } // Must go through render callback
    //bool Supports(ERENDERFEATURE feature) override { return false; } // Must go through render callback
    //unsigned int RenderCaptureAlloc() override { return 0; }
    //void RenderCaptureRelease(unsigned int captureId) override { }
    //void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags) override { }
    //bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size) override { return false; }
  private:
    void InitEvents();

    VLC::Instance *m_lvlc;
    VLC::MediaPlayer *m_mp;
    CFileItem m_playing_file;
    std::atomic<float> m_cache;
    std::atomic<bool> m_playing;
    std::atomic<bool> m_is_360;
    int m_subtitle_invisible;
    GLuint m_texid;
  };
}
