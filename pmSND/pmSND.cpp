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
* pmSND.cpp
* Authors: Created by Mischa Spiegelmock on 2017-09-18.
*
*
* experimental Stereoscopic SBS driver functionality by
*	RobertPancoast77@gmail.com
*/

#include "pmSND.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Renderer/ShaderEngine.hpp"
#include "Renderer/StaticGlShaders.h"

void projectMSND::maximize() {
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return;
    }

    SDL_SetWindowSize(win, dm.w, dm.h);
    resize(dm.w, dm.h);
}

/* Stretch projectM across multiple monitors */
void projectMSND::stretchMonitors()
{
	int displayCount = SDL_GetNumVideoDisplays();
	if (displayCount >= 2)
	{
		std::vector<SDL_Rect> displayBounds;
		for (int i = 0; i < displayCount; i++)
		{
			displayBounds.push_back(SDL_Rect());
			SDL_GetDisplayBounds(i, &displayBounds.back());
		}

		int mostXLeft = 0;
		int mostXRight = 0;
		int mostWide = 0;
		int mostYUp = 0;
		int mostYDown = 0;
		int mostHigh = 0;

		for (int i = 0; i < displayCount; i++)
		{
			if (displayBounds[i].x < mostXLeft) mostXLeft = displayBounds[i].x;
			if ((displayBounds[i].x + displayBounds[i].w) > mostXRight) mostXRight = displayBounds[i].x + displayBounds[i].w;
		}
		for (int i = 0; i < displayCount; i++)
		{
			if (displayBounds[i].y < mostYUp) mostYUp = displayBounds[i].y;
			if ((displayBounds[i].y + displayBounds[i].h) > mostYDown) mostYDown = displayBounds[i].y + displayBounds[i].h;
		}

        mostWide = abs(mostXLeft) + abs(mostXRight);
        mostHigh = abs(mostYUp) + abs(mostYDown);

		SDL_SetWindowPosition(win, mostXLeft, mostYUp);
		SDL_SetWindowSize(win, mostWide, mostHigh);
	}
}

/* Moves projectM to the next monitor */
void projectMSND::nextMonitor()
{
	int displayCount = SDL_GetNumVideoDisplays();
	int currentWindowIndex = SDL_GetWindowDisplayIndex(win);
	if (displayCount >= 2)
	{
		std::vector<SDL_Rect> displayBounds;
		int nextWindow = currentWindowIndex + 1;
		if (nextWindow >= displayCount) nextWindow = 0;
        
		for (int i = 0; i < displayCount; i++)
		{
			displayBounds.push_back(SDL_Rect());
			SDL_GetDisplayBounds(i, &displayBounds.back());
		}
		SDL_SetWindowPosition(win, displayBounds[nextWindow].x, displayBounds[nextWindow].y);
		SDL_SetWindowSize(win, displayBounds[nextWindow].w, displayBounds[nextWindow].h);
	}
}

void projectMSND::toggleFullScreen() {
    maximize();
    if (isFullScreen) {
        SDL_SetWindowFullscreen(win, 0);
        isFullScreen = false;
        SDL_ShowCursor(true);
    } else {
        SDL_ShowCursor(false);
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
        isFullScreen = true;
    }
}

void projectMSND::keyHandler(SDL_Event *sdl_evt) {
    projectMEvent evt;
    projectMKeycode key;
    projectMModifier mod;
    SDL_Keymod sdl_mod = (SDL_Keymod) sdl_evt->key.keysym.mod;
    SDL_Keycode sdl_keycode = sdl_evt->key.keysym.sym;

    // Left or Right Gui or Left Ctrl
    if (sdl_mod & KMOD_LGUI || sdl_mod & KMOD_RGUI || sdl_mod & KMOD_LCTRL)
        keymod = true;

	// handle keyboard input (for our app first, then projectM)
    switch (sdl_keycode) {

    case SDLK_q:
        if (sdl_mod & KMOD_LGUI || sdl_mod & KMOD_RGUI || sdl_mod & KMOD_LCTRL) {
            // cmd/ctrl-q = quit
            done = 1;
            return;
        }
        break;
    default:
	;
    }
    // translate into projectM codes and perform default projectM handler
    evt = sdl2pmEvent(sdl_evt);
    mod = sdl2pmModifier(sdl_mod);
    key = sdl2pmKeycode(sdl_keycode,sdl_mod);
    key_handler(evt, key, mod);
}

void projectMSND::resize(unsigned int width_, unsigned int height_) {
    width = width_;
    height = height_;
    projectM_resetGL(width, height);
}

void projectMSND::pollEvent() {
    SDL_Event evt;
    
    int mousex = 0;
    float mousexscale = 0;
    int mousey = 0;
    float mouseyscale = 0;
    int mousepressure = 0;
    while (SDL_PollEvent(&evt))
    {
        switch (evt.type) {
            case SDL_WINDOWEVENT:
            int h, w;
            SDL_GL_GetDrawableSize(win,&w,&h);
                switch (evt.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						resize(w, h);
						break;
					case SDL_WINDOWEVENT_SIZE_CHANGED:
                        resize(w, h);
						break;
                }
                break;
            case SDL_KEYDOWN:
                keyHandler(&evt);
                break;
	    case SDL_QUIT:
                done = 1;
                break;
        }
    }
}

// This touches the screen to generate a waveform at X / Y. 
void projectMSND::touch(float x, float y, int pressure, int touchtype) {
#ifdef PROJECTM_TOUCH_ENABLED
    projectM::touch(x, y, pressure, touchtype);
#endif
}

// This moves the X Y of your existing waveform that was generated by a touch (only if you held down your click and dragged your mouse around).
void projectMSND::touchDrag(float x, float y, int pressure) {
    projectM::touchDrag(x, y, pressure);
}

// Remove waveform at X Y
void projectMSND::touchDestroy(float x, float y) {
    projectM::touchDestroy(x, y);
}

// Remove all waveforms
void projectMSND::touchDestroyAll() {
    projectM::touchDestroyAll();
}

void projectMSND::renderFrame() {
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    projectM::renderFrame();

    if (renderToTexture) {
        renderTexture();
    }

    SDL_GL_SwapWindow(win);
}

projectMSND::projectMSND(Settings settings, int flags) : projectM(settings, flags) {
    width = getWindowWidth();
    height = getWindowHeight();
    done = 0;
    isFullScreen = false;
}

projectMSND::projectMSND(std::string config_file, int flags) : projectM(config_file, flags) {
    width = getWindowWidth();
    height = getWindowHeight();
    done = 0;
    isFullScreen = false;
}

void projectMSND::init(SDL_Window *window, SDL_GLContext *_glCtx, const bool _renderToTexture) {
    win = window;
    glCtx = _glCtx;
    projectM_resetGL(width, height);

#ifdef WASAPI_LOOPBACK
    wasapi = true;
#endif

    // are we rendering to a texture?
    renderToTexture = _renderToTexture;
    if (renderToTexture) {
        programID = ShaderEngine::CompileShaderProgram(
            StaticGlShaders::Get()->GetV2fC4fT2fVertexShader(),
            StaticGlShaders::Get()->GetV2fC4fT2fFragmentShader(), "v2f_c4f_t2f");
        textureID = projectM::initRenderToTexture();

        float points[16] = {
            -0.8, -0.8,
            0.0,    0.0,

            -0.8, 0.8,
            0.0,   1.0,

            0.8, -0.8,
            1.0,    0.0,

            0.8, 0.8,
            1.0,    1.0,
        };

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, points, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0); // Positions

        glDisableVertexAttribArray(1);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2)); // Textures

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}


std::string projectMSND::getActivePresetName()
{
    unsigned int index = 0;
    if (selectedPresetIndex(index)) {
        return getPresetName(index);
    }
    return std::string();
}


void projectMSND::renderTexture() {
    static int frame = 0;
    frame++;

    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, getWindowWidth(), getWindowHeight());

    glUseProgram(programID);

    glUniform1i(glGetUniformLocation(programID, "texture_sampler"), 0);

    glm::mat4 mat_proj = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 2.0f, 10.0f);

    glEnable(GL_DEPTH_TEST);

    glm::mat4 mat_model = glm::mat4(1.0f);
    mat_model = glm::translate(mat_model, glm::vec3(cos(frame*0.023),
                                                    cos(frame*0.017),
                                                    -5+sin(frame*0.022)*2));
    mat_model = glm::rotate(mat_model, glm::radians((float) sin(frame*0.0043)*360),
                            glm::vec3(sin(frame*0.0017),
                                      sin(frame *0.0032),
                                      1));

    glm::mat4 mat_transf = glm::mat4(1.0f);
    mat_transf = mat_proj * mat_model;

    glUniformMatrix4fv(glGetUniformLocation(programID, "vertex_transformation"), 1, GL_FALSE, glm::value_ptr(mat_transf));

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, textureID);

    glVertexAttrib4f(1, 1.0, 1.0, 1.0, 1.0);

    glBindVertexArray(m_vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);

    glDisable(GL_DEPTH_TEST);
}

void projectMSND::presetSwitchedEvent(bool isHardCut, size_t index) const {
    std::string presetName = getPresetName(index);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Displaying preset: %s\n", presetName.c_str());
    
    std::string newTitle = "projectM ➫ " + presetName;
    SDL_SetWindowTitle(win, newTitle.c_str());
}
