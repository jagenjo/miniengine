#include "utils.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/time.h>
#endif

#include "../includes.h"
#include <string>
#include <iostream>
#include <map>

long getTime()
{
	#ifdef WIN32
		return GetTickCount();
	#else
		struct timeval tv;
		gettimeofday(&tv,NULL);
		return (int)(tv.tv_sec*1000 + (tv.tv_usec / 1000));
	#endif
}

//this function is used to access OpenGL Extensions (special features not supported by all cards)
void* getGLProcAddress(const char* name)
{
	return SDL_GL_GetProcAddress(name);
}


bool checkGLErrors()
{
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		std::cerr << "OpenGL Error: " << errString << std::endl;
		return false;
	}

	return true;
}

//Draw the grid
void drawGrid(float dist, int num_lines, bool flat)
{
	glLineWidth(1);
	glColor3f(0.5,0.5,0.5);
	glBegin( GL_LINES );
		for (int i = 0; i <= num_lines * 0.5; ++i)
		{
			float a = dist * num_lines * 0.5;
			float b = i * dist;

			if (i == num_lines * 0.5)
				glColor3f(1,0.25,0.25);
			else if (i%2)
				glColor3f(0.25,0.25,0.25);
			else
				glColor3f(0.5,0.5,0.5);

			if(flat)
			{
				glVertex3f(a,0,b);
				glVertex3f(-a,0,b);
				glVertex3f(a,0,-b);
				glVertex3f(-a,0,-b);

				glVertex3f(b,0,a);
				glVertex3f(b,0,-a);
				glVertex3f(-b,0,a);
				glVertex3f(-b,0,-a);
			}
			else
			{
				glVertex3f(a,-a,b);
				glVertex3f(-a,-a,b);
				glVertex3f(a,-a,-b);
				glVertex3f(-a,-a,-b);

				glVertex3f(b,-a,a);
				glVertex3f(b,-a,-a);
				glVertex3f(-b,-a,a);
				glVertex3f(-b,-a,-a);

				glVertex3f(a,b,-a);
				glVertex3f(-a,b,-a);
				glVertex3f(a,-b,-a);
				glVertex3f(-a,-b,-a);
				glVertex3f(b,a,-a);
				glVertex3f(b,-a,-a);
				glVertex3f(-b,a,-a);
				glVertex3f(-b,-a,-a);
				glVertex3f(a,b,a);
				glVertex3f(-a,b,a);
				glVertex3f(a,-b,a);
				glVertex3f(-a,-b,a);
				glVertex3f(b,a,a);
				glVertex3f(b,-a,a);
				glVertex3f(-b,a,a);
				glVertex3f(-b,-a,a);

				glVertex3f(-a, a,b);
				glVertex3f(-a, -a,b);
				glVertex3f(-a, a,-b);
				glVertex3f(-a, -a,-b);
				glVertex3f(-a, b,a);
				glVertex3f(-a, b,-a);
				glVertex3f(-a, -b,a);
				glVertex3f(-a, -b,-a);
				glVertex3f(a, a,b);
				glVertex3f(a, -a,b);
				glVertex3f(a, a,-b);
				glVertex3f(a, -a,-b);
				glVertex3f(a, b,a);
				glVertex3f(a, b,-a);
				glVertex3f(a, -b,a);
				glVertex3f(a, -b,-a);
			}
		}
	glEnd();
}

void drawQuad(float width, float height, bool centered, bool wire)
{
	glBegin(wire ? GL_LINE_LOOP : GL_QUADS);

		if (centered)
		{
			glTexCoord2f(0,1);
			glVertex2f(-width*0.5, -height*0.5);
			glTexCoord2f(1,1);
			glVertex2f(width*0.5,-height*0.5);
			glTexCoord2f(1,0);
			glVertex2f(width*0.5,height*0.5);
			glTexCoord2f(0,0);
			glVertex2f(-width*0.5,height*0.5);
		}
		else
		{
			glTexCoord2f(0,1);
			glVertex2f(0,0);
			glTexCoord2f(1,1);
			glVertex2f(width,0);
			glTexCoord2f(1,0);
			glVertex2f(width,height);
			glTexCoord2f(0,0);
			glVertex2f(0,height);
		}
	glEnd();
}

std::string getResourceFilename(const char* name)
{
	std::string root_folder(DEFAULT_ASSETS_ROOT_FOLDER);
	return std::string(root_folder + name).c_str();
}

bool checkGL()
{
	#ifndef _DEBUG
		return true;
	#endif
	GLint err = glGetError();
	if (err == GL_NO_ERROR)
		return true;
	std::cout << "GL ERROR:" << std::endl;
	std::cout << gluErrorString(err) << std::endl;	
	return false;
}

std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings )
{
	std::vector<std::string> tokens;

	std::string str;
	char del_size = strlen(delimiters);
	const char* pos = source.c_str();
	char in_string = 0;
	int i = 0;
	while(*pos != 0)
	{
		bool split = false;

		if (!process_strings || (process_strings && in_string == 0))
		{
			for (i = 0; i < del_size && *pos != delimiters[i]; i++);
			if (i != del_size) split = true;
		}

		if (process_strings && (*pos == '\"' || *pos == '\'') )
		{ 
			if (!str.empty() && in_string == 0) //some chars remaining
			{
				tokens.push_back(str);
				str.clear();
			}
			
			in_string = (in_string != 0 ? 0 : *pos );
			if (in_string == 0) 
			{
				str += *pos;
				split = true;
			}
		}

		if (split)
		{
			if (!str.empty())
			{
				tokens.push_back(str);
				str.clear();
			}
		}
		else
			str += *pos;
		pos++;
	}
	if (!str.empty())
		tokens.push_back(str);
	return tokens;
}


std::string getPath(const char* filename)
{
	std::string str(filename);
	int end = str.find_last_of('/');
	int end2 = str.find_last_of('\\');
	end = ( end > end2 ? end : end2 );
	return str.substr(0,end + 1);
}