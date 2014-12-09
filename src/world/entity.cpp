#include "entity.h"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <set>
#include <list>
#include <string>

#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../gfx/camera.h"
#include "../gfx/shader.h"

#include "../utils/utils.h"
#include "world.h" //used for the global camera
#include "controller.h"

//std::map<std::string, Entity*> Entity::s_registered_entities;
tEntitySet Entity::s_entities_to_destroy;
tEntityList Entity::s_entities_with_alpha;
tEntitySet Entity::s_entities_registered;

bool Entity::s_enable_culling = true;
bool Entity::s_enable_debug_render = false;
bool Entity::s_rendering_alpha_entities = false;
unsigned int Entity::s_entities_rendered = 0;

Entity::Entity()
{
	registerEntity();
	Entity::init();
}

Entity::Entity(Entity* parent)
{
	registerEntity();
	Entity::init();
	if(parent)
		parent->addChild(this);
}

Entity::~Entity()
{
	unregisterEntity();

	#ifdef _DEBUG
		std::cout << "Entity deleted: " << name << std::endl;
	#endif

	//erase children
	for(tEntityListIt i = children.begin(); i != children.end(); i++ )
	{
		(*i)->parent = NULL;
		(*i)->markToDestroy();
	}

	if (parent == NULL)
		return;

	//erase from parent
	tEntityListIt it = std::find( parent->children.begin(), parent->children.end(), this );
	//tEntityList::iterator it = parent->children.find( *this );
	assert( it != parent->children.end() );
	parent->children.erase( it );

	if (controller && controller->canDelete())
		delete controller;
}

void Entity::markToDestroy()
{
	#ifdef _DEBUG
		std::cout << "Entity marked to destroy: " << name << std::endl;
	#endif
	s_entities_to_destroy.insert(this);
}

void Entity::destroyPendingEntities()
{
	while( !s_entities_to_destroy.empty() )
	{
		tEntitySetIt it = s_entities_to_destroy.begin();
		delete *it;
		s_entities_to_destroy.erase(it);
	}
}

void Entity::registerEntity()
{
	assert( s_entities_registered.find(this) == s_entities_registered.end() ); //already in
	s_entities_registered.insert( this );
}

void Entity::unregisterEntity()
{
	tEntitySetIt it = s_entities_registered.find(this);
	assert( it != s_entities_registered.end() );
	s_entities_registered.erase(it);
}

bool Entity::isRegistered()
{
	return s_entities_registered.find(this) != s_entities_registered.end();
}

void Entity::init()
{
	parent = NULL;
	model.setIdentity();
	//angular_speed = 0;
	time_to_destroy = KEEP_ALIVE;
	entity_color = Vector3(1,1,1);
	radius = 0.0;
	render_children = true;
	visible = true;
	max_visible_distance = 0;
	alpha = 1.0f;
	inside_frustum = true;
	controller = NULL;
	is_entitymeshcollide = false;
	distance_to_camera = 0;
}

void Entity::addChild(Entity* child)
{
	assert(child && "Child cannot be NULL");

	if (child->parent)
	{
		//erase from parent
		tEntityList::iterator it = std::find( child->parent->children.begin(), child->parent->children.end(), this );
		child->parent->children.erase( it );
	}

	child->parent = this;
	children.push_back(child);
}

Entity* Entity::getChild(unsigned int num, const char* filter)
{
	assert(num < children.size());
	tEntityList::iterator it = children.begin();
	int i = 0;
	while(it != children.end())
	{
		if (i == num && (filter == NULL || (*it)->isTag(filter)))
			return *it;
		if (filter == NULL || (*it)->isTag(filter) )
			i++;
		it++;
	}
	return NULL;
}

std::vector<Entity*> Entity::getChildByTag(const char* tag)
{
	std::vector<Entity*> result;
	for(tEntityList::iterator it = children.begin(); it != children.end(); it++)
		if ((*it)->isTag(tag))
			result.push_back(*it);
	return result;
}


Entity* Entity::getChildByName(const char* name)
{
	for(tEntityList::iterator it = children.begin(); it != children.end(); it++)
		if ((*it)->name == name)
			return *it;
	return NULL;
}

void Entity::render()
{
	//bool in_frustrum = true;
	Vector3 pos = modelworld.getTranslation();
	if (max_visible_distance && World::instance->current_camera->eye.distance(pos) > max_visible_distance)
		return;

	/*
	if(enable_culling && radius)
	{
		if (World::instance->current_camera->clipper.SphereInFrustum(pos.x, pos.y, pos.z, radius) == false)
			in_frustrum = false;
	}
	*/

	//glPushMatrix();
	//model.set();
	if ( (!s_enable_culling || (s_enable_culling && inside_frustum)) && visible)
	{
		if (this->alpha < 1.0 && !s_rendering_alpha_entities)
			s_entities_with_alpha.push_back(this);
		else
		{
			glLoadMatrixf( (modelworld * World::instance->current_camera->view_matrix).m );
			glPushAttrib( GL_ALL_ATTRIB_BITS );
			renderEntity();
			if (s_enable_debug_render)
			{
				renderBounding();
				renderDebug();
			}
			glPopAttrib();
		}
	}
	//glPopMatrix();

	//children propagation
	if (render_children)
		for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
			(*it)->render();
}

void Entity::renderBounding()
{
	glColor3f(1,0.2,0.2);
	glPushMatrix();
		glTranslatef(oobb.center.x, oobb.center.y, oobb.center.z);
		glScalef(oobb.halfsize.x, oobb.halfsize.y, oobb.halfsize.z);
#ifdef USE_GLUT
		glutWireCube(1.0);
#endif
	glPopMatrix();

	Texture::unbind();
	glColor3f(1,1,1);
	//glutWireSphere(radius,20,20);
	glPushMatrix();
		glLoadMatrixf(World::instance->current_camera->view_matrix.m);
		glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
		glScalef(aabb.halfsize.x, aabb.halfsize.y, aabb.halfsize.z);
#ifdef USE_GLUT
		glutWireCube(1.0);
#endif
	glPopMatrix();
}

//object space
void Entity::renderDebug()
{
	glDisable(GL_TEXTURE_2D);

	Vector3 pos = model * Vector3(0,0,0);
	Vector3 front = model.rotateVector(Vector3(0,0,-1));
	Vector3 right = model.rotateVector(Vector3(1,0,0));
	Vector3 up = model.rotateVector(Vector3(0,1,0));

	glBegin( GL_LINES );
		glColor3f(0,0,1);
		glVertex3fv(pos.v);
		front = pos + front * 20;
		glVertex3fv(front.v);

		glColor3f(1,0,0);
		glVertex3fv(pos.v);
		right = pos + right * 20;
		glVertex3fv(right.v);

		glColor3f(0,1,0);
		glVertex3fv(pos.v);
		up = pos + up * 20;
		glVertex3fv(up.v);

	glEnd();
}

void Entity::setEntityColor(Vector3 color)
{
	entity_color = color;

	//children propagation
	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
		(*it)->setEntityColor(color);
}

Matrix44 Entity::getModelWorld()
{
	modelworld = ( parent ? model * parent->getModelWorld() : model );
	return modelworld;
	//return ( parent ? parent->getModelWorld() * model : model );
}

Vector3 Entity::getWorldCoordinates( Vector3 v )
{
	return getModelWorld() * v;
}

Vector3 Entity::getVelocityWorld()
{
	return ( parent ? velocity_vector + parent->getVelocityWorld() : velocity_vector );
}

void Entity::setPositionAndYaw(Vector3 pos, float yaw_in_deg)
{
	model.setIdentity();
	model.rotateLocal(yaw_in_deg * DEG2RAD, Vector3(0.0f,1.0f,0.0f) );
	model.translate(pos.x, pos.y, pos.z);
}

void Entity::update(float seconds)
{
	//*
	model.translate( velocity_vector.x * seconds, velocity_vector.y * seconds, velocity_vector.z * seconds);
	//model.rotateLocal( angular_speed * seconds, Vector3(0,1,0) );
	//*/

	if (controller)
		controller->update(seconds);

	//children propagation
	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
	{
		if ((*it)->time_to_destroy != KEEP_ALIVE && (*it)->time_to_destroy < 0)
			(*it)->markToDestroy();
		else
			(*it)->update(seconds);

		if ((*it)->time_to_destroy != KEEP_ALIVE)
			(*it)->time_to_destroy -= seconds;
	}
}

std::string Entity::toString() 
{ 
	Vector3 pos = getWorldCoordinates(Vector3(0,0,0) );
	char temp[1024];
	sprintf (temp,"Name: %s:%s\nWPos: %f,%f,%f\n", name.c_str(), getClassName(), pos.x, pos.y,pos.z);
	return temp;
}

void Entity::updateBoundingInfo()
{
	//compute modelworld
	if (parent)
		modelworld = model * parent->modelworld;
	else
		modelworld = model;


	//compute world bounding
	const float max_float = 10000000;
	const float min_float = -10000000;
	Vector3 aabb_min(max_float,max_float,max_float);
	Vector3 aabb_max(min_float,min_float,min_float);

	aabb.center = modelworld.getTranslation();
	aabb.halfsize = Vector3(1,1,1) * radius;
	//*
	if (oobb.halfsize.length2() != 0.0)
	{
		Vector3 corners[8];
		corners[0] = oobb.center + oobb.halfsize * Vector3(-1,-1,-1);
		corners[1] = oobb.center + oobb.halfsize * Vector3(-1,-1,1);
		corners[2] = oobb.center + oobb.halfsize * Vector3(-1,1,-1);
		corners[3] = oobb.center + oobb.halfsize * Vector3(-1,1,1);
		corners[4] = oobb.center + oobb.halfsize * Vector3(1,-1,-1);
		corners[5] = oobb.center + oobb.halfsize * Vector3(1,-1,1);
		corners[6] = oobb.center + oobb.halfsize * Vector3(1,1,-1);
		corners[7] = oobb.center + oobb.halfsize * Vector3(1,1,1);
		Vector3 v;
		for (int i = 0; i < 8; i++)
		{
			v = modelworld * corners[i];
			aabb_min.setMin( v );
			aabb_max.setMax( v );
		}
		aabb.center = (aabb_max + aabb_min) * 0.5;
		aabb.halfsize = (aabb_max - aabb.center);
	}
	//*/

	//propagate
	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
	{
		Entity* e = (*it);
		e->updateBoundingInfo();
		aabb_min.setMin( e->aabb.center + e->aabb.halfsize );
		aabb_max.setMax( e->aabb.center + e->aabb.halfsize );
	}

	if (children.size() > 0)
	{
		aabb_children.center = (aabb_max + aabb_min) * 0.5;
		aabb_children.halfsize = (aabb_max - aabb.center) * 2;
	}
	else
		aabb_children = aabb;
}

void Entity::updateCulling(Camera* camera)
{
	if (radius != 0.0)
	{
		distance_to_camera = camera->eye.distance( modelworld.getTranslation() );
		if (distance_to_camera)
			visibility = (10 * radius) / (camera->tan_fov * distance_to_camera); //average normalized screen space
		else
			visibility = 1.0;

			inside_frustum = camera->clipper.SphereInFrustum( modelworld.getTranslation().x, modelworld.getTranslation().y, modelworld.getTranslation().z, aabb.halfsize.length() ) != Clipper::OUTSIDE;
	}

	//		inside_frustum = World::instance->current_camera->clipper.SphereInFrustum( aabb.center.x, aabb.center.y, aabb.center.z, aabb.halfsize.length() ) != Clipper::OUTSIDE;

		//inside_frustum = World::instance->current_camera->clipper.AABBInFrustrum( aabb.center - aabb.halfsize, aabb.center + aabb.halfsize ) != Clipper::OUTSIDE;

	for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
		(*it)->updateCulling(camera);
}

void Entity::computeProjection(Camera* camera, bool recursive)
{
	screen_pos = camera->project2D( this->getWorldCoordinates(Vector3(0,0,0) ), Camera::window_width, Camera::window_height );
	if (radius)
	{
		distance_to_camera = camera->eye.distance( modelworld.getTranslation() );
		if (distance_to_camera)
			visibility = (10 * radius) / (camera->tan_fov * distance_to_camera); //average normalized screen space
		else
			visibility = 1.0;
	}
	screen_size = Vector2(visibility,visibility);

	if (recursive)
		for (tEntityList::iterator it = children.begin(); it != children.end(); it++)
			(*it)->computeProjection(camera, true);
}

void Entity::updateCamera(char cam_mode, Vector3 controller, Camera* camera, float elapsed_time, bool smooth)
{
	float factor = 1.0;
	Vector3 neweye;
	Vector3 newcenter;
	Vector3 newup;

	if (controller.z != 0) camera->fov *= 0.5;

	if (cam_mode == CAM_GLOBAL)
	{
		Vector3 v(0,factor*radius*0.6,factor*radius*0.8);
		Matrix44 R;
		R.setRotation(controller.x * M_PI, Vector3(0,1,0));
		v = R * v;
		R.setRotation(controller.y * M_PI * 0.5, Vector3(1,0,0));
		v = R * v;
		neweye = modelworld * v;
		newcenter = modelworld * Vector3(0,factor*radius*0.5,0);
		newup = modelworld.topVector();
	}
	else if (cam_mode == CAM_FRONT)
	{
		Vector3 v(0,0,-factor*radius);
		neweye = modelworld * v ;
		newcenter = modelworld * (v*2);
		newup = modelworld.topVector();
	}
	else if (cam_mode == CAM_BACK)
	{
		neweye = modelworld * Vector3(0,radius * 0.5,-factor*radius);
		newcenter = modelworld * Vector3(0,radius*0.5,factor*radius);
		newup = modelworld.topVector();
	}

	float t = 0.2;
	if (smooth)
	{
		Vector3 v = (neweye - camera->eye);
		float d = v.length();

		camera->eye = camera->eye + v * elapsed_time * 10.0;
		camera->center = newcenter;
		camera->up = newup;
	}
	else
	{
		camera->eye = neweye;
		camera->center = newcenter;
		camera->up = newup;
	}
}

bool furtherFromCamera(const Entity* a, const Entity* b)
{
	if (a->distance_to_camera > b->distance_to_camera)
		return true;
	return false;
}

void Entity::renderAlphaEntities()
{
	s_entities_with_alpha.sort(furtherFromCamera);

	glDepthMask(false); //moved to renderAllAlpha
	glEnable( GL_BLEND );

	s_rendering_alpha_entities = true;
	std::list<Entity*>::iterator it = s_entities_with_alpha.begin();
	while(it != s_entities_with_alpha.end())
	{
		Entity* entity = (*it);
		glLoadMatrixf( (entity->modelworld * World::instance->current_camera->view_matrix).m );
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		entity->renderEntity();
		if (s_enable_debug_render)
			entity->renderBounding();
		glPopAttrib();
		it++;
	}
	s_entities_with_alpha.clear();
	s_rendering_alpha_entities = false;

	glDisable( GL_BLEND );
}

//********************************************

EntityMesh::EntityMesh()
{
	EntityMesh::init();
}

EntityMesh::EntityMesh(Entity* parent) : Entity(parent)
{
	EntityMesh::init();
}

void EntityMesh::init()
{
	this->mesh = NULL;
	this->shader = NULL;
	two_sided = false;
	alpha_test = false;
	additive_blend = false;
	emissive_color.set(0,0,0);
	specular = 0.5;
	specular_gloss = 2.0;
	mesh_lowpoly = NULL;
	mesh_lowpoly_flat = NULL;
	texture_lowpoly_flat = NULL;
	lod_factor = 1.0;
}

void EntityMesh::update(float seconds)
{
	Entity::update(seconds);
}

void EntityMesh::updateOOBB()
{
	radius = mesh->radius;
	oobb.center = this->mesh->center;
	oobb.halfsize = this->mesh->halfsize;
}

void EntityMesh::setMesh( const char* mesh_filename )
{
	Mesh* mesh = Mesh::Load( getResourceFilename(mesh_filename).c_str() );
	assert(mesh);
	this->setMesh(mesh);
}

void EntityMesh::setMesh( Mesh* mesh )
{
	this->mesh = mesh;
}

void EntityMesh::setTexture(const char* filename, unsigned int i)
{
	Texture* texture = Texture::Load( getResourceFilename(filename).c_str() );
	assert( texture );
	if (texture) 
		textures.push_back(texture);
}

void EntityMesh::setData(Mesh* mesh, Texture* texture)
{
	this->mesh = mesh;
	this->textures.clear();
	this->textures.push_back(texture);
	updateOOBB();
}

void EntityMesh::setData(Mesh* mesh, std::vector<Texture*> textures)
{
	this->setMesh(mesh);
	this->textures = textures;
}

void EntityMesh::setData(const char* mesh_filename, const char* texture_filename)
{
	Mesh* mesh = Mesh::Load( getResourceFilename(mesh_filename).c_str(), texture_filename == NULL);
	assert(mesh);
	setMesh( mesh );

	textures.clear();

	if (texture_filename != NULL)
	{
		Texture* texture = Texture::Load( getResourceFilename(texture_filename).c_str());
		assert(texture);
		if (texture) textures.push_back(texture);
	}
	else
	{
		for (size_t i = 0; i < mesh->material_name.size(); i++)
		{
			Texture* texture = Texture::Load( getResourceFilename(mesh->material_name[i].c_str()).c_str());
			assert(texture);
			if (texture) textures.push_back(texture);
		}
	}
}

void EntityMesh::setData(const char* mesh_filename, std::vector<std::string> textures_filename)
{
	Mesh* mesh = Mesh::Load( getResourceFilename(mesh_filename).c_str());
	assert(mesh);
	setMesh(mesh);

	for (unsigned int i = 0; i < textures_filename.size();i++)
	{
		Texture* texture = Texture::Load( getResourceFilename(textures_filename[i].c_str() ).c_str() );
		assert(texture);
		if (texture) textures.push_back(texture);
	}


}

void EntityMesh::renderEntity()
{
	s_entities_rendered++;
	//material settings
	if (alpha_test)
	{
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( GL_GEQUAL, 0.5 );
	}

	if (alpha < 1.0)
	{
		//glDepthMask(false); //moved to renderAlphaEntities to save some calls
		//glEnable( GL_BLEND );
		if (additive_blend)
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		else
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	//else
	//	glDisable( GL_BLEND );

	if (two_sided) glDisable( GL_CULL_FACE );

	//LOD computation and rendering
	float lod_level = 1.0;
	if ( visibility < (lod_factor * 0.05) )
		lod_level = 0.0;
	else if ( visibility < (lod_factor * 0.2) )
		lod_level = 0.5;
	else
		lod_level = 1.0;

	//Render
	render_children = true;
	if (mesh_lowpoly && lod_level < 1.0)
	{
		renderMesh(0,lod_level);
		render_children = false;
	}
	else if (mesh)
	{
		for (unsigned int i = 0; i < mesh->getNumSubmeshes(); i++)
			renderMesh(i);
	}

	if (alpha_test) glDisable( GL_ALPHA_TEST );
	if (two_sided) glEnable( GL_CULL_FACE );
}

void EntityMesh::renderMesh(int submaterial_id, float lod )
{
	glColor4f( entity_color.x, entity_color.y, entity_color.z, alpha );

	//flatmode
	if ( mesh_lowpoly_flat && lod == 0.0)
	{
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( GL_GEQUAL, 0.5 );
		if (texture_lowpoly_flat)
			texture_lowpoly_flat->bind();
		mesh_lowpoly_flat->render();
		glDisable( GL_ALPHA_TEST );
		return;	
	}

	if ( getTexture(submaterial_id) ) 
		getTexture(submaterial_id)->bind();
	else
		glDisable( GL_TEXTURE_2D) ;

	if (shader)
	{
		shader->enable();
		uploadShaderParameters(submaterial_id);
	}

	if (lod < 1.0)
		mesh_lowpoly->render();
	else
		mesh->render(submaterial_id);
	
	if (shader)	shader->disable();

	glDisable(GL_TEXTURE_2D);
	//glutWireSphere( mesh->radius, 10,10 );
}

void EntityMesh::uploadShaderParameters(unsigned int submaterial_id)
{
	if (shader == NULL)
		return;

	Matrix44 mod = modelworld;
	if (shader->IsVar("model"))
		shader->setMatrix44("model", mod.m );

	mod.removeTranslation();
	//mod.inverse();
	if (shader->IsVar("modelt"))
		shader->setMatrix44("modelt", mod.getRotationMatrix().m );

	if (shader->IsVar("camera_pos"))
		shader->setUniform3Array("camera_pos", World::instance->current_camera->eye.v, 1 );

	if (shader->IsVar("light_color"))
		shader->setUniform3Array("light_color", World::instance->sun_color.v, 1 );
	if (shader->IsVar("light_dir"))
		shader->setUniform3Array("light_dir", World::instance->sun_direction.v, 1 );

	if (shader->IsVar("fog_color"))
		shader->setUniform4Array("fog_color", World::instance->fog_color.v, 1 );
	if (shader->IsVar("fog_density"))
		shader->setUniform1("fog_density", World::instance->fog_density  );

	if (getTexture(submaterial_id))
		shader->setTexture("texture", getTexture(submaterial_id)->texture_id );

	if (shader->IsVar("time"))
		shader->setUniform1("time",World::instance->global_time);

	if (shader->IsVar("emissive_color"))
		shader->setUniform3Array("emissive_color",emissive_color.v,1);

	if (shader->IsVar("specular"))
		shader->setUniform1("specular",specular);
	if (shader->IsVar("specular_gloss"))
		shader->setUniform1("specular_gloss",specular_gloss);
	if (shader->IsVar("ground_color"))
		shader->setUniform3Array("ground_color",World::instance->ground_color.v,1);
	if (shader->IsVar("ambient_color"))
		shader->setUniform3Array("ambient_color",World::instance->ambient_color.v,1);

	if (shader->IsVar("discard_up"))
	{
		if (World::instance->rendering_reflection)
		{
			//float f = World::instance->current_camera->eye.y < 0.0 ? 1.0f : -1.0f;
			shader->setUniform1("discard_up", 1.0f);
		}
		else
		{
			shader->setUniform1("discard_up", 0.0f);
		}
	}
}

// *********************************
std::list<EntityMeshCollide*> EntityMeshCollide::s_collision_entities_list;

EntityMeshCollide::EntityMeshCollide() : EntityMesh()
{
	collision_mode = NULL_COLLISION;
	is_entitymeshcollide = true;
	entity_collision = false;
	bullet_collision = false;
	ground_collision = false;
	yield = false;

	s_collision_entities_list.push_back(this);
}

EntityMeshCollide::EntityMeshCollide(Entity* parent) : EntityMesh(parent)
{
	collision_mode = NULL_COLLISION;
	entity_collision = false;
	bullet_collision = false;
	ground_collision = false;
	is_entitymeshcollide = true;
	yield = false;

	s_collision_entities_list.push_back(this);
}

EntityMeshCollide::~EntityMeshCollide()
{
	//remove
	for (std::list<EntityMeshCollide*>::iterator it = s_collision_entities_list.begin(); it != s_collision_entities_list.end(); it++)
		if ( *it == this )
		{
			s_collision_entities_list.erase(it);
			return;
		}
}

bool EntityMeshCollide::testCollision(EntityMeshCollide* object)
{
	assert(object != this);

	if (!entity_collision || !object->entity_collision) return false;
	if (yield && object->yield) return false; //static objects cant collide between them

	//sphere
	if (collision_mode == NULL_COLLISION || object->collision_mode == NULL_COLLISION )
		return false;


	if (collision_mode == SPHERE_COLLISION && object->collision_mode == SPHERE_COLLISION && modelworld.getTranslation().distance( object->modelworld.getTranslation()) < (radius + object->radius) )
	{
		onEntityCollision(object);
		object->onEntityCollision(this);
		return true;
	}
	
	if (collision_mode == SPHERE_COLLISION && object->collision_mode == PLANE_COLLISION && fabs(modelworld.getTranslation().y - object->modelworld.getTranslation().y) < radius)
	{
		onEntityCollision(object);
		object->onEntityCollision(this);
		return true;
	}

	if (collision_mode == PLANE_COLLISION && object->collision_mode == SPHERE_COLLISION && fabs(modelworld.getTranslation().y - object->modelworld.getTranslation().y) < object->radius)
	{
		onEntityCollision(object);
		object->onEntityCollision(this);
		return true;
	}

	if (collision_mode == MESH_COLLISION && object->collision_mode == SPHERE_COLLISION)
	{
		mesh->collision_model->setTransform( modelworld.m );
		bool test = mesh->collision_model->sphereCollision( object->modelworld.getTranslation().v, object->radius );
		if (test == true)
		{
			onEntityCollision(object);
			object->onEntityCollision(this);
			return true;
		}
	}

	if (collision_mode == SPHERE_COLLISION && object->collision_mode == MESH_COLLISION)
	{
		object->mesh->collision_model->setTransform( object->modelworld.m );
		bool test = object->mesh->collision_model->sphereCollision( modelworld.getTranslation().v, radius );
		if (test == true)
		{
			onEntityCollision(object);
			object->onEntityCollision(this);
			return true;
		}
	}

	if (collision_mode == MESH_COLLISION && object->collision_mode == MESH_COLLISION)
	{
		mesh->collision_model->setTransform( modelworld.m );
		bool test = object->mesh->collision_model->collision(mesh->collision_model,-1,0, object->modelworld.m );
		if (test == true)
		{
			onEntityCollision(object);
			object->onEntityCollision(this);
			return true;
		}
	}

	return false;
}

/*
void EntityMeshCollide::TestAllCollisions()
{
	for (std::list<EntityMeshCollide*>::iterator it = s_collision_entities_list.begin(); it != s_collision_entities_list.end(); it++)
		for (std::list<EntityMeshCollide*>::iterator it2 = it; it2 != s_collision_entities_list.end(); it2++)
		{
			if (it != it2)
				(*it)->testCollision( (*it2) );
		}
}
*/

void EntityMeshCollide::TestCollisions(Entity* group_a, Entity* group_b)
{
	if (group_a != group_b)
		for (std::list<Entity*>::iterator it = group_a->children.begin(); it != group_a->children.end(); it++)
		{
			if ((*it)->is_entitymeshcollide == false) continue;

			for (std::list<Entity*>::iterator it2 = group_b->children.begin(); it2 != group_b->children.end(); it2++)
			{
				if ((*it2)->is_entitymeshcollide == false) continue;
				if (*it != *it2)
					((EntityMeshCollide*)(*it))->testCollision( (EntityMeshCollide*)(*it2) );
			}
		}
	else
	{
		for (std::list<Entity*>::iterator it = group_a->children.begin(); it != group_a->children.end(); it++)
		{
			if ((*it)->is_entitymeshcollide == false) continue;

			for (std::list<Entity*>::iterator it2 = it; it2 != group_a->children.end(); it2++)
			{
				if ((*it2)->is_entitymeshcollide == false) continue;
				if (*it != *it2)
					((EntityMeshCollide*)(*it))->testCollision( (EntityMeshCollide*)(*it2) );
			}
		}
	}
}

//**********************************
