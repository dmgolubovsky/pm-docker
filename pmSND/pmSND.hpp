/**
* projectM -- Milkdrop-esque visualisation SDK
* Copyright (C)2003-2019 projectM Team
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
* See 'LICENSE.txt' included within this release
*
* projectM-sndf
* This is an implementation of projectM using libSDL2 and reading audio from a file
* 
* pmSND.hpp 
* Authors: Created by Mischa Spiegelmock on 2017-09-18.
*          Modified by Dmitry Golubovsky on 2021-02-21.
*
*/


#ifndef pmSND_hpp
#define pmSND_hpp


#define FAKE_AUDIO          0
// ----------------------------
#define TEST_ALL_PRESETS    0
#define STEREOSCOPIC_SBS    0

#include "projectM-opengl.h"
#include <projectM.hpp>
#include <sdltoprojectM.h>
#include <iostream>
#include <sys/stat.h>
#include <sndfile.h>


#include <SDL2/SDL.h>

// DATADIR_PATH should be set by the root Makefile if this is being
// built with autotools.
#ifndef DATADIR_PATH
    #ifdef DEBUG
        #define DATADIR_PATH "."
        #warning "DATADIR_PATH is not defined - falling back to ./"
    #else
        #define DATADIR_PATH "/usr/local/share/projectM"
    #endif
#endif

class projectMSND : public projectM {
public:


    bool done;
    bool mouseDown = false;
    bool wasapi = false; // Used to track if wasapi is currently active. This bool will allow us to run a WASAPI app and still toggle to microphone inputs.
    bool fakeAudio = false; // Used to track fake audio, so we can turn it off and on.
    bool stretch = false; // used for toggling stretch mode
    projectMSND(Settings settings, int flags);
    projectMSND(std::string config_file, int flags);
    void init(SDL_Window *window, SDL_GLContext *glCtx, const bool renderToTexture = false);
    int openAudioInput();
    int toggleAudioInput();
    int initAudioInput();
    void beginAudioCapture();
    void endAudioCapture();
    void stretchMonitors();
    void nextMonitor();
    void toggleFullScreen();
    void resize(unsigned int width, unsigned int height);
    void touch(float x, float y, int pressure, int touchtype = 0);
    void touchDrag(float x, float y, int pressure);
    void touchDestroy(float x, float y);
    void touchDestroyAll();
    void setHelpText(const std::string& theValue);
    void renderFrame();
    void pollEvent();
    void maximize();
    bool keymod = false;
    std::string sndFileName;
    std::string presetName;
    std::string getActivePresetName();
    SNDFILE *sndFile;
    SF_INFO sndInfo;
    void addFakePCM();
    
    virtual void presetSwitchedEvent(bool isHardCut, size_t index) const;

private:
    SDL_Window *win;
    SDL_GLContext *glCtx;
    bool isFullScreen;
    SDL_AudioDeviceID audioInputDevice;
    unsigned int width, height;
    bool renderToTexture;
    GLuint programID = 0;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    GLuint textureID = 0;

    // audio input device characteristics
    unsigned int NumAudioDevices;
    unsigned int CurAudioDevice;
    unsigned short audioChannelsCount;
    unsigned short audioSampleRate;
    unsigned short audioSampleCount;
    SDL_AudioFormat audioFormat;
    SDL_AudioDeviceID audioDeviceID;
    SDL_AudioDeviceID selectedAudioDevice;

    static void audioInputCallbackF32(void *userdata, unsigned char *stream, int len);
    static void audioInputCallbackS16(void *userdata, unsigned char *stream, int len);

    void scrollHandler(SDL_Event *);
    void keyHandler(SDL_Event *);
    void renderTexture();
};


#endif /* pmSND_hpp */
