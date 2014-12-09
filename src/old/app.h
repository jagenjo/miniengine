#ifndef APP_H
#define APP_H

#include <vector>

#include "includes.h"
#include "utils/math.h"

class App
{
public:
	Uint8* keystate;
	unsigned int mousestate;
	//bool free_camera;

	bool mouse_locked;
	Vector2 delta_mouse;
	float time_modifier;
	unsigned int frame;
	unsigned int fps;
	bool main_loop;
	bool fixed_update;

	App();
	~App();

	virtual void init() {}
	virtual void render();
	virtual void update(float seconds_elapsed) {}

	virtual void onKeyEvent( SDL_KeyboardEvent event );
	virtual void onEvent( SDL_Event event );


	static App* instance;
	
	void renderApp();
	void lockMouse();
	
	void start();
	void mainLoop();
	
	int createWindow(const char* caption, int width, int height, bool fullscreen = false);

};

#endif