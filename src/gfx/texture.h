#ifndef TEXTURE_H
#define TEXTURE_H

#include "../includes.h"
#include <map>
#include <string>

//function to create mipmaps using the GPU (much faster)
typedef void (APIENTRY *glGenerateMipmapEXT_func)( GLenum target );
extern "C" glGenerateMipmapEXT_func glGenerateMipmapEXT;


class Texture
{
	typedef struct sTGAInfo //a general struct to store all the information about a TGA file
	{
		GLuint width;
		GLuint height;
		GLuint bpp; //bits per pixel
		GLubyte* data; //bytes with the pixel information
	} TGAInfo;

	static std::map<std::string, Texture*> sTexturesLoaded;

public:
	GLuint texture_id; // GL id to identify the texture in opengl, every texture must have its own id
	float width;
	float height;
	std::string filename;
	bool hasMipmaps;

	static Texture* Load(const char* filename);
	Texture();
	void bind();
	static void unbind();
	bool load(const char* filename);

	void generateMipmaps();

protected:
	TGAInfo* loadTGA(const char* filename);
	bool loadDDS(const char* filename);
};

#endif