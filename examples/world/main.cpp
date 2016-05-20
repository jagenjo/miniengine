#include "../../src/miniengine.h"
#include <iostream>

class DemoApp : public Application
{
	Camera* camera;
	World* world;
	EntityMesh* teapot; 

	void init()
	{
		std::cout << "Launching app..." << std::endl;
		
		world = new World();

		Mesh* box = new Mesh();
		box->createSolidBox(100,100,100);

		int num = 10;
		for(int x = -num; x <= num; x++)
		for(int z = -num; z <= num; z++)
		{
			teapot = new EntityMesh();
			teapot->setMesh(box);
			teapot->setTexture("data/checkers.tga");
			teapot->model.translate(x*100.0f,0.0f,z*100.0f);
			world->addChild(teapot);
			if(x == 0 && z == 0)
				teapot->setName("center");
		}

		camera = new Camera();
		camera->setPerspective(45, this->window_width / (float)this->window_height,0.1f, 5000.0f);
		camera->lookAt(Vector3(1000.0f,1000.0f,1000.0f), Vector3(), Vector3(0.0f,1.0f,0.0f));
	}

	void render()
	{
		glClearColor(0,0,0,1);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		world->instance->renderWorld(camera);

		swapBuffers();
	}

	void update(double dt)
	{
		for(auto it = world->children.begin(); it != world->children.end(); it++)
		{
			Entity* teapot = *it;
			Vector3 pos = teapot->getWorldPosition();
			teapot->model.setTranslation( pos.x, (sin(time*0.003f + pos.x) + cos(time*0.003f + pos.z)) * 15.0f , pos.z );
		}

		//world->model.rotateLocal( mouse_delta.x * 0.01, Vector3(0,1,0) );
		camera->orbit(Vector3(), mouse_delta.x * 0.01f, Vector3(0.0f,1.0f,0.0f) );
	}

};

int main(int argc, char **argv)
{
	DemoApp app;
	app.createWindow("test",1024,768);
	app.start();

	return 0;
}