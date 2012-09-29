/*
 * Copyright (c) 2011-2012 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <bcm_host.h>
#include <EGL/egl.h>
#include <SDL/SDL.h>

#include "albase/common.h"
#include "albase/gl/system.h"

EGLBoolean eglSaneChooseConfigBRCM(EGLDisplay, const EGLint *, EGLConfig, EGLint, EGLint *);

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static Vec2 size;

AlError al_gl_system_init()
{
	BEGIN();

	bcm_host_init();

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		al_log_error("Couldn't get EGL display");
		THROW(AL_ERROR_GRAPHICS);
	}

	if (!eglInitialize(display, NULL, NULL)) {
		al_log_error("Couldn't initialize EGL display");
		THROW(AL_ERROR_GRAPHICS);
	}

	EGLint attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLConfig config;
	int numConfigs;

	if (!eglSaneChooseConfigBRCM(display, attributes, &config, 1, &numConfigs)) {
		al_log_error("Couldn't get EGL config");
		THROW(AL_ERROR_GRAPHICS);
	}

	if (numConfigs != 1) {
		al_log_error("Couldn't get valid EGL config");
		THROW(AL_ERROR_GRAPHICS);
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		al_log_error("Couldn't bind OpenGL|ES API to EGL");
		THROW(AL_ERROR_GRAPHICS);
	}

	context = eglCreateContext(display, config, NULL, contextAttributes);
	if (context == EGL_NO_CONTEXT) {
		al_log_error("Couldn't create EGL context");
		THROW(AL_ERROR_GRAPHICS);
	}

	surface = eglCreateWindowSurface(display, config, 0, NULL);
	if (surface == EGL_NO_SURFACE) {
		al_log_error("Couldn't create EGL window surface");
		THROW(AL_ERROR_GRAPHICS);
	}

	if (!eglMakeCurrent(display, surface, surface, context)) {
		al_log_error("Couldn't make EGL context current");
		THROW(AL_ERROR_GRAPHICS);
	}

	EGLint width, height;
	eglQuerySurface(display, surface, EGL_WIDTH, &width);
	eglQuerySurface(display, surface, EGL_HEIGHT, &height);

	size = (Vec2){width, height};

	if (!SDL_SetVideoMode(width, height, 0, 0)) {
		al_log_error("Couldn't set up dummy SDL video");
		THROW(AL_ERROR_GRAPHICS);
	}

	PASS();
}

void al_gl_system_free()
{
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(display, surface);
	eglDestroyContext(display, context);
	eglTerminate(display);
}

Vec2 al_gl_system_screen_size()
{
	return size;
}

void al_gl_system_swap_buffers()
{
	eglSwapBuffers(display, surface);
}
