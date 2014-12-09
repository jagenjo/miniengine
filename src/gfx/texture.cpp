#include "texture.h"
#include "../utils/utils.h"

#include <iostream> //to output
#include <cmath>

std::map<std::string, Texture*> Texture::sTexturesLoaded;
glGenerateMipmapEXT_func glGenerateMipmapEXT = NULL;

Texture::Texture()
{
	width = 0;
	height = 0;
	hasMipmaps = false;

	if(glGenerateMipmapEXT == NULL) //get the extension
		glGenerateMipmapEXT = (glGenerateMipmapEXT_func) SDL_GL_GetProcAddress("glGenerateMipmapEXT");
}


Texture* Texture::Load(const char* filename)
{
	std::map<std::string, Texture*>::iterator it = sTexturesLoaded.find(filename);
	if (it == sTexturesLoaded.end())
	{
		std::cout << "Texture loading: " << filename << " ... ";
		long time = getTime();
		Texture* t = new Texture();
		if (t->load(filename) == false)
		{
			delete t;
			std::cout << "[ERROR]: Texture not found" << std::endl;
			return NULL;
		}
		std::cout << "[OK] Size: " << t->width << "," << t->height << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
		sTexturesLoaded[filename] = t;
		return t;
	}
	return it->second;
}

bool Texture::load(const char* filename)
{
	std::string str = filename;
	std::string ext = str.substr( str.size() - 4,4 );

	if (ext == ".tga" || ext == ".TGA")
	{
		TGAInfo* tgainfo = loadTGA(filename);
		if (tgainfo == NULL)
			return false;

		this->filename = filename;

		//How to store a texture in VRAM
		glGenTextures(1, &texture_id); //we need to create an unique ID for the texture
		glBindTexture(GL_TEXTURE_2D, texture_id);	//we activate this id to tell opengl we are going to use this texture
		//glTexImage2D(GL_TEXTURE_2D, 0, 3, tgainfo->width, tgainfo->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tgainfo->data); //upload without mipmaps
		gluBuild2DMipmaps(GL_TEXTURE_2D, ( tgainfo->bpp == 24 ? 3 : 4), tgainfo->width, tgainfo->height, ( tgainfo->bpp == 24 ? GL_RGB : GL_RGBA) , GL_UNSIGNED_BYTE, tgainfo->data); //upload the texture and create their mipmaps
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); //set the mag filter
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	//set the min filter
		width = tgainfo->width;
		height = tgainfo->height;
		delete tgainfo->data;
		delete tgainfo;
		return true;
	}
	else if (ext == ".dds" || ext == ".DDS")
	{
		if (loadDDS(filename) == false)
			return false;
		return true;
	}
	return false;
}

void Texture::bind()
{
	glEnable( GL_TEXTURE_2D ); //enable the textures 
	glBindTexture( GL_TEXTURE_2D, texture_id );	//enable the id of the texture we are going to use
}

void Texture::unbind()
{
	glEnable( GL_TEXTURE_2D ); //enable the textures 
	glBindTexture( GL_TEXTURE_2D, 0 );	//enable the id of the texture we are going to use
}

void Texture::generateMipmaps()
{
	if(!glGenerateMipmapEXT) return;

	glBindTexture( GL_TEXTURE_2D, texture_id );	//enable the id of the texture we are going to use
	glGenerateMipmapEXT(GL_TEXTURE_2D);
}

Texture::TGAInfo* Texture::loadTGA(const char* filename)
{
    GLubyte TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    GLubyte TGAcompare[12];
    GLubyte header[6];
    GLuint bytesPerPixel;
    GLuint imageSize;
    GLuint temp;
    GLuint type = GL_RGBA;

    FILE * file = fopen(filename, "rb");
    
    if (file == NULL || fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
        memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
        fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        if (file == NULL)
            return NULL;
        else
        {
			std::cerr << "TGA file has wrong encoding" << std::endl;
            fclose(file);
            return NULL;
        }
    }

	TGAInfo* tgainfo = new TGAInfo;
    
    tgainfo->width = header[1] * 256 + header[0];
    tgainfo->height = header[3] * 256 + header[2];
    
    if (tgainfo->width <= 0 || tgainfo->height <= 0 || (header[4] != 24 && header[4] != 32))
    {
        fclose(file);
		delete tgainfo;
        return NULL;
    }
    
    tgainfo->bpp = header[4];
    bytesPerPixel = tgainfo->bpp / 8;
    imageSize = tgainfo->width * tgainfo->height * bytesPerPixel;
    
    tgainfo->data = (GLubyte*)malloc(imageSize);
    
    if (tgainfo->data == NULL || fread(tgainfo->data, 1, imageSize, file) != imageSize)
    {
        if (tgainfo->data != NULL)
            free(tgainfo->data);
            
        fclose(file);
		delete tgainfo;
        return NULL;
    }
    
    for (GLuint i = 0; i < int(imageSize); i += bytesPerPixel)
    {
        temp = tgainfo->data[i];
        tgainfo->data[i] = tgainfo->data[i + 2];
        tgainfo->data[i + 2] = temp;
    }
    
    fclose(file);

	return tgainfo;
}
