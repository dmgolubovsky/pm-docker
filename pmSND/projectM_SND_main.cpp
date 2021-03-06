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
#include <time.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

#include "pmSND.hpp"


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
//      -d playback audio device
//      -b seconds before audio starts
//      -a seconds after audio ends
//      -v video name to be passed to ffmpeg
//      -f <fullscreen>
//      -x <debug openGL>

void usage(char *av0) {
    std::cerr << "Usage: " << av0 << " [-p preset] [-D datadir] [-d device] [-b before] [-a after] [-s beatsens] [-v video] [-fx] audiofile" << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
#ifndef WIN32
srand((int)(time(NULL)));
#endif

    int opt;
    long int before = 0, after = 0;
    float beatsens = 1.0;

    std::string presetName;
    std::string audioFile;
    std::string datadirPath;
    std::string pcmDevice;
    std::string videoName;
    bool fullscrn = false;
    bool dbgogl = false;

    if (argc == 1) {
	usage(argv[0]);
    }

    while ((opt = getopt(argc, argv, "v:s:a:b:d:D:p:fx")) != -1) {
	char *endptr;
	switch (opt) {
	    case 'x':
		dbgogl = true;
		break;
	    case 'f':
		fullscrn = true;
		break;
	    case 'p':
		presetName = optarg;
		break;
	    case 'D':
		datadirPath = optarg;
		break;
	    case 'd':
		pcmDevice = optarg;
		break;
	    case 'v':
		videoName = optarg;
		break;
	    case 's':
		errno = 0;
		beatsens = strtof(optarg, &endptr);
		if (errno) {
		    std::cerr << "-s: cannot convert " << optarg << " to a number" << std::endl;
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'b':
		before = strtol(optarg, &endptr, 10);
		if (endptr == optarg) {
		    std::cerr << "-b: cannot convert " << optarg << " to a number" << std::endl;
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'a':
		after = strtol(optarg, &endptr, 10);
		if (endptr == optarg) {
		    std::cerr << "-a: cannot convert " << optarg << " to a number" << std::endl;
		    exit(EXIT_FAILURE);
		}
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


    SDL_Init(SDL_INIT_VIDEO);

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);


    
    SDL_Window *win = SDL_CreateWindow("projectM", 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    
    SDL_GL_GetDrawableSize(win,&width,&height);
    

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

    current.format = SDL_PIXELFORMAT_BGRA32;

    int sdlrc = SDL_SetWindowDisplayMode(win, &current);
    if (sdlrc < 0) {
	std::cerr << "cannot set display mode: " << SDL_GetError() << std::endl;
    }

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
    settings.beatSensitivity = beatsens;
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
    app->init(win, &glCtx);

    if (fullscrn) {
	app->setFullScreen();
    }

    int wh, ww;

    SDL_GetWindowSize(win, &ww, &wh);

    std::cout << "window height: " << wh << " width: " << ww << std::endl;

    if (dbgogl) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DebugLog, NULL);
    }

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
    // standard main loop
    int fps = app->settings().fps;
    printf("fps: %d\n", fps);
    if (fps <= 0)
        fps = 60;
    const Uint32 frame_delay = 1000/fps;
    struct timespec frstart, frend, frsleep, fract;
    long long compns = 0;
    useconds_t prevdly;
    Uint32 last_time = SDL_GetTicks();
    app->audioChannelsCount = app->sndInfo.channels;
    snd_pcm_t *pcm_handle = NULL;
    snd_pcm_hw_params_t *params;
    unsigned int tmp;
    snd_pcm_uframes_t period, bufsz;
    unsigned int pcm;
    if (!pcmDevice.empty()) {
        pcm = snd_pcm_open(&pcm_handle, pcmDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
        if (pcm < 0) {
            std::cerr << "cannot open PCM " << snd_strerror(pcm) << std::endl;
	    exit(EXIT_FAILURE);
        }
    }

    if (pcm_handle != NULL) {

        /* Allocate parameters object and fill it with default values*/
        snd_pcm_hw_params_alloca(&params);

        snd_pcm_hw_params_any(pcm_handle, params);

        /* Set parameters */
        if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
             SND_PCM_ACCESS_RW_INTERLEAVED) < 0) 
             printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

        if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
             SND_PCM_FORMAT_S16_LE) < 0) 
             printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

        if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, app->sndInfo.channels) < 0) 
             printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

        if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, (unsigned int *)&app->sndInfo.samplerate, 0) < 0) 
             printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

        std::cout << "set sample rate " << app->sndInfo.samplerate << std::endl;

        printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

        printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

        snd_pcm_hw_params_get_channels(params, &tmp);
        printf("channels: %i\n", tmp);
 
        snd_pcm_hw_params_get_rate(params, &tmp, 0);
        printf("rate: %d bps\n", tmp);


     /* Write parameters */
        if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
            printf("ERROR: Can't set hardware parameters. %s\n", snd_strerror(pcm));


        snd_pcm_hw_params_get_period_size(params, &period, 0);
        std::cout << "period: " << period << std::endl;


        printf("pcm ready\n");
    }

    int asamples = app->sndInfo.samplerate / fps;
    std::cout << "Videoframe: " << frame_delay << " ms.; " << asamples << " audio samples/video frame" << std::endl;

    prevdly = asamples;

    int npresets = app->getPlaylistSize();

    std::cout << "N presets: " << npresets << std::endl;

    int sel = -1;
    for (int i = 0 ; i < npresets ; i++) {
        if (presetName == app->getPresetName(i)) {
	   app->selectPreset(i);
	   app->setPresetLock(1);
	   sel = i;
	   break;
        }
    }

    if (sel == -1) {
        std::cerr << "Could not find preset " << presetName << std::endl;
    }

    int glbufsz = sizeof( GLubyte ) * ww * wh * 4;
    int ffmpipe[2] = {-1, -1};
    GLuint pbo[2];
    unsigned int frameno = 0;

    if (!videoName.empty()) {
        int rc = pipe(ffmpipe);
	if (rc == -1) {
	    std::cerr << "ffmpeg video pipe creation error " << strerror(errno) << std::endl;
	    exit(EXIT_FAILURE);
	}
	int ffmpid = fork();
	if (ffmpid < 0) {
	    std::cerr << "ffmpeg process creation error " << strerror(errno) << std::endl;
	    exit(EXIT_FAILURE);
	} else if (ffmpid == 0) {
	    char fmtbuf[50];
	    snprintf(fmtbuf, 49, "%dx%d", ww, wh);
	    char fpsbuf[10];
	    snprintf(fpsbuf, 9, "%d", fps);
	    char pipebuf[20];
	    snprintf(pipebuf, 19, "pipe:%d", ffmpipe[0]);
	    char itsoffset[10];
	    snprintf(itsoffset, 9, "%ld", before);
	    const char *ffmargs[] = { "-y", "-video_size", fmtbuf, "-framerate", fpsbuf,
		                "-f", "rawvideo", "-pix_fmt", "bgra", "-s", fmtbuf,
				"-i", pipebuf,
			        "-itsoffset", itsoffset, 
				"-i", audioFile.c_str(), 
				"-vcodec", "ffvhuff", "-pix_fmt", "yuv420p", 
				"-acodec", "aac", "-filter_complex", "[1:0] apad", "-shortest",
				"-async", "1", "-y", videoName.c_str(), 
				NULL};
	    rc = execvp("ffmpeg", (char **)ffmargs);
	    if (rc < 0) {
 	        std::cerr << "ffmpeg process exec error " << strerror(errno) << std::endl;
	        exit(EXIT_FAILURE);
	    }
	}
        glGenBuffers(2, pbo);

	for (int np = 0; np < 2; np++) {
	    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[np]);
            glBufferData(GL_PIXEL_PACK_BUFFER, glbufsz, 0, GL_DYNAMIC_READ);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);	
	}
	
    }

    auto oneframe = [&](SNDFILE *sndf, snd_pcm_t *pcm_hnd) {
	frameno++;
	clock_gettime(CLOCK_REALTIME, &frstart);
        app->renderFrame();
	if (ffmpipe[1] > -1) {
    	    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[(frameno - 1) & 1]);
            glReadPixels(0, 0, ww, wh, GL_BGRA, GL_UNSIGNED_BYTE, 0);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	    glFlush(); // its position here is very important
            GLubyte *ptr = (GLubyte *)glMapNamedBuffer(pbo[frameno & 1], GL_READ_ONLY);
	    int rc = write(ffmpipe[1], ptr, glbufsz);
	    glUnmapNamedBuffer(pbo[frameno & 1]);
	    if (rc < 0) {
	        std::cerr << "error writing to ffmpeg video pipe " << strerror(errno) << std::endl;
		close(ffmpipe[1]);
		ffmpipe[1] = -1;
		app->done = 1;
		return;
	    }
	    fsync(ffmpipe[1]);
	}
        unsigned char *samplebuf;
	sf_count_t nsamples;
	int smplsize, buflen;
        if (sndf != NULL) {
	    smplsize = app->sndInfo.channels * sizeof(short),
	    buflen = asamples * smplsize;
	    samplebuf = (unsigned char *)alloca(buflen);
            nsamples = sf_readf_short(app->sndFile, (short *)samplebuf, asamples);
	    if (nsamples <= 0) {
                sf_close(app->sndFile);
		app->done = 2;
                return;
	    } else {
	        app->pcm()->addPCM16Data((short *)samplebuf, nsamples);
	    }
	} else {
	    samplebuf = (unsigned char *)alloca(128);
	    app->pcm()->addPCM16Data((short *)samplebuf, 32);
	}
	unsigned char *aptr;
	sf_count_t left;
	if (pcm_hnd != NULL && sndf != NULL) {
	    snd_pcm_nonblock(pcm_hnd, 0);
	    for(aptr = samplebuf, left = nsamples; left > 0 ; left -= period, aptr += period * smplsize) {
	        sf_count_t wrtsize = (left > period)?period:left;
                if (pcm = snd_pcm_writei(pcm_hnd, aptr, wrtsize) == -EPIPE) {
 	            snd_pcm_prepare(pcm_hnd);
                } else if (pcm < 0) {
                    printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
  	        }
	    }
	}
        app->pollEvent();

        if (pcm_hnd) {
	    clock_gettime(CLOCK_REALTIME, &frend);
	    unsigned long long startns, endns, durns, planns = frame_delay * 1000000ull, actns;
	    startns = frstart.tv_sec * 1000000000ull + frstart.tv_nsec;
	    endns = frend.tv_sec * 1000000000ull + frend.tv_nsec;
	    durns = endns - startns;
	    if (durns < planns) {
                frsleep.tv_sec = 0;
		frsleep.tv_nsec = planns - durns - (compns > 0 ? compns : 0);
                clock_nanosleep(CLOCK_REALTIME, 0, &frsleep, NULL);
	    }
	    clock_gettime(CLOCK_REALTIME, &fract);
	    actns = fract.tv_sec * 1000000000ull + fract.tv_nsec;
	    compns = actns - startns - planns;
	}
	
    };


    for(int i = 0; !app->done && i < before * fps ; i++) {
	oneframe(NULL, NULL);
    }
    while (!app->done) {
        oneframe(app->sndFile, pcm_handle);
    }

    if (app->done == 2) {
	app->done = 0;
        if (pcm_handle) snd_pcm_close(pcm_handle);
	pcm_handle = NULL;
    }

    for(int i = 0; !app->done && i < after * fps ; i++) {
	oneframe(NULL, NULL);
    }

    close(ffmpipe[1]);

    SDL_GL_DeleteContext(glCtx);

    delete app;

    return PROJECTM_SUCCESS;
}


