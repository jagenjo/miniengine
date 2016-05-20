#ifndef ENTITY_H
#define ENTITY_H

#include "../includes.h"
#include "../utils/math.h"

#include <string>
#include <list>
#include <set>
#include <map>

class Shader;
class Texture;
class Mesh;
class Camera;
class Entity;
class Controller;

#define KEEP_ALIVE -100


typedef std::list<Entity*> tEntityList;
typedef std::set<Entity*> tEntitySet;
typedef std::list<Entity*>::iterator tEntityListIt;
typedef std::set<Entity*>::iterator tEntitySetIt;

class Entity
{
public:
	//Composite
	Entity* parent;
	tEntityList children;
	float time_to_destroy; //(in secs) KEEP_ALIVE means keep alive

	//static std::map<std::string, Entity*> s_registered_entities;
	static tEntitySet s_entities_to_destroy;
	static tEntitySet s_entities_registered;
	static tEntityList s_entities_with_alpha; //they must be rendered at the end

	static bool s_enable_culling;
	static bool s_enable_debug_render;
	static bool s_rendering_alpha_entities;
	static unsigned int s_entities_rendered;

	//properties
	Matrix44 model;
	Matrix44 modelworld;
	std::string name;
	float radius;
	std::set<std::string> tags;

	//Control
	Controller* controller;

	//projection info
	Vector3 screen_pos;
	Vector2 screen_size;

	//animation
	Vector3 old_position;
	Vector3 velocity_vector;

	//material
	Vector3 entity_color;
	bool visible;
	bool render_children;
	float alpha; //alpha changes the rendering flow so it needs to be here

	//culling info
	float max_visible_distance;
	float distance_to_camera;
	float visibility; //tells a factor about how much space it takes of the screen
	bool inside_frustum;
	AABB oobb;
	AABB aabb;
	AABB aabb_children;

	//control flags
	bool is_entitymeshcollide;

public:
	//Ctor
	Entity();
	Entity(Entity* parent);
	virtual ~Entity();
	virtual void markToDestroy();

	void init();
	virtual const char* getClassName() { return "Entity"; }

	void addChild(Entity* child);
	Entity* getChild(unsigned int num, const char* filter = NULL);
	Entity* getChildByName(const char* name);
	unsigned int getNumChildren() { return children.size(); }
	std::vector<Entity*> getChildByTag(const char* tag);
	void setName(const char* v) {name = v;}

	void addTag(const char* tag) { tags.insert(tag); }
	void removeTag(const char* tag) { if (isTag(tag)) tags.erase(tags.find(tag)); }
	bool isTag(const char* tag) { return tags.find(tag) != tags.end(); }

	void registerEntity();
	void unregisterEntity();
	bool isRegistered();


	virtual bool processAction(const char* action, Vector3 params) { return false; }

	virtual void render();
	virtual void renderEntity() {}
	virtual void update(float seconds);

	virtual void renderDebug();
	virtual void renderBounding();

	//properties
	virtual void setEntityColor(Vector3 color);

	//transforms
	inline Vector3 getLocalPosition() { return model.getTranslation(); }
	inline Vector3 getWorldPosition() { return modelworld.getTranslation(); }
	virtual Matrix44 getModelWorld();
	Vector3 getWorldCoordinates( Vector3 v );
	Vector3 getVelocityWorld();
	void setPositionAndYaw(Vector3 pos, float yaw_in_deg);

	//computations
	void updateBoundingInfo(); //extract info about boundings and wolrd matrix
	void updateCulling(Camera* camera); //compute if it is inside the camera
	void computeProjection(Camera* cam, bool recursive = false); //project to camera space

	//change the camera
	virtual void updateCamera(char cam_mode, Vector3 controller, Camera* camera, float elapsed_time, bool smooth = false);

	//dump for debug
	std::string virtual toString();



	static void destroyPendingEntities();
	static void renderAlphaEntities();
};

class EntityMesh : public Entity
{
protected:
	Mesh* mesh;
	Mesh* mesh_lowpoly;
	Mesh* mesh_lowpoly_flat;
	Texture* texture_lowpoly_flat;
	std::vector<Texture*> textures;

public:
	Shader* shader;
	bool additive_blend;
	bool alpha_test;
	bool two_sided;
	Vector3 emissive_color;
	float specular;
	float specular_gloss;

	float lod_factor;

	EntityMesh();
	EntityMesh(Entity* parent);
	void init();
	virtual const char* getClassName() { return "EntityMesh"; }

	//virtual void render();
	virtual void renderEntity();
	//virtual void update(float seconds);

	void update(float seconds);
	void renderMesh(int submaterial_id = 0, float lod = 1.0); //1.0 regular, 0.0 flat, other values lod_levels

	//Methods
	void setMesh( const char* mesh_filename );
	void setMesh( Mesh* mesh );

	Texture* getTexture(unsigned int id = 0) { return (textures.size() > id ? textures[id] : NULL); }
	void setTexture(Texture* texture, unsigned int i = 0) { if (textures.size() <= i+1) textures.resize(i+1); textures[i] = texture; }
	void setTexture(const char* filename, unsigned int i = 0);

	void setData(const char* mesh_filename, const char* texture_filename);
	void setData(const char* mesh_filename, std::vector<std::string> textures_filename);
	void setData(Mesh* mesh, Texture* texture);
	void setData(Mesh* mesh, std::vector<Texture*> textures);


	Mesh* getMesh() { return mesh; }
	virtual void uploadShaderParameters(unsigned int submaterial_id = 0);

	private:
	void updateOOBB();

};

//************** Colliding meshes ***********************

class EntityMeshCollide : public EntityMesh
{
public:
	enum { NULL_COLLISION, BOUNDING_COLLISION, SPHERE_COLLISION, PLANE_COLLISION, MESH_COLLISION };

	static std::list<EntityMeshCollide*> s_collision_entities_list;
	char collision_mode;

	bool entity_collision;
	bool bullet_collision;
	bool ground_collision;
	bool yield; //means it doesnt move so do not test collision with other yield objects

	EntityMeshCollide();
	EntityMeshCollide(Entity* parent);
	~EntityMeshCollide();

	virtual bool testCollision(EntityMeshCollide* object);
	virtual bool onEntityCollision(EntityMeshCollide* object) { return false; }
	virtual bool onSpecialCollision(char type, float force, Vector3 collision_point, Vector3 params) { return false; }

	//static void TestAllCollisions();

	static void TestCollisions(Entity* group_a, Entity* group_b);
};

//********* SOME HELPFUL MACROS ***************************

#define GENERIC_ENTITY(T, VARS) \
class T : public EntityMesh \
{ \
public: \
	VARS; \
	T(Entity* parent); \
\
	virtual const char* getClassName() { return #T; } \
};

#define GENERIC_RENDER_ENTITY(T, VARS) \
class T : public EntityMesh \
{ \
public: \
	VARS; \
	T(Entity* parent); \
	void update(float elapsed); \
	void renderEntity(); \
	virtual const char* getClassName() { return #T; } \
};

#endif