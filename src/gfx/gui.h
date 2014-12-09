#ifndef INC_GUI_H_
#define INC_GUI_H_

/**************************************************************************\
 
    GTI-Framework - C++ library for realtime graphics rendering using OpenGL.
    Copyright (C) 2001-2008 Universitat Pompeu Fabra - Barcelona (Spain)

    This file is part of GTI-Framework.

    GTI-Framework is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GTI-Framework is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with GTI-Framework.  If not, see <http://www.gnu.org/licenses/>.

/*****************************************************************************/
#include "../includes.h"

#include "XMLDOM.h"
#include <string>
#include <vector>
#include <list>

#include <cassert>
#include "Events.h"
#include "Camera.h"
#include "Parameter.h"
#include "MatricesStack.h"
#include "Math/Quaternion.h"

namespace gti {

	class Widget;
	class Texture;
	class BitmapFont;

	// -----------------------------------------------------

	//minimal area
	struct Area {
		float       left;
		float       width;
		float       top;
		float       height;
		Area( ) : left( 0.0f ), width( 0.1f ), top( 0.0f ), height( 0.1f ) { }
		Area( float aleft, float atop, float awidth, float aheight );
		void set(float l, float t, float w, float h) { left = l; top = t; width = w; height = h; }

		bool isInside( const vector2f &pos ) const;
		void growToInclude( const Area &area );
		Area overlap( const Area &area );
		vector2f inLocalCoords( const vector2f &pos ) const;
	};

	//base class
	class Widget : public EventReceiver, public EventDispatcher
	{
	protected:
		//static MatricesStack stack;
		static matrix44 current_mvp;

	public:
		enum ePosition {
			UNDEFINED = 0,
			TOP,
			RIGHT,
			BOTTOM,
			LEFT,
			CENTER,
		} ;

		enum eEvent {
			CLICK_EVENT = 1,
			INTRO_EVENT,
			DRAG_EVENT,
			CLOSE_EVENT
		};


		static vector2f mouse_position;
		static Widget* widget_on_focus;
		static Widget* widget_captured;//widget that receives mouse events although the cursor is not over widget area
		static float	current_alpha;
		static std::list<Widget*> pending_to_destroy;
		static void destroyPendingWidgets();

		std::string name;
		std::string caption;
		Widget*		parent;

		Area		area; //pos and size
		Area		world_area; //area in world coordinates
		ePosition	x_origin; //where is the 0,0
		ePosition	y_origin;

		vector4f    color;
		vector4f    bg_color;
		float		alpha;
		vector4f    bg_color_on_over;
		Texture*	bg_texture;

		BitmapFont*		font;
		unsigned int	font_size;
		vector2f	padding;

		bool enabled;
		bool over;
		bool render_border;
		bool is_clicked;
		bool clickable;
		bool blend;

		Parameter* external_var;

		std::list<Widget*> children;
		typedef std::list<Widget*>::iterator tWidgetIterator;
		std::map<std::string,std::string> data;

		Widget();
		virtual ~Widget();

		Widget* clone();
		virtual void destroy();
		virtual const char* getClassName() { return "widget"; }

		//flags
		bool isEnabled() { return enabled; }
		void setEnabled(bool v) { enabled = v; }
		bool isOnFocus();

		//children
		void setParent(Widget* parent);
		void addChild(Widget* w, bool send_events = true); //set parent, push in children list, call onNewParent and onNewChild
		bool removeChild(Widget* w); //remove child from children list, set parent null, nothing else
		Widget* getChild(unsigned int i);
		std::vector<Widget*> getChildVector();
		unsigned int getNumChild();
		virtual void onNewChild(Widget* new_child) {}
		virtual void onNewParent(Widget* new_parent) {}
		virtual Widget* getWidget(const char* name);
		virtual void bringFront();
		
		//actions
		virtual void render();
		virtual void renderWidget();
		virtual void renderText( const std::string &text, vector2f pos, vector4f color, float scale = 1.0) const;
		virtual void renderArea( const Area& area, vector4f front_color, vector4f back_color, float shadow = 0.0);
		virtual void update(float elapsed_time_in_ms);
		virtual void updateWidget(float elapsed_time_in_ms);

		//font
		BitmapFont* getFont() const; //Search for the first widget with a font defined
		unsigned int getFontSize() const; //Search for the first widget with a font size defined

		virtual Area getWorldArea();
		virtual std::string getFullName();

		//events
		std::string getEventDispatcherName();
		bool dispatchEvent(Event event);
		bool dispatchWidgetEvent(unsigned char widget_event, std::string msg);
		virtual bool onMouseButton( const Event& event );
		virtual bool onMouseMotion( const Event& event ) { return false; }
		virtual bool onKeyboard( const Event& event );
		//virtual bool onEvent(Event& event ) { return false; }
		virtual bool propagateEvent( const Event& event);
		virtual bool executeCommand(const std::string& cmd);

		//XML
		virtual bool configureFromXML(XML::Element* element);
		virtual bool configureFromXMLFile(const char* filename);

		//properties
		vector4f getColor() { return color; }
		void setColor(vector4f c) { color = c; }
		vector4f getBackgroundColor() { return bg_color; }
		void setBackgroundColor(vector4f c) { bg_color = c; }
		void setAlpha(float v) { alpha = v; }
		float getAlpha() { return alpha; }
		float getGlobalAlpha();
		void setBackgroundTexture(Texture* tex) { bg_texture = tex; }
		Texture* getBackgroundTexture() { return bg_texture; }
		void setPosition(float x, float y) { area.left = x; area.top = y; }
		vector2f getPosition() { return vector2f(area.left,area.top); }
		void setSize(float width, float height) { area.width = width; area.height = height; }
		vector2f getSize() { return vector2f(area.width,area.height); }

		template <class T> void setExternalVar(T& var);

	protected:
		virtual Widget* getWidget(const std::vector<std::string>& names, int iteration);
	};

	//Dialog *******************
	class Dialog : public Widget
	{
	public:
		Dialog();
		const char* getClassName() { return "dialog"; }

		void renderWidget();
		bool configureFromXML(XML::Element* element);

		//virtual bool onMouseButton( const Event& event );
		virtual bool onMouseMotion( const Event& event );

	public:
		bool translation[2];//indicates if the dialog can be displaced in x,y dimensions
	};

	class VerticalArrange: public Widget
	{
	public:
		VerticalArrange();
		virtual void onNewChild(Widget* new_child);
		virtual void onNewParent(Widget* new_parent);
	};

	// *************************
	class Button : public Widget
	{
	public:
		Button();
		const char* getClassName() { return "button"; }

		void renderWidget();
	};

	// *************************
	class TextBox : public Widget
	{
	public:
		TextBox();
		const char* getClassName() { return "textbox"; }

		void renderWidget();
		virtual bool onKeyboard(const Event& event);
	};

	// *************************
	class Static : public Widget
	{
	public:
		Static();
		const char* getClassName() { return "static"; }

		void renderWidget();
	};

	// *************************
	class Slider : public Widget
	{
		vector2f slider_pos;

		float bit_width;

		bool horizontal;
		bool vertical;

		Texture* bit_texture;

	public:
		EventSlot actionChangeSlider;

		Slider();
		const char* getClassName() { return "slider"; }

		bool onMouseButton( const Event& event);
		bool onMouseMotion( const Event& event );		
		void renderWidget();

		virtual void setSliderPos(float x, float y = 0);
		//! Returns normalized slider value
		float getSliderValue() const;

		bool onEvent(Event& event);
		virtual bool configureFromXML(XML::Element* element);
	protected:

		vector2f getBitPos();
	};

	// *************************

	class ParametersDialog : public Widget
	{
	public:

	};

	//************************************
	class GUI : public Widget, public MouseEventReceiver
	{
	public:
		static GUI* instance;

		GUI();
		const char* getClassName() { return "GUI"; }

		virtual void render();
		virtual void renderWidget() {}
		virtual void update(float elapsed_time_in_ms);

		bool propagateEvent( const Event& event);
		virtual bool loadFromXML(const char* xml_filename);

		void setAllWidgetsStatus(bool v);
		void setWidgetStatus(const char* name, bool v);

	public:
		static Widget* createWidget(std::string widget_class_name); //factory
		static Widget* createWidgetFromXML(XML::Element* element); //factory

		static bool isGUIVisible() { return instance->getAlpha() > 0.0; }

	}; //GUI

}; //gti

#endif //INC_GUI_H_