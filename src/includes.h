/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This file has all the includes so the app works in different systems.
	It is a little bit low level so do not worry about the code.
*/

#ifndef INCLUDES_H
#define INCLUDES_H

//define the application window size
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define DEFAULT_WINDOW_FULLSCREEN false
#define DEFAULT_WINDOW_CAPTION "Miniengine app"
#define DEFAULT_ASSETS_ROOT_FOLDER "./"

//under windows we need this file to make opengl work
#ifdef WIN32 
	#include <windows.h>
#endif


//SDL

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#include <SDL2/SDL.h>

#define glActiveTexture glActiveTexture_SDL
#include <SDL2/SDL_opengl.h>
#undef glActiveTexture 

#include <GL/glu.h>

#ifdef USE_GLUT
	#ifdef WIN32
		#include <GL/glut.h>
	#endif

	#ifdef __APPLE__
		#include <GLUT/glut.h>
	#endif
#endif

#include <iostream>

//OpenGL

//used to access opengl extensions
#define REGISTER_GLEXT(RET, FUNCNAME, ...) typedef RET (APIENTRY * FUNCNAME ## _func)(__VA_ARGS__); FUNCNAME ## _func FUNCNAME = NULL; 
#define IMPORT_GLEXT(FUNCNAME) FUNCNAME = (FUNCNAME ## _func) SDL_GL_GetProcAddress(#FUNCNAME); if (FUNCNAME == NULL) { std::cout << "ERROR: This Graphics card doesnt support " << #FUNCNAME << std::endl; }
//used for opengl extensions already defined in SDL_opengl.h to avoid collisions with names
#define REGISTER_GLEXT_(RET, FUNCNAME, ...) typedef RET (APIENTRY * FUNCNAME ## _func)(__VA_ARGS__); FUNCNAME ## _func _ ## FUNCNAME = NULL; 
#define IMPORT_GLEXT_(FUNCNAME) _ ## FUNCNAME = (FUNCNAME ## _func) SDL_GL_GetProcAddress(#FUNCNAME); if ( _ ## FUNCNAME == NULL) { std::cout << "ERROR: This Graphics card doesnt support " << #FUNCNAME << std::endl; }

#endif