#include "app.h"

#include <cassert>
#include <iostream>

#include "utils/utils.h"
#include "utils/sound.h"

#include "world/entity.h"
#include "world/world.h"
#include "world/worlditems.h"

#include "gfx/camera.h"
#include "gfx/texture.h"
#include "gfx/shader.h"
#include "gfx/mesh.h"
#include "gfx/rendertotexture.h"

 //here we store the keyboard state
int mouse_last_x, mouse_last_y; //last mouse position
long last_time = 0; //this is used to calcule the elapsed time between frames
long last_update = 0;
bool fixed_update = true;
#define UPDATE_FPS 60
#define MAX_FRAMESKIP 10

App* App::instance = NULL;

App::App()
{
	RenderToTexture::init();
	AudioManager::init(1);

	instance = this;
	time_modifier = 1.0;
	frame = 0;
	fps = 0;
	main_loop = true;
	fixed_update = true;

	SDL_GetMouseState(&mouse_last_x,&mouse_last_y);
}

App::~App()
{
	AudioManager::deinit();
}

void App::render()
{
	//OpenGL flags
	glDisable( GL_CULL_FACE );
	glEnable( GL_DEPTH_TEST );

	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor4f(1.0,1.0,1.0,1.0);
	glutWireSphere(10,10,10);
}

void App::renderApp()
{
	Entity::s_entities_rendered = 0;
	Mesh::num_meshes_rendered = 0;
	Mesh::num_triangles_rendered = 0;
	
	render();

	//swap between front buffer and back buffer
	SDL_GL_SwapBuffers();

#ifdef _DEBUG
	if (frame % 30 == 0)
		std::cout << "Entities rendered: " << Entity::s_entities_rendered << "   Meshes: " << Mesh::num_meshes_rendered << "   Triangles: " << int(Mesh::num_triangles_rendered * 0.001 ) << "K" << std::endl;
#endif
	frame++;
}

//*****************************************************

//Keyboard event handler (sync input)
void App::onKeyEvent( SDL_KeyboardEvent event )
{
	if (event.type != SDL_KEYDOWN) return;

	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: main_loop = false; break;
		default: break;
	}
}

void App::lockMouse()
{
	mouse_locked = !mouse_locked;
	SDL_ShowCursor(!mouse_locked);
}

void App::onEvent( SDL_Event sdlEvent )
{
	switch(sdlEvent.type)
	{
		case SDL_QUIT: main_loop = false; break; //EVENT for when the user clicks the [x] in the corner
		case SDL_MOUSEBUTTONDOWN: //EXAMPLE OF sync mouse input
			break;
		case SDL_MOUSEBUTTONUP:
			//...
			break;
		case SDL_KEYUP: 
		case SDL_KEYDOWN: //EXAMPLE OF sync keyboard input
			onKeyEvent( sdlEvent.key );
			break;
		case SDL_JOYBUTTONUP:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			break;
			
	}
}

//create a window using SDL
int App::createWindow(const char* caption, int width, int height, bool fullscreen)
{
	int bpp = 0;
	int flags;

	if( SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(-1);
	}
	SDL_EnableUNICODE(1);
	atexit(SDL_Quit);

	flags = SDL_OPENGL | ( fullscreen ? SDL_FULLSCREEN : 0);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); //or 24
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	//antialiasing (disable this lines if it goes too slow)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4 ); //increase to have smoother polygons

	const SDL_VideoInfo* info = SDL_GetVideoInfo();

	if(!info)
	{
		fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
		return 0;
	}

	bpp = info->vfmt->BitsPerPixel;
	if(SDL_SetVideoMode(width, height, bpp, flags) == 0)
	{
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
		return 0;
	}

	SDL_PumpEvents(); //without this line asserts could fail on windows
	SDL_WM_SetCaption(caption,NULL);
	return bpp;
}

void App::start()
{
	init();
	mainLoop();
	return;
}

void App::mainLoop()
{
	SDL_Event sdlEvent;
	last_time = SDL_GetTicks();
	last_update = last_time;

	while (main_loop)
	{
		//render frame
		renderApp();

		//update events
		while(SDL_PollEvent(&sdlEvent))
			onEvent(sdlEvent);

		//read keyboard state and stored in keystate
		keystate = SDL_GetKeyState(NULL);

		//update logic
		double elapsed_time = (SDL_GetTicks() - last_time) * 0.001; //0.001 converts from milliseconds to seconds
		last_time = SDL_GetTicks();
		if (elapsed_time)
			fps = 1.0/elapsed_time;

		int num_frameskip = 0;
		if (!fixed_update)
			update(elapsed_time); 
		else
			while ( (SDL_GetTicks() - last_update) > 1000.0/UPDATE_FPS && num_frameskip < MAX_FRAMESKIP)
			{
				update(1.0/UPDATE_FPS); 
				last_update += 1000.0/UPDATE_FPS;
				num_frameskip++;
			}
	}

	return;
}