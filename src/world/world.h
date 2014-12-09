#ifndef WORLD_H
#define WORLD_H

#include "entity.h"

class Camera;

class World : public Entity
{
	public:

	static World* instance;

	bool free_camera;
	Camera* main_camera; //the camera we use to move around 
	Camera* current_camera; //the camera being using rendering in this frame
	Matrix44 frustum_mvp;

	EntityMesh* skybox;

	float elapsed_time;
	float global_time;
	Vector3 gravity;

	Vector3 background_color;
	Vector3 sun_color;
	Vector3 sun_direction;
	Vector3 ground_color;
	Vector3 ambient_color;
	Vector4 fog_color;
	float fog_density;

	bool rendering_reflection;
	bool render_wireframe;
	bool freeze_culling;

	World();

	virtual void renderWorld(Camera* camera);
	void renderEntity();
	void renderDebug();

	void update(float elapsed);

	void switchFreeCamera();
	Entity* searchEntityByTag(const char* tag, int num);
	std::vector<Entity*> getEntitiesByTag(const char* tag);
};

#endif //WORLD_H