#ifndef INC_BITMAPFONT_H
#define INC_BITMAPFONT_H

//It uses angel code bitmap font generator

#include <string>
#include <vector>
#include <map>
#include "../utils/math.h"

class Texture;
class Matrix44;

class BitmapFont {

	std::string font_name;
	std::string font_filename;
	std::string texture_filename;
	Texture* texture;

	int font_size;
	int line_height;
	int base; //space dist?
	int scale_width;
	int scale_height;

	typedef struct {
		unsigned int id;
		int x,y,w,h,xoff,yoff,xadv;
		float ix,iy;
		std::map<unsigned int, int> kerning;
	} character;

	std::map<unsigned int, character> chars_map;

	//rendering style
	float current_font_size;
	Vector4 current_color;
	float current_kerning;
	float current_spacing;
	float current_line_height;
	bool additive_blending;

	static std::map<std::string,BitmapFont*> s_loaded_fonts;
	static std::map<std::string,BitmapFont*> s_fonts;

	BitmapFont();
	virtual ~BitmapFont();

public:

	static BitmapFont* getFont(const char* filename); //if you pass null it returns the first font loaded
	static void deInit();

	void setFontStyle(float font_size, Vector4 color, bool blend = false, float kerning = 1.0, float spacing = 1.0);

	void setColor(const Vector4& color);

	Vector2 renderText(const std::string& text, Vector2 pos, int window_width, int window_height, int max_width = -1);

	//! Renders the text in the given 3d position
	Vector3 renderText3D(const std::string& text, const Vector3& pos, const Matrix44& vp) const;

	//! Returns the line height
	float getLineHeight() const;

	//! Computes minimum rectangle that contains the text
	void computeRectangle(const std::string& text, float& x, float& y) const;

	bool loadFromXML(const char* filename);
};


#endif