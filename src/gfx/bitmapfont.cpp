#include "bitmapfont.h"
//#include "XMLDOM.h"
//#include "Utils.h"
//#include "Painter.h"
//#include "ShaderSet.h"

#include "texture.h"
#include "mesh.h"
#include "shader.h"
#include "../utils/math.h"
#include "../utils/utils.h"

#include "../extra/tinyxml/tinyxml.h"

#include <cassert>
#include <iostream>

std::map<std::string,BitmapFont*> BitmapFont::s_loaded_fonts;

BitmapFont* BitmapFont::getFont(const char* filename)
{
	if (filename == NULL)
	{
		if (!s_loaded_fonts.empty())
			return s_loaded_fonts.begin()->second;
		return NULL;
	}

	std::map<std::string,BitmapFont*>::iterator it = s_loaded_fonts.find(filename);
	if (it != s_loaded_fonts.end() )
		return it->second;

	//load
	BitmapFont* f = new BitmapFont();
	if ( f->loadFromXML(filename) == false )
	{
		delete f;
		f = NULL;
		std::cerr << "Font path not found: " << filename << std::endl;
		return NULL;
	}

	s_loaded_fonts[filename] = f;
	return f;
}

void BitmapFont::deInit()
{
	std::map<std::string,BitmapFont*>::iterator it;
	for (it = s_loaded_fonts.begin(); it != s_loaded_fonts.end(); it++)
		delete it->second;
	s_loaded_fonts.clear();
}

BitmapFont::BitmapFont()
{
	texture = NULL;
	current_font_size = 16;
	current_color.set(1.0,1.0,1.0,1.0);
	current_kerning = 1.0;
	current_spacing = 1.0;
	current_line_height = 1.0;
	additive_blending = false;
}

BitmapFont::~BitmapFont()
{
	//Done in the texture manager
	//SAFE_DELETE(texture);
}

Vector2 BitmapFont::renderText(const std::string& text, Vector2 start_pos, int window_width, int window_height, int max_width)
{
	std::vector<Vector3> corners;
	std::vector<Vector2> tcoords;

	//count rendereable chars
	int chars = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		std::map<unsigned int, character>::iterator it = chars_map.find(text[i]);
		if (it != chars_map.end() && text[i] != ' ')
			chars++;
	}

	//alloc buffers
	corners.resize( chars * 4 );
	tcoords.resize( chars * 4 );

	//fill buffers
	Vector3 pos = Vector3( start_pos.x, start_pos.y , 0.0 );
	int chars_rendered = 0;
	float i_w = 1.0 / (float)scale_width;
	float i_h = 1.0 / (float)scale_height;
	int max_x = 0;
	character* last_character = NULL;
	
	float scale = current_font_size / (float)font_size;

	for (size_t i = 0; i < text.size(); i++)
	{
		int id = text[i];

		if (id == '\n')
		{
			pos.x = start_pos.x;
			pos.y += line_height * scale * current_line_height;
		}
//		else if (id == ' ')
//			pos.x += base;
		else if (id == '\t')
			pos.x += base * scale * 4;
		else 
		{
			std::map<unsigned int, character>::iterator it = chars_map.find(id);
			if (it == chars_map.end()) //not found
				continue;

			character& c = it->second;

			//Kerning
			if (last_character != NULL)
			{
				std::map<unsigned int, int>::iterator it_k = last_character->kerning.find(c.id);
				if (it_k != last_character->kerning.end() )
					pos.x += it_k->second * scale * current_kerning;
			}

			//break line when max width reached
			
			if ( max_width != -1 && (pos.x + c.xadv * scale * current_spacing) > start_pos.x + max_width )
			{
				pos.x = start_pos.x;
				pos.y += line_height * scale * current_line_height;
			}
			

			//offset vector
			Vector3 off( -c.xoff * scale, +c.yoff * scale, 0.0 );

			//rendereable character
			if (c.id != ' ')
			{
				assert( chars_rendered < chars );

				//four corners
				corners[ chars_rendered * 4 + 0 ] = pos + off * Vector3(1,1,0);
				corners[ chars_rendered * 4 + 1 ] = pos + off * Vector3(-1,1,0) + Vector3(c.w * scale, 0, 0);
				corners[ chars_rendered * 4 + 2 ] = pos + off * Vector3(-1,1,0) + Vector3(c.w * scale, c.h * scale, 0);
				corners[ chars_rendered * 4 + 3 ] = pos + off * Vector3(1,1,0) + Vector3(0, c.h * scale, 0);

				//tex coords
				tcoords[ chars_rendered * 4 + 0 ] = Vector2( c.x * i_w, c.y * i_h );
				tcoords[ chars_rendered * 4 + 1 ] = Vector2( (c.x + c.w) * i_w, c.y * i_h );
				tcoords[ chars_rendered * 4 + 2 ] = Vector2( (c.x + c.w) * i_w, (c.y + c.h) * i_h );
				tcoords[ chars_rendered * 4 + 3 ] = Vector2( c.x * i_w, (c.y + c.h) * i_h );

				chars_rendered++;
			}

			//spacing
			if (!(c.id == ' ' && pos.x == start_pos.x) )
				pos.x += c.xadv * scale * current_spacing;

			if (max_x < pos.x) max_x = pos.x;
			last_character = &it->second;

			continue;
		}
		last_character = NULL;
	}

	if (chars_rendered == 0)
		return start_pos;

	//blending for alpha
	//glDisable( GL_BLEND );
	
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, ( additive_blending ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA ) );	

	Shader* sh = Shader::getGenericTextureShader();
	sh->enable();
	sh->setTexture("texture",texture->texture_id);

	Matrix44 mvp;
	mvp.setOrthoProjection(0,window_width,window_height,0,-1,1);
	sh->setMatrix44("mvp",mvp.m);

	Mesh mesh;
	mesh.vertices = corners;
	mesh.uvs = tcoords;

	float maxAniso = 1;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maxAniso);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,maxAniso);//improve readibility

	mesh.render(0,true);

	pos.x = max_x;
	sh->disable();
	return Vector2( max_x, pos.y );
}

float BitmapFont::getLineHeight() const
{
	float scale = current_font_size / (float)font_size;
	return line_height * scale * current_line_height;
}

void BitmapFont::computeRectangle(const std::string& text, float& rx, float& ry) const
{
	rx = 0;
	ry = 0;

	//fill buffers
	Vector3 start_pos(0,0,0);
	Vector3 pos = start_pos;
	int chars_rendered = 0;
	float i_w = 1.0 / (float)scale_width;
	float i_h = 1.0 / (float)scale_height;
	int max_x = 0;
	const character* last_character = NULL;
	
	float scale = current_font_size / (float)font_size;

	for (size_t i = 0; i < text.size(); i++)
	{
		int id = text[i];

		if (id == '\n')
		{
			pos.x = start_pos.x;
			pos.y -= line_height * scale * current_line_height;
		}
//		else if (id == ' ')
//			pos.x += base;
		else if (id == '\t')
			pos.x += base * scale * 4;
		else 
		{
			std::map<unsigned int, character>::const_iterator it = chars_map.find(id);
			if (it == chars_map.end()) //not found
				continue;

			const character& c = it->second;

			//Kerning
			if (last_character != NULL)
			{
				std::map<unsigned int, int>::const_iterator it_k = last_character->kerning.find(c.id);
				if (it_k != last_character->kerning.end() )
					pos.x += it_k->second * scale * current_kerning;
			}

			//spacing
			if (!(c.id == ' ' && pos.x == start_pos.x) )
				pos.x += c.xadv * scale * current_spacing;

			if (max_x < pos.x) max_x = pos.x;
			last_character = &it->second;

			continue;
		}
		last_character = NULL;
	}

	rx = pos.x;
	ry = pos.y - getLineHeight();
}

Vector3 BitmapFont::renderText3D(const std::string& text, const Vector3& start_pos, const Matrix44& vp) const
{
	std::vector<Vector3> corners;
	std::vector<Vector2> tcoords;

	//count rendereable chars
	int chars = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		std::map<unsigned int, character>::const_iterator it = chars_map.find(text[i]);
		if (it != chars_map.end() && text[i] != ' ')
			chars++;
	}

	//alloc buffers
	corners.resize( chars * 4 );
	tcoords.resize( chars * 4 );

	//fill buffers
	Vector3 pos = start_pos;
	int chars_rendered = 0;
	float i_w = 1.0 / (float)scale_width;
	float i_h = 1.0 / (float)scale_height;
	int max_x = 0;
	const character* last_character = NULL;
	
	float scale = current_font_size / (float)font_size;

	for (size_t i = 0; i < text.size(); i++)
	{
		int id = text[i];

		if (id == '\n')
		{
			pos.x = start_pos.x;
			pos.y -= line_height * scale * current_line_height;
		}
//		else if (id == ' ')
//			pos.x += base;
		else if (id == '\t')
			pos.x += base * scale * 4;
		else 
		{
			std::map<unsigned int, character>::const_iterator it = chars_map.find(id);
			if (it == chars_map.end()) //not found
				continue;

			const character& c = it->second;

			//Kerning
			if (last_character != NULL)
			{
				std::map<unsigned int, int>::const_iterator it_k = last_character->kerning.find(c.id);
				if (it_k != last_character->kerning.end() )
					pos.x += it_k->second * scale * current_kerning;
			}			

			//offset vector
			

			//rendereable character
			if (c.id != ' ')
			{
				assert( chars_rendered < chars );

				Vector3 off( -c.xoff * scale, +c.yoff * scale, 0.0 );			
				//four corners
				corners[ chars_rendered * 4 + 0 ] = pos + off * Vector3(1,-1,0);
				corners[ chars_rendered * 4 + 1 ] = pos + off * Vector3(-1,-1,0) + Vector3(c.w * scale, 0, 0);
				corners[ chars_rendered * 4 + 2 ] = pos + off * Vector3(-1,-1,0) + Vector3(c.w * scale, -c.h * scale, 0);
				corners[ chars_rendered * 4 + 3 ] = pos + off * Vector3(1,-1,0) + Vector3(0, -c.h * scale, 0);

				//tex coords				
				tcoords[ chars_rendered * 4 + 0 ] = Vector2( c.x * i_w, c.y * i_h );
				tcoords[ chars_rendered * 4 + 1 ] = Vector2( (c.x + c.w) * i_w, c.y * i_h );
				tcoords[ chars_rendered * 4 + 2 ] = Vector2( (c.x + c.w) * i_w, (c.y + c.h) * i_h );
				tcoords[ chars_rendered * 4 + 3 ] = Vector2( c.x * i_w, (c.y + c.h) * i_h );

				chars_rendered++;
			}

			//spacing
			if (!(c.id == ' ' && pos.x == start_pos.x) )
				pos.x += c.xadv * scale * current_spacing;

			if (max_x < pos.x) max_x = pos.x;
			last_character = &it->second;

			continue;
		}
		last_character = NULL;
	}

	if (chars_rendered == 0)
		return start_pos;

	//blending for alpha
	//glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, ( additive_blending ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA ) );

	/*
	// draws polygon
	ShaderSet* sh = Painter::getTextureShader();
	sh->enable();

	// set shader parameters	
	sh->setMatrix44("mvp",vp);

	sh->setUniform4Array("color",&current_color.x,1);
	assert(texture != NULL);
	texture->set(5); //why 5? there is a problem with the pipeline because both use the sloth 0

	float maxAniso = 1;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&maxAniso);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,maxAniso);//improve readibility

	sh->setTexture("tex",5);
	int loc = sh->getAttribLocation("uv");
	Painter::drawQuads(loc, chars_rendered, &corners[0], &tcoords[0]);

	pos.x = max_x;
	sh->disable();
	//glDisable( GL_BLEND );
	*/
	return Vector3( max_x, pos.y, pos.z );
}

void BitmapFont::setFontStyle(float font_size, Vector4 color, bool blend, float kerning, float spacing)
{
	current_font_size = font_size;
	current_color = color;
	current_kerning = kerning;
	current_spacing = spacing;
	additive_blending = blend;
}

void BitmapFont::setColor(const Vector4& color)
{
	current_color = color;
}

bool BitmapFont::loadFromXML(const char* filename)
{
	TiXmlDocument doc;

	if ( doc.LoadFile(filename) )
	{
		std::cerr << "Error: Font not found: " << filename << std::endl; 
		return false;
	}

	font_filename = filename;
	TiXmlElement* elem = NULL;
	TiXmlElement* aux = NULL;

	elem = doc.RootElement()->FirstChildElement("info");
	assert(elem);

	std::string tmp;

	font_name = elem->Attribute("face");
	elem->QueryIntAttribute("size", &font_size);

	elem = doc.RootElement()->FirstChildElement("common");
	assert(elem);

	elem->QueryIntAttribute("lineHeight",&line_height);
	elem->QueryIntAttribute("base",&base);
	elem->QueryIntAttribute("scaleW",&scale_width);
	elem->QueryIntAttribute("scaleH",&scale_height);

	elem = doc.RootElement()->FirstChildElement("pages");
	assert(elem && !elem->NoChildren());

	aux = elem->FirstChildElement();
	assert( std::string(aux->Value()) == "page");

	texture_filename = aux->Attribute("file");
	std::string path = getPath(filename);

	texture = Texture::Load( (path + std::string("/") + texture_filename).c_str() );
	if (texture == NULL)
	{
		std::cerr << "Error: Texture Font not found: " << texture_filename << std::endl; 
		return false;
	}

	//texture->setMinifyingFunction( Texture::MIN_MIPMAP_LINEAR ); //TODO

	elem = doc.RootElement()->FirstChildElement("chars");
	assert(elem);
	int count = 0;
	elem->QueryIntAttribute("count",&count);
	//assert (elem->getNumberOfChildren() == count);

	aux = elem->FirstChildElement();
	while(aux)
	{
		if ( strcmp(aux->Value(),"char") )
			continue;

		character c;
		aux->QueryUnsignedAttribute("id",&c.id);
		aux->QueryIntAttribute("x",&c.x);
		aux->QueryIntAttribute("y",&c.y);
		aux->QueryIntAttribute("width",&c.w);
		aux->QueryIntAttribute("height",&c.h);
		aux->QueryIntAttribute("xoffset",&c.xoff);
		aux->QueryIntAttribute("yoffset",&c.yoff);
		aux->QueryIntAttribute("xadvance",&c.xadv);

		chars_map[ c.id ] = c;

		aux = elem->NextSiblingElement();
	}

	elem = doc.RootElement()->FirstChildElement("kernings");
	assert(elem);
	aux = elem->FirstChildElement();
	int first=0,second=0,amount=0;
	while(aux)
	{
		if ( strcmp(aux->Value(), "kerning"))
			continue;

		aux->QueryIntAttribute("first",&first);
		aux->QueryIntAttribute("second",&second);
		aux->QueryIntAttribute("amount",&amount);
		chars_map[first].kerning[second] = amount;
	}

	return true;
}
