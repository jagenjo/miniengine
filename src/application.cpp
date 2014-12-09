#include "application.h"
#include "utils/utils.h"

Application::Application()
{
	this->window = NULL;
	time = 0;
	// initialize attributes
	// Warning: DO NOT CREATE STUFF HERE, USE THE INIT 
	// things create here cannot access opengl
	window_width = DEFAULT_WINDOW_WIDTH;
	window_height = DEFAULT_WINDOW_HEIGHT;

	keystate = NULL;
	mouse_locked = true;
}

//Here we have already GL working, so we can create meshes and textures
void Application::init(void)
{
}

//what to do when the image has to be draw
void Application::render(void)
{
	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	swapBuffers();
}

void Application::update(double seconds_elapsed)
{
}

void Application::swapBuffers()
{
	//swap between front buffer and back buffer
	SDL_GL_SwapWindow(this->window);
}

//Keyboard event handler (sync input)
void Application::onKey( SDL_KeyboardEvent event )
{
	if(event.type == SDL_KEYDOWN)
	{
		switch(event.keysym.sym)
		{
			case SDLK_ESCAPE: exit(0); //ESC key, kill the app
		}
	}
}


void Application::onMouseButton( SDL_MouseButtonEvent event )
{
}

void Application::setWindowSize(int width, int height)
{
	glViewport( 0,0, width, height );
	window_width = width;
	window_height = height;
}

//create a window using SDL
SDL_Window* Application::createWindow(const char* caption, int width, int height, bool fullscreen)
{
	int bpp = 0;

	SDL_Init(SDL_INIT_EVERYTHING);

	//set attributes
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); //or 24
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	//antialiasing (disable this lines if it goes too slow)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8 ); //increase to have smoother polygons

	//create the window
	window = SDL_CreateWindow(
		caption, 100, 100, width, height, 
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

	if(!window)
	{
		fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
		exit(-1);
	}
  
	// Create an OpenGL context associated with the window.
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	//in case of exit...
	atexit(SDL_Quit);

	//get events from the queue of unprocessed events
	SDL_PumpEvents(); //without this line asserts could fail on windows

	return window;
}

void Application::start()
{
	init();
	mainLoop();
}


//The application main loop
void Application::mainLoop()
{
	SDL_Event sdlEvent;
	int x,y;
	static long last_time = 0;

	SDL_GetMouseState(&x,&y);
	mouse_position.set(x,y);

	while (1)
	{
		//read keyboard state and stored in keystate
		keystate = SDL_GetKeyboardState(NULL);

		//render frame
		render();

		//update events
		while(SDL_PollEvent(&sdlEvent))
		{
			switch(sdlEvent.type)
				{
					case SDL_QUIT: return; break; //EVENT for when the user clicks the [x] in the corner
					case SDL_MOUSEBUTTONDOWN: //EXAMPLE OF sync mouse input
					case SDL_MOUSEBUTTONUP:
						onMouseButton( sdlEvent.button );
						break;
					case SDL_KEYDOWN: //EXAMPLE OF sync keyboard input
					case SDL_KEYUP: //EXAMPLE OF sync keyboard input
						onKey( sdlEvent.key );
						break;
					case SDL_WINDOWEVENT:
						switch (sdlEvent.window.event) {
							case SDL_WINDOWEVENT_RESIZED: //resize opengl context
								setWindowSize( sdlEvent.window.data1, sdlEvent.window.data2 );
								break;
						}
				}
		}

		//get mouse position and delta
		mouse_state = SDL_GetMouseState(&x,&y);
		mouse_delta.set( mouse_position.x - x, mouse_position.y - y );
		mouse_position.set(x,y);

		//update logic
		time = getTime();
		double elapsed_time = (time - last_time) * 0.001; //0.001 converts from milliseconds to seconds
		last_time = time;
		update(elapsed_time); 

		//check errors in opengl only when working in debug
		#ifdef _DEBUG
			checkGLErrors();
		#endif
	}

	return;
}
