#include "sound.h"
#include <iostream>
#include <cassert>

#include "../gfx/camera.h"

std::map<std::string, AudioFile*> AudioFile::sSamplesLoaded;

AudioFile* AudioFile::Load(const char* filename, bool allows_3d)
{
	std::map<std::string, AudioFile*>::iterator it = sSamplesLoaded.find(filename);
	if (it == sSamplesLoaded.end() )
	{
		AudioFile* a = new AudioFile();
		if (a->load(filename, allows_3d) == false)
		{
			delete a;
			std::cout << "Error: Audio not found: " << filename << std::endl;
			return NULL;
		}
		std::cout << "Audio loaded: " << filename << std::endl;
		sSamplesLoaded[filename] = a;
		return a;
	}
	return it->second;
}

AudioFile::AudioFile()
{
	hSample = 0;
}

AudioFile::~AudioFile()
{
#ifdef BASS_SOUND
	if (hSample != 0) 
		BASS_SampleFree(hSample);
#endif
}

bool AudioFile::load(const char* filename, bool allows_3d)
{
#ifdef BASS_SOUND
	hSample = BASS_SampleLoad(false,filename,0,0,3,allows_3d ? (BASS_SAMPLE_3D | BASS_SAMPLE_OVER_DIST): 0 ); //BASS_SAMPLE_LOOP
	if (hSample == 0)
	{
		int err = BASS_ErrorGetCode();
		std::cerr << "Error ["<< err <<"] while loading sample: " << filename << std::endl;
		if (allows_3d && err == -1)
			std::cerr << "\t3D Sounds cant be Stereo" << std::endl;
	}

	BASS_SAMPLE info;
	BASS_SampleGetInfo(hSample, &info);
	//info.volume = volume; //set data
	
	BASS_SampleSetInfo(hSample, &info);

#endif
	return true;
}

//*************************+

AudioSample::AudioSample(const char* filename, bool allows_3d)
{
	hSampleChannel = 0;
	sample = AudioFile::Load(filename, allows_3d);
	min_distance = 0;
	max_distance = 0;
	is_3d = allows_3d;
	is_loop = false;
	volume = 1.0;
}

AudioSample::~AudioSample()
{
#ifdef BASS_SOUND
	if(hSampleChannel)
		BASS_ChannelStop(hSampleChannel);
#endif
}

void AudioSample::play(float volume, bool loop)
{
	assert(sample);
	BOOL result = TRUE;
	is_loop = loop;
	this->volume = volume;

#ifdef BASS_SOUND
	if (sample->hSample == 0)
	{
		std::cerr << "Error no sample id" << std::endl;
		return;
	}

	if (hSampleChannel == 0) //Create channel for the first time
		hSampleChannel = BASS_SampleGetChannel(sample->hSample,false); //bool: only new channel
	if (hSampleChannel == 0)
	{
		int err = BASS_ErrorGetCode(); 
		if (err != BASS_ERROR_NOCHAN) //no channels free
			std::cerr << "Error ["<< err <<"] no channel id" << std::endl;
		return;
	}

	setVolume(volume);
	if (loop) BASS_ChannelFlags(hSampleChannel, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
	result = BASS_ChannelPlay(hSampleChannel, true);

	if (result == FALSE)
		std::cerr << "Error ["<< BASS_ErrorGetCode() <<"] while playing sample" << std::endl;
#endif

}

void AudioSample::play3D(float volume, Vector3& pos, Vector3& vel, bool loop)
{
#ifdef BASS_SOUND

	assert(sample);
	BOOL result = TRUE;

	if (sample->hSample == 0)
	{
		std::cerr << "Error no sample id" << std::endl;
		return;
	}

	this->is_loop = loop;
	this->volume = volume;

	//too far
	if (max_distance && pos.distance(AudioManager::camera_pos) > max_distance)
	{
		position = pos;
		return;
	}

	//create sound
	if (hSampleChannel == 0)
		hSampleChannel = BASS_SampleGetChannel(sample->hSample,false); //bool: only new channel

	//did the sound was created?
	if (hSampleChannel == 0)
	{
		int err = BASS_ErrorGetCode(); 
		if (err != BASS_ERROR_NOCHAN) //no channels free
			std::cerr << "Error ["<< err <<"] no channel id" << std::endl;
		return;
	}

	//sound settings
	set3D(pos,vel); //set position
	setVolume(volume); //set volume
	BASS_ChannelSet3DAttributes(hSampleChannel,-1, min_distance,0,-1,-1,0); //set min distance
	if (loop) BASS_ChannelFlags(hSampleChannel, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);

	//play
	result = BASS_ChannelPlay( hSampleChannel, true );

	//check errors
	if (result == FALSE)
		std::cerr << "Error ["<< BASS_ErrorGetCode() <<"] while playing sample" << std::endl;

#endif
}

bool AudioSample::isPlaying()
{
	if (hSampleChannel == 0) return false;

#ifdef BASS_SOUND
	if (BASS_ChannelIsActive(hSampleChannel) == BASS_ACTIVE_STOPPED) //has finished
		hSampleChannel = 0;
#endif

	if (hSampleChannel) 
		return true;
	return false;
}


void AudioSample::stop()
{
	if (hSampleChannel == 0) return;
#ifdef BASS_SOUND
	BASS_ChannelStop(hSampleChannel);
	hSampleChannel = 0;
#endif
}

void AudioSample::setVolume(float v)
{
	if (hSampleChannel == 0) return;
#ifdef BASS_SOUND
	BASS_ChannelSetAttribute(hSampleChannel,BASS_ATTRIB_VOL,v);
#endif
}

void AudioSample::set3D(Vector3& pos, Vector3& vel)
{
	if (hSampleChannel == 0) return;
	#ifdef BASS_SOUND
	BASS_ChannelSet3DPosition(hSampleChannel,(const BASS_3DVECTOR*)&pos,NULL,(const BASS_3DVECTOR*)&vel);
	#endif

	position = pos;
}

void AudioSample::setDistances(float min_dist, float max_dist)
{
	min_distance = min_dist;
	max_distance = max_dist;

#ifdef BASS_SOUND
	if (hSampleChannel)
		BASS_ChannelSet3DAttributes(hSampleChannel,-1, min_distance,0,-1,-1,0);
#endif
}

void AudioSample::setSamplingRateFactor(float f)
{
	if (hSampleChannel == 0)
		return;
#ifdef BASS_SOUND
	BASS_ChannelSetAttribute(hSampleChannel,BASS_ATTRIB_FREQ,44000 * f);
#endif
}

void AudioSample::update(Vector3& pos, Vector3& vel)
{
	if (is_3d == false) return;

	bool is_playing = isPlaying(); //checks and frees the channel when finished

	//is playing and it is too far from the camera
	if (is_playing && max_distance && pos.distance(AudioManager::camera_pos) > max_distance)
		stop(); //then stop the sample
	else
	{
		if (is_playing) //update properties
		{
			#ifdef BASS_SOUND
				BASS_ChannelSet3DAttributes(hSampleChannel,-1, min_distance,0,-1,-1,0);
			#endif
			set3D(pos, vel);
		}
		else //is not playing
			if (is_loop) //but it should be
				play3D(1.0, pos, vel, true);
	}
}

//******************************************
Vector3 AudioManager::camera_pos(10000,10000,10000);
Vector3 AudioManager::prev_camera_pos(10000,10000,10000);

void AudioManager::init(int device)
{
	#ifdef BASS_SOUND
		BASS_Init(device, 44100, BASS_DEVICE_3D, 0, NULL);
		//BASS_SetVolume(0.1);
		BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE,0.1);
	/*
	BOOL r = BASS_SetEAXParameters(EAX_ENVIRONMENT_MOUNTAINS,1,0.5,0.2);
	if (r == BASS_ERROR_NOEAX)
		std::cerr << "Warning: No EAX supported" << std::endl;
	//*/
	#endif
}

void AudioManager::deinit()
{
	#ifdef BASS_SOUND
		//BASS_SetVolume(1.0);
		BASS_Free();
	#endif
}

void AudioManager::setCamera(Camera* cam, float elapsed)
{
	Vector3 vel;
	if (elapsed != 0)
		vel = (camera_pos - prev_camera_pos) * (1.0 / elapsed);

	#ifdef BASS_SOUND
		BASS_Set3DPosition((const BASS_3DVECTOR*)&cam->eye,(const BASS_3DVECTOR*)&vel,(const BASS_3DVECTOR*)&cam->getLocalVector(Vector3(0,0,1)), (const BASS_3DVECTOR*)&cam->getLocalVector(Vector3(0,1,0)));
		BASS_Apply3D();
	#endif

	prev_camera_pos = camera_pos;
	camera_pos = cam->eye;

	//BASS_SetVolume( sin( GetTickCount() * 0.01 ) );
}

void AudioManager::playSound(const char* filename, Vector3 pos, Vector3 vel, float volume, float mindist, float maxdist)
{
	
}
