#ifndef PARTICLES_H
#define PARTICLES_H

#include "../utils/math.h"
#include <vector>
#include <list>
#include "../world/entity.h"

class Particle
{
public:
	Vector3 pos;
	Vector3 dir;
	float reamining_time;
};

class Texture;

//******************************
class ParticleEmissor
{
	static std::list<ParticleEmissor*> sParticleEmissors;

public:
	static bool render_debug;

	enum {
		STOPPED = 0,
		EMITTING = 1,
		FINISHED = 2
	};

	enum {
		SMOKE,
		SMOKE_ENGINE, //when engine works
		SMOKE_ENGINE_BURNING, //when engine burns
		SMOKE_EXPLOSION, //when something explodes and throws smoke
		EXPLOSION,
		DUST,
		FIRE,
		FIRE_EXPLOSION,
		PLASMA_EXPLOSSION,
		FLASH,
		FAIRY,
		WATER_EXPLOSSION,
		BUBBLES,
		GAUSS_ENGINE,
		SPLASH,
		GUNS_FIRE
	};



	//Config for particles
	Vector3 start_color;
	Vector3 end_color;

	double start_alpha;
	double end_alpha;

	double start_size;
	double end_size;

	bool blend_particles;

	Vector3 gravity;
	Vector3 start_velocity;

	Vector3 min_velocity; //define a bounding for direction
	Vector3 max_velocity;

	bool front_aligned;
	bool vertical_align;
	bool velocity_dependant;

	Texture* texture;

	//Emissor
	char emissor_state; //0 -> stopped, 1 -> emiting, -1 -> finished
	double particle_life;	//duration in seconds
	Vector3 emissor_position;
	Vector3 emissor_last_position;
	Vector3 emissor_front;

	double time_between_particles;	//time between one particle and the next one
	int num_particles_to_emit;	//total amount of particles to emit (-1 means forever)
	int max_particles;	//max number of particles simultaneos in screen (reduce it on slow cards)

	double time_since_last_particle;

	std::vector<Particle> particles;

	ParticleEmissor();
	virtual ~ParticleEmissor();

	void init();

	void createParticle();
	void start();
	void stop();

	void scale(float f);

	void renderParticles();
	void updateParticles(float seconds_elapsed);

	static void RenderAll();
	static void UpdateAll(float seconds_elapsed);
};

class ParticleEmissorEntity : public Entity, public ParticleEmissor
{
public:
	ParticleEmissorEntity( Entity* parent );
	void update(float time);
	virtual const char* getClassName() { return "ParticleEmissorEntity"; }

};


#endif