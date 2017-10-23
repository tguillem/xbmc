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

#include "LibVLCPlayer.h"

#include "Application.h"
#include "URL.h"

#include "cores/DataCacheCore.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "rendering/RenderSystem.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"

#include <vlcpp/vlc.hpp>

#define LIBVLC_VERBOSE "1"

using namespace KODI;
using namespace VLC;

CLibVLCPlayer::CLibVLCPlayer(IPlayerCallback& callback) :
  IPlayer(callback), m_playing_file(),
  m_cache(0.0f), m_playing(false), m_is_360(false),
  m_subtitle_invisible(-1)
{
  static const char *args[] = {
    "--verbose", LIBVLC_VERBOSE
  };
  m_lvlc = new Instance(sizeof(args) / sizeof(*args), args);
  m_mp = new MediaPlayer(*m_lvlc);
}

CLibVLCPlayer::~CLibVLCPlayer()
{
  CloseFile();
  delete m_mp;
  delete m_lvlc;
}

void CLibVLCPlayer::InitEvents()
{
  MediaPlayerEventManager &em = m_mp->eventManager();

  em.onBuffering([this](float buffering)
  {
    m_cache = buffering;
  });

  em.onPlaying([this]
  {
      m_callback.OnPlayBackResumed();
  });

  em.onPaused([this]
  {
    m_callback.OnPlayBackPaused();
  });

  em.onStopped([this]
  {
    m_callback.OnPlayBackStopped();
    m_playing = false;
  });

  em.onEndReached([this]
  {
    m_callback.OnPlayBackEnded();
    m_playing = false;
  });

  em.onEncounteredError([this]
  {
    m_callback.OnPlayBackError();
    m_playing = false;
  });

  em.onTimeChanged([this](libvlc_time_t time)
  {
    CDataCacheCore::GetInstance().SetPlayTimes(0, time, 0, m_mp->length());
  });

  em.onPositionChanged([this](float pos)
  {
    if (m_mp->length() == -1)
     CDataCacheCore::GetInstance().SetPlayTimes(0, pos * 100, 0, 100);
  });

  em.onVout([this](int vout)
  {
    g_windowManager.CloseDialogs(true);
  });

  em.onESAdded([this](libvlc_track_type_t type, int)
  {
    MediaPtr media = m_mp->media();
    for (MediaTrack track : media->tracks())
    {
      if (track.type() == MediaTrack::Type::Video
       && track.projection() != MediaTrack::Projection::Rectangular)
      {
        m_is_360 = true;
        break;
      }
    }
  });

  em.onESDeleted([this](libvlc_track_type_t type, int)
  {
    if (!m_is_360)
      return;
    MediaPtr media = m_mp->media();
    bool has_360 = false;
    for (MediaTrack track : media->tracks())
    {
      if (track.type() == MediaTrack::Type::Video
       && track.projection() != MediaTrack::Projection::Rectangular)
      {
        has_360 = true;
        break;
      }
    }
    m_is_360 = has_360;
  });

  /*
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackSeek(int64_t iTime, int64_t seekOffset) {};
  virtual void OnPlayBackSeekChapter(int iChapter) {};
  virtual void OnPlayBackSpeedChanged(int iSpeed) {};
*/
}

bool CLibVLCPlayer::Initialize(TiXmlElement* pConfig)
{
#ifndef HAS_GLX
#error only setGLXContext is implemented via libvlc
#endif
  if (!g_application.IsCurrentThread())
    return false;

  CWinSystemX11GLContext *rendering = &g_Windowing;
  Display *dpy = rendering->GetDisplay();
  GLXContext glxc = rendering->GetGlxContext();
  if (!dpy || !glxc)
  {
    CLog::Log(LOGERROR, "Inbalid X11 display or GLXContext");
    return false;
  }

  InitEvents();

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &m_texid);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glDisable(GL_TEXTURE_2D);

  m_mp->setGLXContext(dpy, glxc, m_texid);

  CLog::Log(LOGINFO, "Using LibVLC as Player");
  return true;
}

bool CLibVLCPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  // FIXME handle options

  CDataCacheCore::GetInstance().Reset();
  CDataCacheCore::GetInstance().SetVideoRender(false);
  CDataCacheCore::GetInstance().SetGuiRender(true);

  CDataCacheCore::GetInstance().SetVideoDecoderName("vlc", true);

  m_playing_file = file;
  CURL url = file.GetURL();
  Media m = Media(*m_lvlc, url.Get(),
                  url.GetProtocol().empty() ? Media::FromType::FromPath
                                            : Media::FromType::FromLocation);
  m_mp->setMedia(m);

  if (!m_mp->play())
  {
    m_callback.OnPlayBackError();
    m_playing_file.Reset();
    return false;
  }
  m_callback.OnPlayBackStarted(m_playing_file);
  m_playing = true;
  return true;
}

bool CLibVLCPlayer::CloseFile(bool reopen /* = false */)
{
  m_mp->stop();
  m_cache = 0.0f;
  m_callback.OnPlayBackEnded();
  m_playing_file.Reset();
  m_playing = false;
  m_is_360 = false;
  m_subtitle_invisible = -1;
  return true;
}

bool CLibVLCPlayer::IsPlaying() const
{
  return m_playing;
}

bool CLibVLCPlayer::CanPause()
{
  return m_mp->canPause();
}

void CLibVLCPlayer::Pause()
{
  m_mp->pause();
}

bool CLibVLCPlayer::HasVideo() const
{
  return m_mp->videoTrackCount() > 0;
}

bool CLibVLCPlayer::HasAudio() const
{
  return m_mp->audioTrackCount() > 0;
}

bool CLibVLCPlayer::CanSeek()
{
  return m_mp->isSeekable();
}

void CLibVLCPlayer::Seek(bool bPlus /* = true */,
                         bool bLargeStep /* = false */,
                         bool bChapterOverride /* = false */)
{
}

void CLibVLCPlayer::SeekPercentage(float fPercent /* = 0 */)
{
  m_mp->setPosition(fPercent / 100.);
}

float CLibVLCPlayer::GetCachePercentage()
{
  return m_cache;
}

void CLibVLCPlayer::SetMute(bool bOnOff)
{
  m_mp->setMute(bOnOff);
}

void CLibVLCPlayer::SetVolume(float volume)
{
  m_mp->setVolume(volume * 100);
}

int CLibVLCPlayer::GetSubtitleCount()
{
  return m_mp->spuCount();
}

int CLibVLCPlayer::GetSubtitle()
{
  return m_mp->spu();
}

void CLibVLCPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info)
{
  for (TrackDescription track : m_mp->spuDescription())
  {
    if (index == track.id())
    {
      info.name = track.name();
      break;
    }
  }
}

void CLibVLCPlayer::SetSubtitle(int iStream)
{
   m_mp->setSpu(iStream);
}

bool CLibVLCPlayer::GetSubtitleVisible()
{
  return m_mp->spu() != -1;
}

void CLibVLCPlayer::SetSubtitleVisible(bool bVisible)
{
  if (!bVisible)
  {
    m_subtitle_invisible = m_mp->spu();
    m_mp->setSpu(-1);
  }
  else
  {
    m_mp->setSpu(m_subtitle_invisible);
    m_subtitle_invisible = -1;
  }
}

void CLibVLCPlayer::SeekTime(int64_t iTime /* = 0 */)
{
  int64_t seekOffset = iTime - m_mp->time();

  m_mp->setTime(iTime);
  m_callback.OnPlayBackSeek(iTime, seekOffset);
}

bool CLibVLCPlayer::SeekTimeRelative(int64_t iTime)
{
  SeekTime(m_mp->time() + iTime);
}

void CLibVLCPlayer::SetSpeed(float speed)
{
  m_mp->setRate(speed);
  m_callback.OnPlayBackSpeedChanged(speed);
}

bool CLibVLCPlayer::IsCaching() const
{
  return m_cache < 100.f;
}

bool CLibVLCPlayer::IsInMenu() const
{
  return m_is_360; // fake menu to send navigation to VLC
}

bool CLibVLCPlayer::HasMenu() const
{
  return false; /* FIXME */
}

bool CLibVLCPlayer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_MOVE_LEFT:
    m_mp->navigate(libvlc_navigate_left);
    break;
  case ACTION_MOVE_RIGHT:
    m_mp->navigate(libvlc_navigate_right);
    break;
  case ACTION_MOVE_UP:
    m_mp->navigate(libvlc_navigate_up);
    break;
  case ACTION_MOVE_DOWN:
    m_mp->navigate(libvlc_navigate_down);
    break;
  default:
    break;
  }

  return false;
}

std::string CLibVLCPlayer::GetPlayerState()
{
  return "";
}

bool CLibVLCPlayer::SetPlayerState(const std::string& state)
{
  return false;
}

void CLibVLCPlayer::FrameMove()
{
}

void CLibVLCPlayer::Render(bool clear, uint32_t alpha /* = 255 */, bool gui /* = true */)
{
  // Performed by callbacks
  glEnable(GL_BLEND);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texid);

  glBegin(GL_QUADS);
  glVertex3f(-1., -1., 0.); glTexCoord2f(1., 0.);
  glVertex3f( 1., -1., 0.); glTexCoord2f(1., 1.);
  glVertex3f( 1.,  1., 0.); glTexCoord2f(0., 1.);
  glVertex3f(-1.,  1., 0.); glTexCoord2f(0., 0.);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  glBlendFunc(GL_ONE, GL_ZERO);
  glFlush();
}

void CLibVLCPlayer::FlushRenderer()
{
}

void CLibVLCPlayer::TriggerUpdateResolution()
{
}

bool CLibVLCPlayer::IsRenderingVideo()
{
  return m_mp->hasVout();
}
