/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This class encapsulates the game, is in charge of creating the game, getting the user input, process the update and render.
*/

#ifndef APPLICATION_H
#define APPLICATION_H

#include "includes.h"
#include "utils/math.h"

class Application
{
public:
	//window
	SDL_Window* window;
	float window_width;
	float window_height;

	//keyboard state
	const Uint8* keystate;

	//mouse state
	float time;
	int mouse_state; //tells which buttons are pressed
	Vector2 mouse_position; //last mouse position
	Vector2 mouse_delta; //mouse movement in the last frame
	bool mouse_locked; //tells if the mouse is locked (not seen)

	Application();
	SDL_Window* createWindow(const char* caption, int width, int height, bool fullscreen = false);
	void mainLoop();
	void start();

	virtual void init( void );
	virtual void render( void );
	virtual void update( double dt );

	virtual void onKey( SDL_KeyboardEvent event );
	virtual void onMouseButton( SDL_MouseButtonEvent event );

	void setWindowSize(int width, int height);
	void swapBuffers();

};


#endif 