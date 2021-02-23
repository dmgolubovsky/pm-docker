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
* projectM-sdl
* This is an implementation of projectM using libSDL2
* 
* main.cpp
* Authors: Created by Mischa Spiegelmock on 6/3/15.
*
* 
*	RobertPancoast77@gmail.com :
* experimental Stereoscopic SBS driver functionality
* WASAPI looback implementation
* 
*
*/

#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h>
#include <sndfile.h>

#include "pmSND.hpp"

#if OGL_DEBUG
void DebugLog(GLenum source,
               GLenum type,
               GLuint id,
               GLenum severity,
               GLsizei length,
               const GLchar* message,
               const void* userParam) {

    /*if (type != GL_DEBUG_TYPE_OTHER)*/ 
	{
        std::cerr << " -- \n" << "Type: " <<
           type << "; Source: " <<
           source <<"; ID: " << id << "; Severity: " <<
           severity << "\n" << message << "\n";
       }
 }
#endif

// return path to config file to use
std::string getConfigFilePath(std::string datadir_path) {
  char* home = NULL;
  std::string projectM_home;
  std::string projectM_config = DATADIR_PATH;

  projectM_config = datadir_path;

#ifdef _MSC_VER
  home=getenv("USERPROFILE");
#else
  home=getenv("HOME");
#endif
    
  projectM_home = std::string(home);
  projectM_home += "/.projectM";
  
  // Create the ~/.projectM directory. If it already exists, mkdir will do nothing
#if defined _MSC_VER
  _mkdir(projectM_home.c_str());
#else
  mkdir(projectM_home.c_str(), 0755);
#endif
  
  projectM_home += "/config.inp";
  projectM_config += "/config.inp";
  
  std::ifstream f_home(projectM_home);
  std::ifstream f_config(projectM_config);
    
  if (f_config.good() && !f_home.good()) {
    std::ifstream f_src;
    std::ofstream f_dst;
      
    f_src.open(projectM_config, std::ios::in  | std::ios::binary);
    f_dst.open(projectM_home,   std::ios::out | std::ios::binary);
    f_dst << f_src.rdbuf();
    f_dst.close();
    f_src.close();
    return std::string(projectM_home);
  } else if (f_home.good()) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "created ~/.projectM/config.inp successfully\n");
    return std::string(projectM_home);
  } else if (f_config.good()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot create ~/.projectM/config.inp, using default config file\n");
    return std::string(projectM_config);
  } else {
    SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "Using implementation defaults, your system is really messed up, I'm surprised we even got this far\n");
	return "";
  }
}

//	-p preset name
//      -D datadir path

void usage(char *av0) {
    std::cerr << "Usage: " << av0 << " [-p preset] [-D datadir] audiofile" << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
#ifndef WIN32
srand((int)(time(NULL)));
#endif

    int opt;

    std::string presetName;
    std::string audioFile;
    std::string datadirPath;

    if (argc == 1) {
	usage(argv[0]);
    }

    while ((opt = getopt(argc, argv, "D:p:")) != -1) {
	switch (opt) {
	    case 'p':
		presetName = optarg;
		break;
	    case 'D':
		datadirPath = optarg;
		break;
	    default:
		usage(argv[0]);
	}
    }
    if ((optind > argc) || (argv[optind] == NULL)) {
	usage(argv[0]);
    }
    audioFile = argv[optind];
    if (audioFile.empty()) {
	usage(argv[0]);
    }


    // Open the audio file

    SF_INFO sfinfo;
    SNDFILE *sndf;

    sndf = sf_open(audioFile.c_str(), SFM_READ, &sfinfo);
    if (sndf == NULL) {
	std::cerr << "Error opening audio file: " << sf_strerror(NULL) << std::endl;
	exit(EXIT_FAILURE);
    }


#if UNLOCK_FPS
    setenv("vblank_mode", "0", 1);
#endif
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    if (! SDL_VERSION_ATLEAST(2, 0, 5)) {
        SDL_Log("SDL version 2.0.5 or greater is required. You have %i.%i.%i", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
        return 1;
    }

    SDL_Log("Opened audio file %s: %ld frames, %d channels, samplerate %d\n", audioFile.c_str(), sfinfo.frames, sfinfo.channels, sfinfo.samplerate);

    // default window size to usable bounds (e.g. minus menubar and dock)
    SDL_Rect initialWindowBounds;
#if SDL_VERSION_ATLEAST(2, 0, 5)
    // new and better
    SDL_GetDisplayUsableBounds(0, &initialWindowBounds);
#else
    SDL_GetDisplayBounds(0, &initialWindowBounds);
#endif
    int width = initialWindowBounds.w;
    int height = initialWindowBounds.h;

#ifdef USE_GLES
    // use GLES 2.0 (this may need adjusting)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

#else
	// Disabling compatibility profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#endif

    
    SDL_Window *win = SDL_CreateWindow("projectM", 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    
    SDL_GL_GetDrawableSize(win,&width,&height);
    
#if STEREOSCOPIC_SBS

	// enable stereo
	if (SDL_GL_SetAttribute(SDL_GL_STEREO, 1) == 0) 
	{
		SDL_Log("SDL_GL_STEREO: true");
	}

	// requires fullscreen mode
	SDL_ShowCursor(false);
	SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);

#endif


	SDL_GLContext glCtx = SDL_GL_CreateContext(win);


    SDL_Log("GL_VERSION: %s", glGetString(GL_VERSION));
    SDL_Log("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    SDL_Log("GL_VENDOR: %s", glGetString(GL_VENDOR));

    SDL_SetWindowTitle(win, "projectM Visualizer");
    
    SDL_GL_MakeCurrent(win, glCtx);  // associate GL context with main window
    int avsync = SDL_GL_SetSwapInterval(-1); // try to enable adaptive vsync
    if (avsync == -1) { // adaptive vsync not supported
        SDL_GL_SetSwapInterval(1); // enable updates synchronized with vertical retrace
    }

    
    projectMSND *app;
    
    std::string base_path = DATADIR_PATH;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Using data directory: %s\n", base_path.c_str());

    base_path = datadirPath.empty()?DATADIR_PATH:datadirPath;
    if (base_path.back() != '/') {
        base_path = base_path + "/";
    }
    SDL_Log("Config file not found, using built-in settings. Data directory=%s\n", base_path.c_str());

    // Get max refresh rate from attached displays to use as built-in max FPS.
    int i = 0;
    int maxRefreshRate = 0;
    SDL_DisplayMode current;
    for (i = 0; i < SDL_GetNumVideoDisplays(); ++i)
    {
	if (SDL_GetCurrentDisplayMode(i, &current) == 0)
	{
    	    if (current.refresh_rate > maxRefreshRate) maxRefreshRate = current.refresh_rate;
	}
    }
    if (maxRefreshRate <= 60) maxRefreshRate = 60;

    float heightWidthRatio = (float)height / (float)width;
    projectM::Settings settings;
    settings.windowWidth = width;
    settings.windowHeight = height;
    settings.meshX = 128;
    settings.meshY = settings.meshX * heightWidthRatio;
    settings.fps = 25; //maxRefreshRate;
    settings.smoothPresetDuration = 3; // seconds
    settings.presetDuration = 22; // seconds
    settings.hardcutEnabled = true;
    settings.hardcutDuration = 60;
    settings.hardcutSensitivity = 1.0;
    settings.beatSensitivity = 1.0;
    settings.aspectCorrection = 1;
    settings.shuffleEnabled = 1;
    settings.softCutRatingsEnabled = 1; // ???
    // get path to our app, use CWD or resource dir for presets/fonts/etc
    settings.presetURL = base_path + "presets";
    settings.menuFontURL = base_path + "fonts/Vera.ttf";
    settings.titleFontURL = base_path + "fonts/Vera.ttf";
    // init with settings
    app = new projectMSND(settings, 0);

    // Populate the app fields from command line args

    app->sndFileName = audioFile;
    app->presetName = presetName;
    app->sndFile = sndf;
    app->sndInfo = sfinfo;

    // If our config or hard-coded settings create a resolution smaller than the monitors, then resize the SDL window to match.
    if (height > app->getWindowHeight() || width > app->getWindowWidth()) {
        SDL_SetWindowSize(win, app->getWindowWidth(),app->getWindowHeight());
        SDL_SetWindowPosition(win, (width / 2)-(app->getWindowWidth()/2), (height / 2)-(app->getWindowHeight()/2));
    } else if (height < app->getWindowHeight() || width < app->getWindowWidth()) {
        // If our config is larger than our monitors resolution then reduce it.
        SDL_SetWindowSize(win, width, height);
        SDL_SetWindowPosition(win, 0, 0);
    }

    // Create a help menu specific to SDL
    std::string modKey = "CTRL";


    std::string sdlHelpMenu = "\n"
		"F1: This help menu""\n"
		"F3: Show preset name""\n"
		"F4: Show details and statistics""\n"
		"F5: Show FPS""\n"
		"L or SPACE: Lock/Unlock Preset""\n"
		"R: Random preset""\n"
		"N: Next preset""\n"
		"P: Previous preset""\n"
		"UP: Increase Beat Sensitivity""\n"
		"DOWN: Decrease Beat Sensitivity""\n"
		"Left Click: Drop Random Waveform on Screen""\n"
		"Right Click: Remove Random Waveform""\n" +
		modKey + "+Right Click: Remove All Random Waveforms""\n" +
		modKey + "+I: Audio Input (listen to next device)""\n" +
		modKey + "+M: Change Monitor""\n" +
		modKey + "+S: Stretch Monitors""\n" +
		modKey + "+F: Fullscreen""\n" +
		modKey + "+Q: Quit";
    app->setHelpText(sdlHelpMenu.c_str());
    app->init(win, &glCtx);

#if STEREOSCOPIC_SBS
	app->toggleFullScreen();
#endif

#if OGL_DEBUG && !USE_GLES
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugLog, NULL);
#endif

#if !FAKE_AUDIO && !WASAPI_LOOPBACK
	// get an audio input device
	if (app->openAudioInput())
		app->beginAudioCapture();
#endif


#if TEST_ALL_PRESETS
    uint buildErrors = 0;
    for(unsigned int i = 0; i < app->getPlaylistSize(); i++) {
        std::cout << i << "\t" << app->getPresetName(i) << std::endl;
        app->selectPreset(i);
        if (app->getErrorLoadingCurrentPreset()) {
            buildErrors++;
        }
    }

    if (app->getPlaylistSize()) {
        fprintf(stdout, "Preset loading errors: %d/%d [%d%%]\n", buildErrors, app->getPlaylistSize(), (buildErrors*100) / app->getPlaylistSize());
    }

    sf_close(app->sndFile)
    delete app;

    return PROJECTM_SUCCESS;
#endif

#if UNLOCK_FPS
    int32_t frame_count = 0;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    int64_t start = ( ((int64_t)now.tv_sec * 1000L) + (now.tv_nsec / 1000000L) );
#endif

    // standard main loop
    int fps = app->settings().fps;
    printf("fps: %d\n", fps);
    if (fps <= 0)
        fps = 60;
    const Uint32 frame_delay = 1000/fps;
    int asamples = app->sndInfo.samplerate / fps;
    std::cout << "Videoframe: " << frame_delay << " ms.; " << asamples << " audio samples/video frame" << std::endl;
    Uint32 last_time = SDL_GetTicks();
    while (! app->done) {
        app->renderFrame();

        // Get samples from the audio file as needed per frame
	
	float samplebuf[asamples * app->sndInfo.channels * sizeof(float)];
        sf_count_t nsamples = sf_readf_float(app->sndFile, samplebuf, asamples);
	std::cout << "read " << nsamples << " samples" << std::endl;
	if (nsamples < asamples) {
            sf_close(app->sndFile);
	    app->done = 1;
	}

#if FAKE_AUDIO
		app->fakeAudio  = true;
#endif
		// fakeAudio can also be enabled dynamically.
		if (app->fakeAudio )
		{
			app->addFakePCM();
		}

#if UNLOCK_FPS
        frame_count++;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        if (( ((int64_t)now.tv_sec * 1000L) + (now.tv_nsec / 1000000L) ) - start > 5000) {
            SDL_GL_DeleteContext(glCtx);
            delete(app);
            fprintf(stdout, "Frames[%d]\n", frame_count);
            exit(0);
        }
#else
        app->pollEvent();
        Uint32 elapsed = SDL_GetTicks() - last_time;
        if (elapsed < frame_delay)
            SDL_Delay(frame_delay - elapsed);
        last_time = SDL_GetTicks();
#endif
    }
    
    SDL_GL_DeleteContext(glCtx);
#if !FAKE_AUDIO
	if (!app->wasapi) // not currently using WASAPI, so we need to endAudioCapture.
		app->endAudioCapture();
#endif

    delete app;

    return PROJECTM_SUCCESS;
}


