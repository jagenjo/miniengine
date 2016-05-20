#include "particles.h"

#include <cassert>
#include <algorithm>

#include "../world/entity.h"
#include "../world/world.h"

#include "texture.h"
#include "camera.h"
#include "mesh.h"

#include "../utils/utils.h"


std::list<ParticleEmissor*> ParticleEmissor::sParticleEmissors;
bool ParticleEmissor::render_debug = false;

ParticleEmissor::ParticleEmissor()
{
	sParticleEmissors.push_back(this);
	ParticleEmissor::init();
}

ParticleEmissor::~ParticleEmissor()
{
	std::list<ParticleEmissor*>::iterator it = std::find(sParticleEmissors.begin(),sParticleEmissors.end(),this);
	if (it != sParticleEmissors.end() )
		sParticleEmissors.erase(it);
}

void ParticleEmissor::init()
{
	time_since_last_particle = 0;
	num_particles_to_emit = 0;
	texture = NULL;
	emissor_state = 0;
	num_particles_to_emit = 100;
	max_particles = 100;
	min_velocity.set(-1,-1,-1);
	max_velocity.set(1,1,1);

	start_velocity.set(0,0,0);
	blend_particles = false;
	vertical_align = false;
	velocity_dependant = false;
	front_aligned = false;
	emissor_front.set(0,0,-1);
}

void ParticleEmissor::renderParticles()
{
	if (particles.empty())
		return;

	if (texture == NULL)
		return;

	Camera* cam = World::instance->current_camera;

	Vector3 up, right;
	if (!vertical_align)
	{
		up = cam->getLocalVector(Vector3(0,1,0));
		right = cam->getLocalVector(Vector3(1,0,0));
	}
	else
	{
		up = Vector3(0,0,1);
		right = Vector3(-1,0,0);
	}

	/*
	glPointSize(15);
	glColor3f(0,1,1);
	glBegin( GL_POINTS );
		glVertex3fv( emisor_position.v );
	glEnd();
	//*/

	/*
	glColor3f(0,1,0);
	glBegin( GL_LINES );
		glVertex3fv( emisor_position.v );
		glVertex3fv( (emisor_position + up * 1000).v );
	glEnd();
	glColor3f(1,0,0);
	glBegin( GL_LINES );
		glVertex3fv( emisor_position.v );
		glVertex3fv( (emisor_position + right * 1000).v );
	glEnd();
	//*/
	glEnable( GL_BLEND );
	if (blend_particles)
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	else
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	/*
	float fog[4] = {1.0,1.0,1.0,0.0};
	memcpy(fog,start_color.v,sizeof(Vector3));
	glFogfv(GL_FOG_COLOR,fog);
	*/

	assert(texture);

	if (render_debug)
		glDisable(GL_TEXTURE_2D);
	else
		texture->bind();

	Vector4 color;

	static Mesh mesh;

	mesh.primitive = GL_QUADS;
	mesh.vertices.resize(particles.size() * 4);
	mesh.uvs.resize(particles.size() * 4);
	mesh.colors.resize(particles.size() * 4);

	int painted_particles = 0;
	for (size_t i = 0; i < particles.size(); ++i)
	{
		Particle& p = particles[i];
		if (p.reamining_time <= 0)
			continue;

		double factor = 1 - (p.reamining_time / particle_life);
		double alpha = start_alpha * (1-factor) + end_alpha * factor;
		if (alpha <= 0.01)	continue;

		double size = (start_size * (1-factor) + end_size * factor);

		color = start_color * (1-factor) + end_color * factor;
		color.a = alpha;

		mesh.colors[painted_particles*4] = color;
		mesh.colors[painted_particles*4+1] = color;
		mesh.colors[painted_particles*4+2] = color;
		mesh.colors[painted_particles*4+3] = color;

		mesh.uvs[painted_particles*4].set(0,0);
		mesh.uvs[painted_particles*4+1].set(0,1);
		mesh.uvs[painted_particles*4+2].set(1,1);
		mesh.uvs[painted_particles*4+3].set(1,0);

		mesh.vertices[painted_particles*4] = p.pos + up * size - right * size;
		mesh.vertices[painted_particles*4+1] = p.pos - up * size - right * size;
		mesh.vertices[painted_particles*4+2] = p.pos - up * size + right * size;
		mesh.vertices[painted_particles*4+3] = p.pos + up * size + right * size;

		painted_particles++;
	}

	if (painted_particles)
	{
		mesh.vertices.resize(painted_particles*4);
		mesh.uvs.resize(painted_particles*4);
		mesh.colors.resize(painted_particles*4);

		mesh.render(0,true);
	}

	glDisable( GL_BLEND );
	glDisable( GL_TEXTURE_2D );
}

void ParticleEmissor::updateParticles(float seconds_elapsed)
{
	//update
	for (size_t i = 0; i < particles.size(); ++i)
	{
		Particle& p = particles[i];
		if (p.reamining_time <= 0)
			continue;

		p.reamining_time -= seconds_elapsed;
		p.pos = p.pos + p.dir * seconds_elapsed;
		p.dir = p.dir + gravity * seconds_elapsed;
	}

	if (emissor_state == 0)
		return;

	time_since_last_particle += seconds_elapsed;
	int particles_to_create;
	if (time_between_particles == 0)
		particles_to_create = num_particles_to_emit;
	else
		particles_to_create = int(time_since_last_particle / time_between_particles);
	for (int j = 0; j < particles_to_create; j++)
		createParticle();
}


void ParticleEmissor::start()
{
	emissor_state = 1;
	if (particles.size() < (size_t)max_particles)
	{
		int old = particles.size();
		particles.resize(max_particles);
		memset( (void*)&particles[old], 0, (max_particles - old) * sizeof( Particle ) );
	}

	updateParticles(0);
}

void ParticleEmissor::stop()
{
	emissor_state = 0;
}

void ParticleEmissor::createParticle()
{
	time_since_last_particle = 0;
	for (size_t i = 0; i < particles.size(); ++i)
		if ( particles[i].reamining_time <= 0)
		{
			particles[i].pos = emissor_position + (emissor_last_position - emissor_position) * ((rand()%1000)/1000.0);


			Vector3 r;
			if (front_aligned)
			{
				particles[i].dir = emissor_front;
			}
			else
			{
				r.random(max_velocity - min_velocity);
				r = r + min_velocity;
				particles[i].dir = r;
			}
			
			if (velocity_dependant)
				particles[i].dir += start_velocity;

			particles[i].reamining_time = particle_life;

			if (num_particles_to_emit > 0)
				num_particles_to_emit--;
			if (num_particles_to_emit == 0)
				stop();
			return;
		}
}

void ParticleEmissor::scale(float f)
{
	start_size *= f;
	end_size *= f;
	min_velocity = min_velocity * f;
	max_velocity = max_velocity * f;
	gravity = gravity * f;
}


//*******************************
ParticleEmissorEntity::ParticleEmissorEntity( Entity* parent ) : Entity(parent)
{
	setName("ParticleEmissorEntity");
}


void ParticleEmissorEntity::update(float time)
{
	Entity::update(time);
	emissor_last_position = emissor_position;
	emissor_position = getModelWorld().getTranslation();
	emissor_front = getModelWorld().rotateVector(Vector3(0,0,-1));
	start_velocity = getVelocityWorld();
}

//**************************************
void ParticleEmissor::RenderAll()
{
	//glDisable( GL_CULL_FACE );
	glDepthMask(false);
	glEnable( GL_DEPTH_TEST );

	std::list<ParticleEmissor*>::iterator it = sParticleEmissors.begin();
		for (; it != sParticleEmissors.end(); it++)
			(*it)->renderParticles();

	glDepthMask(true);

}

void ParticleEmissor::UpdateAll(float seconds_elapsed)
{
	std::list<ParticleEmissor*>::iterator it = sParticleEmissors.begin();
		for (; it != sParticleEmissors.end(); it++)
			(*it)->updateParticles(seconds_elapsed);
}
