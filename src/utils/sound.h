#ifndef SOUND_H
#define SOUND_H

#include "../includes.h"
#ifdef BASS_SOUND
	#include <bass.h>
	#pragma comment(lib, "bass.lib")
#endif
#include <map>
#include <string>

#include "../utils/math.h"

class Camera;


class AudioFile
{
public:

#ifdef BASS_SOUND
	HSAMPLE hSample;
#else
	int hSample;
#endif

	static std::map<std::string, AudioFile*> sSamplesLoaded;
	static AudioFile* Load(const char* filename, bool allows_3d);

	AudioFile();
	~AudioFile();
	bool load(const char* filename, bool allows_3d);
};

class AudioSample
{
	AudioFile* sample;
#ifdef BASS_SOUND
	HCHANNEL hSampleChannel;
#else
	int hSampleChannel;
#endif

	bool is_3d;
	Vector3 position;
	float min_distance; //closer to this the sounds plays at 1.0 Volume
	float max_distance; //further from this the sound doesnt play
	bool is_loop;
	float volume;

	public:
	AudioSample(const char* filename, bool allows_3d = true);
	~AudioSample();

	void play3D(float volume, Vector3& pos, Vector3& vel, bool loop = false);
	void play(float volume = 1.0, bool loop = false);
	void stop();

	void update(Vector3& pos, Vector3& vel);

	bool isPlaying();

	void set3D(Vector3& pos, Vector3& vel);
	void setVolume(float v);
	void setDistances(float min_dist, float max_dist);
	void setSamplingRateFactor(float f);
};

class AudioManager
{
public:
	static Vector3 camera_pos;
	static Vector3 prev_camera_pos;

	static void init(int device = -1); //-1 default
	static void deinit();

	static void setCamera(Camera*, float elapsed = 1.0);
	static void playSound(const char* filename, Vector3 pos, Vector3 vel, float volume, float mindist, float maxdist);
};


#endif