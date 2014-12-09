#ifndef UTILS_H
#define UTILS_H

#include "math.h"

#define CLAMP(a,min,max) (a<min?min: (a>max?max:a))

//General functions **************
long getTime();

std::string getResourceFilename(const char* name);
std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings = false);

std::string getPath(const char* filename);

bool checkGLErrors();

//generic purposes fuctions
void drawGrid(float dist, int num_lines, bool flat);
void drawQuad(float width, float height, bool centered = false, bool wire=false);

//mapped as in SDL
enum XBOXpad
{
	//axis
	LEFT_ANALOG_X = 0,
	LEFT_ANALOG_Y = 1,
	RIGHT_ANALOG_X = 4,
	RIGHT_ANALOG_Y = 3,
	TRIGGERS = 2, //both triggers share an axis (positive is right, negative is left trigger)

	//buttons
	A_BUTTON = 0,
	B_BUTTON = 1,
	X_BUTTON = 2,
	Y_BUTTON = 3,
	LB_BUTTON = 4,
	RB_BUTTON = 5,
	BACK_BUTTON = 6,
	START_BUTTON = 7,
	LEFT_ANALOG_BUTTON = 8,
	RIGHT_ANALOG_BUTTON = 9
};

enum HATState
{
	HAT_CENTERED = 0x00,
	HAT_UP = 0x01,
	HAT_RIGHT = 0x02,
	HAT_DOWN = 0x04,
	HAT_LEFT = 0x08,
	HAT_RIGHTUP = (HAT_RIGHT|HAT_UP),
	HAT_RIGHTDOWN = (HAT_RIGHT|HAT_DOWN),
	HAT_LEFTUP = (HAT_LEFT|HAT_UP),
	HAT_LEFTDOWN = (HAT_LEFT|HAT_DOWN)
};

struct JoystickState
{
	int num_axis;	//num analog sticks
	int num_buttons; //num buttons
	float axis[8]; //analog stick
	char button[16]; //buttons
	HATState hat; //digital pad
};

#endif
