#ifndef INCLUDES_H
#define INCLUDES_H

//under windows we need this file to make opengl work
#ifdef WIN32 
	#include <windows.h>
#endif

//SDL
#ifdef WIN32
	#pragma comment(lib, "SDL.lib")
	#pragma comment(lib, "SDLmain.lib")
#endif
#include <SDL/SDL.h>
#ifndef NO_SHADERS
	#include <GL/glew.h>
#else
	#include <SDL/SDL_opengl.h>
#endif

//GLUT
#ifdef WIN32
	#include <GL/glut.h>
	#define ASSETS_ROOT_FOLDER "./"
#endif

#ifdef __APPLE__
	#include <GLUT/glut.h>
	#define ASSETS_ROOT_FOLDER "/tmp/"
#endif


//OpenGL
//#include <GL/glu.h> //including GLUT we include everything (opengl, glu and glut)



#endif