#include "src/miniengine.h"
#include <iostream>

class TestApp : public Application
{
	bool clicked;
	void init()
	{
		std::cout << "Launching app..." << std::endl;
		clicked = false;
	}

	void render()
	{
		if(clicked)
			glClearColor(1,0,0,1);
		else
			glClearColor(0,0,1,1);
		glClear( GL_COLOR_BUFFER_BIT );

		swapBuffers();
	}

	void onMouseButton(SDL_MouseButtonEvent event)
	{
		clicked = !clicked;
	}

};

int main(int argc, char **argv)
{
	TestApp app;
	app.createWindow("test",1024,768);
	app.start();

	return 0;
}