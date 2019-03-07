#include "world.h"

#include <cassert>
#include <iostream>

#include "../utils/utils.h"

#include "../gfx/texture.h"
#include "../gfx/mesh.h"
#include "../gfx/camera.h"
#include "../gfx/particles.h"
#include "../gfx/shader.h"

World* World::instance = NULL;

World::World()
{
	assert(instance == NULL);
	instance = this;
	global_time = 0;
	free_camera = false;

	current_camera = NULL;
	rendering_reflection = false;
	freeze_culling = false;

	render_wireframe = false;

	background_color.set(155.0f/256.0f, 177.0f/256.0f,184.0f/256.0f);
	gravity.set(0,-30,0);
	fog_density = 0.0001f;
	fog_color.set(0.5,0.5,0.5,1.0);

	skybox = NULL;
}


//prerender
void World::renderEntity()
{
	glDisable( GL_DEPTH_TEST );
	if (skybox)
	{
		glPushMatrix();
			//glScalef(0.1,0.1,0.1);
			skybox->modelworld.setTranslation( current_camera->eye.x, current_camera->eye.y, current_camera->eye.z );
			skybox->render();
		glPopMatrix();
		glDisable( GL_TEXTURE_2D );
	}	
	glEnable( GL_DEPTH_TEST );
}

void World::renderWorld(Camera* cam)
{
	assert(checkGLErrors());

	current_camera = cam;
	current_camera->set();

	if (!freeze_culling)
	{
		frustum_mvp = (current_camera->view_matrix * current_camera->projection_matrix);
		updateCulling(current_camera);
	}

	updateBoundingInfo();

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	
	glPolygonMode(GL_FRONT_AND_BACK, render_wireframe ? GL_LINE : GL_FILL);

	//render children
	glPushMatrix();
	Entity::render();
	glPopMatrix();

	//render alpha
	glPushMatrix();
	Entity::renderAlphaEntities();
	glPopMatrix();

	ParticleEmissor::RenderAll();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void World::renderDebug()
{
	Matrix44 f = frustum_mvp;
	f.inverse();
	glDisable( GL_BLEND );
	glDisable( GL_TEXTURE_2D );
	glPushMatrix();
		glMultMatrixf(f.m);
		glTranslatef(0,0,1);
		glColor3f(1,0,1);
#ifdef USE_GLUT
		glutWireCube(2.0);
#endif
		//drawGrid(0.1,20,false);
	glPopMatrix();
}

void World::update(float elapsed)
{
	if (elapsed == 0) return;

	elapsed_time = elapsed;
	global_time += elapsed;

	Entity::update(elapsed);

	if(skybox)
		skybox->update(elapsed);
	ParticleEmissor::UpdateAll(elapsed);
	//EntityMeshCollide::TestAllCollisions(); //collisions between entities
}

void World::switchFreeCamera()
{
	free_camera = !free_camera;
	if (free_camera && current_camera != main_camera)
		*main_camera = *current_camera;
	//main_camera->setPerspective(70, main_camera->aspect, main_camera->near_plane, main_camera->far_plane);
	main_camera->lookAt( main_camera->eye, main_camera->center, Vector3(0,1,0) );
}

std::vector<Entity*> World::getEntitiesByTag(const char* tag)
{
	std::vector<Entity*> entities;
	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
		if ( (*it)->isTag(tag) )
			entities.push_back(*it);
	return entities;
}

Entity* World::searchEntityByTag(const char* tag, int num)
{
	int i = 0;
	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
		if ( (*it)->isTag(tag) )
		{
			if (i == num)
				return *it;
			else
				i++;
		}
	return NULL;
}

