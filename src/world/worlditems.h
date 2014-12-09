#ifndef WORLDITEMS_H
#define WORLDITEMS_H

#include "Entity.h"

GENERIC_ENTITY(SkyboxEntity, void uploadShaderParameters(unsigned int submaterial_id = 0); );

//***************************
class Quad
{
public:
	Quad() { id = 0; size = 1.0; }
	Quad(Vector3 pos) { this->pos = pos; id = 0; size = 1.0; }
	Quad(Vector3 pos, unsigned int id) { this->pos = pos; this->id = id; size = 1.0; }
	Quad(Vector3 pos, unsigned int id, float size) { this->pos = pos; this->id = id; this->size = size; }
	Vector3 pos;
	float size;
	unsigned int id;
};

GENERIC_RENDER_ENTITY(QuadCloud, 
					std::vector<Quad> quads; 
					float points_size;
					bool fixed_size;
					float blinking_freq;
					bool vertical_align;
					void configCloud(Vector3 color, float size, float freq, bool fixed_size);
					void QuadCloud::addLandingLights(float distance, float separation, int num_lights));


GENERIC_ENTITY(Planet, void uploadShaderParameters(unsigned int submaterial_id = 0););

GENERIC_RENDER_ENTITY(DebugEntity,);
GENERIC_ENTITY(WaterEntity, static int tex_reflection_id; float water_reflection; void uploadShaderParameters(unsigned int submaterial_id = 0); );



class Sun : public QuadCloud
{
public:
	Sun(Entity* parent);
	void postRender();
};

/*
class FXContainer : public Entity
{
public:
	class Item {
	public:
		char type;
		Vector4 color;
		Vector3 start;
		Vector3 end;
		float size;
	};

	void renderEntity();

	std::list<FXItem*> items;

	void drawLine();
};
*/


class MapProp : public EntityMeshCollide
{
public:
	float health;

	MapProp(Entity* parent);
	void explode();
	bool onEntityCollision(EntityMeshCollide* object);
	bool onSpecialCollision(char type, float force, Vector3 collision_point, Vector3 params);
};


#endif

