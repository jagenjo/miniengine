#include "worlditems.h"

#include <iostream>
#include <algorithm>

#include "../gfx/camera.h"
#include "../gfx/shader.h"
#include "../gfx/texture.h"
#include "../gfx/particles.h"

#include "world.h"
#include "../utils/utils.h"

//******************************
int WaterEntity::tex_reflection_id = -1;

WaterEntity::WaterEntity(Entity* parent) : EntityMesh(parent)
{
	//rendering_reflection = false;
	//tex_reflection_id = -1;
	alpha = 0.5;
	water_reflection = 1.0;
	two_sided = true;
	setData( "water_big.ASE", "textures/water_normalmap.tga" );
	//shader = Shader::Load( getResourceFilename( "shaders/water.vs" ).c_str(), getResourceFilename( "shaders/water.ps" ).c_str() );
	shader = Shader::Load( getResourceFilename( "shaders/reflection.vs" ).c_str(), getResourceFilename( "shaders/reflection.ps" ).c_str() );
}

void WaterEntity::uploadShaderParameters(unsigned int submaterial_id)
{
	EntityMesh::uploadShaderParameters();

	if (shader->IsVar("water_reflection"))
		shader->setFloat("water_reflection", water_reflection );

	if (shader->IsVar("reflection"))
	{
		if (tex_reflection_id != -1)
		{
			shader->setTexture("reflection",tex_reflection_id);
		}
		else
		{
			Texture* cielo = Texture::Load( getResourceFilename("textures/sky_polar.tga").c_str() );
			shader->setTexture("reflection", cielo->texture_id );
		}
	}
}


//***********************

SkyboxEntity::SkyboxEntity(Entity* parent)
{
	two_sided = true;
	//setData( "box.ASE", "cielo.tga" );
	//shader = Shader::Load( getResourceFilename( "shaders/bg.vs" ).c_str(), getResourceFilename( "shaders/bg.ps" ).c_str() );
	setData("cubemap.ASE","textures/cielo2.tga");
	getTexture()->bind();
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR); //set the mag filter
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	//set the min filter
}

void SkyboxEntity::uploadShaderParameters(unsigned int submaterial_id)
{
	EntityMesh::uploadShaderParameters();
}

// ************************

QuadCloud::QuadCloud(Entity* parent) : EntityMesh(parent)
{
	emissive_color.set(1.0,1.0,1.0);
	points_size = 25;
	fixed_size = true;
	blinking_freq = 0.0;
	alpha = 0.99f;
	vertical_align = false;
	setTexture( Texture::Load( getResourceFilename("textures/stars1.tga").c_str() ) );
}

void QuadCloud::configCloud(Vector3 color, float size, float freq, bool fixed_size)
{
	this->emissive_color = color;
	this->points_size = size;
	this->blinking_freq = freq;
	this->fixed_size = fixed_size;
}

void QuadCloud::addLandingLights(float distance, float separation, int num_lights)
{
	emissive_color.set(0.3f,0.5f,1.0f);
	quads.resize(num_lights*2);
	for (int i = 0; i < num_lights; i++)
	{
		quads[2*i].pos.set(separation,0,i*distance);
		quads[2*i+1].pos.set(-separation,0,i*distance);
	}
	fixed_size = false;
	blinking_freq = 1.0;
}

void QuadCloud::update(float elapsed)
{
}

bool distance_func (Quad a, Quad b)
{
	const Vector3& eye = World::instance->current_camera->eye;
	return (a.pos.distance(eye) > b.pos.distance(eye));
}

void QuadCloud::renderEntity()
{
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_CULL_FACE );
	//glDisable( GL_DEPTH_TEST );

	//glEnable( GL_ALPHA_TEST );
	//glAlphaFunc( GL_GEQUAL, fabs( sin( (float)getTime() * 0.0001 ) ) );

	if (!World::instance->render_wireframe)
		glEnable( GL_BLEND );

	//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
	if (additive_blend)
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	glDepthMask( false );

	Camera* cam = World::instance->current_camera;
	Vector3 top(0,1,0);
	Vector3 right(1,0,0);

	if (vertical_align)
	{
		top = Vector3(0,1,0);
		Vector3 front = cam->getLocalVector( Vector3(0,0,-1) );
		right = top.cross(front);
		right.normalize();
	}
	else
	{
		//top = cam->getLocalVector( top );
		top = cam->up;
		right = cam->getLocalVector( right );
	}
	Vector3 pos;

	std::sort(quads.begin(), quads.end(), distance_func );


	for (size_t i = 0; i < quads.size(); i++)
	{
		//textures[i % textures.size() ]->bind();
		Quad& q = quads[i];
		textures[q.id]->bind();
		glBegin( GL_QUADS );

		pos = modelworld * q.pos;

		float dist = 1.0;
		
		if (vertical_align)
			dist = ((pos - cam->eye) * Vector3(1.0f,0.0f,1.0f)).length(); //2d
		else
			dist = (pos - cam->eye).length();
		float alpha = 1.0f - (0.5f * points_size) / dist;

		glColor4f(entity_color.x,entity_color.y,entity_color.z, alpha);

		right = top.cross( (pos - cam->eye).normalize() ).normalize();

		float size = points_size * q.size;
		if (fixed_size == false)
			size *= (cam->eye - pos).length() / 500.0f;

		if (blinking_freq > 0.0f)
			glColor4f(entity_color.x, entity_color.y, entity_color.z,(0.9f + 0.5 * sin( i*0.5 + (float)getTime() * -0.005 * blinking_freq ) ) );

			glTexCoord2f(0,1);
			glVertex3fv( (pos + (top - right)* size).v );
			glTexCoord2f(0,0);
			glVertex3fv( (pos + (top*-1 - right)* size).v );
			glTexCoord2f(1,0);
			glVertex3fv( (pos + (top*-1 + right)* size).v );
			glTexCoord2f(1,1);
			glVertex3fv( (pos + (top + right)* size).v );
		glEnd();
	}
}

//*********************

Sun::Sun(Entity* parent) : QuadCloud(parent)
{
	points_size = 50;
	additive_blend = true;
	blinking_freq = 0.01;
	quads.push_back( Quad(Vector3(0,0,0)) );
	quads.push_back( Quad(Vector3(3,1,-4)) );
	quads.push_back( Quad(Vector3(+2,-1,3)) );
}

void Sun::postRender()
{
	Camera* cam = World::instance->current_camera;

	Matrix44 mvp = cam->view_matrix * cam->projection_matrix;
	
	/*
	Vector3 screensun = mvp.project2D( cam->eye + getPosition() );
	screensun = screensun * 0.5 + Vector3(0.5,0.5,0.0);
	*/

	Vector3 screensun = cam->project2D(cam->eye + getLocalPosition(), 1,1);

	//std::cout << screensun.z << std::endl;

	if (screensun.z >= 1.0)	return;
	screensun.z = 0.0;

	float alpha = 0.5 - (2*(screensun - Vector3(0.5,0.5,0.0)).length());

	Camera cam2d;
	cam2d.setOrthographic(0,1,1,0,-1,1);
	cam2d.set();

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	glColor4f(1,1,1,alpha);
	glPushMatrix();	
		glTranslatef(screensun.x, screensun.y, 0.0);
		Texture* tex = Texture::Load("textures/rainbow.tga");
		tex->bind();
		drawQuad(0.3,0.3,true);
	glPopMatrix();

	glColor4f(1,1,1,0.5);
	glPushMatrix();	
		glTranslatef(screensun.x, screensun.y, 0.0);
		tex = Texture::Load("textures/gauss.tga");
		tex->bind();
		drawQuad(1.4f,1.4f,true);
	glPopMatrix();
}


DebugEntity::DebugEntity(Entity* parent) : EntityMesh(parent) {}

void DebugEntity::renderEntity()
{
	glDisable( GL_TEXTURE_2D );
	glPushMatrix();
	drawGrid(100,10,true);
	glColor4f(1.0,1.0,1.0,1.0);
#ifdef USE_GLUT
	glutWireSphere(10,10,10);
#endif
	glPopMatrix();
}

void DebugEntity::update(float t)
{
}

//*********************************
/*
TerrainEntity::TerrainEntity(Entity* parent) : EntityMesh(parent)
{
}

void TerrainEntity::renderEntity()
{
	for (int i = 0; i < 10; i++)
	{

	}
	EntityMesh::renderEntity();
}

void TerrainEntity::update(float t)
{
}
*/


MapProp::MapProp(Entity* parent) : EntityMeshCollide(parent)
{
	collision_mode = SPHERE_COLLISION;
	bullet_collision = true;
	health = 4;
}

//plane collision
bool MapProp::onEntityCollision(EntityMeshCollide* object)
{
	explode();
	return true;
}

void MapProp::explode()
{
	//explossion
	ParticleEmissorEntity* pe = new ParticleEmissorEntity(this);
	pe->setupEmissor( ParticleEmissor::FIRE_EXPLOSION );
	pe->start();
	pe->time_to_destroy = 3;

	//smoke
	entity_color.set(0.2,0.2,0.2);
	pe = new ParticleEmissorEntity(this);
	pe->setupEmissor( ParticleEmissor::SMOKE );
	pe->start();

	std::cout << "PROP DESTROYED!!" << std::endl;
}

bool MapProp::onSpecialCollision(char type, float force, Vector3 collision_point, Vector3 params)
{
	if (isTag("hit")) return false;

	health -= force;
	std::cout << "PROP HIT!! " << health << std::endl;


	if (health > 0) return true;

	addTag("hit");
	explode();
	return true;
}


