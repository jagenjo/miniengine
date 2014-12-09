#include "../includes.h"

#include "GUI.h"
#include "Painter.h"
#include "Application.h"
#include "SimpleGLText.h"
#include "Utils.h"
#include "BitmapFont.h"

namespace gti
{

// ------------------------------------------------
Area::Area( float aleft, float atop, float awidth, float aheight ) 
		: left( aleft )
		, top( atop )
		, width( awidth )
		, height( aheight ) 
{
}

	
bool Area::isInside( const vector2f &pos ) const {
  return ( pos.x >= left && pos.x < ( left + width ) && pos.y >= top && pos.y < ( top + height ) );
}

void Area::growToInclude( const Area &area ) {
  if( left > area.left )
    left = area.left;
  if( top > area.top )
    top = area.top;
  if( left + width < area.left + area.width )
    width = area.left + area.width - left;
  if( top + height < area.top + area.height )
    height = area.top + area.height - top;
}

vector2f Area::inLocalCoords( const vector2f &pos ) const {
  return vector2f( ( pos.x - left ) / width,  (pos.y - top ) / height );
}

Area Area::overlap( const Area &area )
{
	Area tmp;
	tmp.left = std::max(area.left,left);
	tmp.top = std::max(area.top,top);
	tmp.width = std::min(area.left + area.width,left + width) - tmp.left;
	tmp.height = std::min(area.top + area.height,top + height) - tmp.top;

	return tmp;
}

//MatricesStack Widget::stack;
vector2f Widget::mouse_position;
Widget* Widget::widget_on_focus = NULL;
Widget* Widget::widget_captured = NULL;
matrix44 Widget::current_mvp;
float	Widget::current_alpha = 1.0;
std::list<Widget*> Widget::pending_to_destroy;

Widget::Widget()
{
	parent = NULL;
	color.set(0.7,0.75,0.75,0.8);
	bg_color.set(0.2,0.2,0.2,0.8);
	alpha = 1.0;
	over = false;
	is_clicked = false;
	clickable = true;
	enabled = true;
	blend = false;

	bg_texture = NULL;
	render_border = false;
	name = "";
	font = NULL;
	font_size = 0;
	padding.set(0,0);

	x_origin = LEFT;
	y_origin = TOP;

	external_var = NULL;
}

Widget::~Widget()
{
	setParent(NULL);

	std::vector<Widget*> v = getChildVector();
	for (int i = 0; i < v.size(); i++)
		v[i]->destroy();
}

void Widget::destroy()
{
	setParent(NULL);
	pending_to_destroy.push_back(this);
}

void Widget::destroyPendingWidgets()
{
	//for (std::list<Widget*>::iterator it = pending_to_destroy.begin(); it != pending_to_destroy.end(); it++)
	std::list<Widget*>::iterator it;
	while( !pending_to_destroy.empty() )
	{
		it = pending_to_destroy.begin();
		delete (*it);
		pending_to_destroy.erase( it );
	}
}

Widget* Widget::clone()
{
	Widget* new_widget = GUI::createWidget(this->getClassName());
	assert(new_widget);
	
	//DANGEROUS!!!!!
	*new_widget = *this;

	new_widget->children.clear();
	new_widget->parent = NULL;
	new_widget->name += "_";

	//replicate children
	for( std::list<Widget*>::iterator it = children.begin(); it != children.end(); it++)
	{
		Widget* c = (*it)->clone();
		new_widget->addChild(c);
	}

	return new_widget;
}

void Widget::setParent(Widget* parent)
{
	if (this->parent == parent)
		return;

	if (this->parent != NULL)
		this->removeChild(this);

	this->parent = parent;
	onNewParent(parent);

	if (parent != NULL)
		parent->addChild(this);
}

void Widget::addChild(Widget* w,bool send_events)
{
	assert(w);
	w->setParent(NULL);
	children.push_back(w);
	w->parent = this;

	if (send_events)
	{
		w->onNewParent(this);
		onNewChild(w);
	}
}

bool Widget::removeChild(Widget* w)
{
	for( std::list<Widget*>::iterator it = children.begin(); it != children.end(); it++)
	{
		if (*it == w)
		{
			children.erase(it);
			w->parent = NULL;
			return true;
		}
	}
	return false;
}


void Widget::bringFront()
{
	if (parent == NULL)
		return;

	Widget* p = parent;
	p->removeChild(this);
	p->addChild(this,false);

	//propagate up in inherarchy
	p->bringFront();
}

Widget* Widget::getChild(unsigned int num)
{
	int i = 0;
	for( std::list<Widget*>::iterator it = children.begin(); it != children.end(); it++)
	{
		if (i == num)
			return *it;
		i++;
	}
	return NULL;
}

std::vector<Widget*> Widget::getChildVector()
{
	std::vector<Widget*> result;
	result.reserve( children.size() );
	for( std::list<Widget*>::reverse_iterator it = children.rbegin(); it != children.rend(); it++)
		result.push_back(*it);
	return result;
}

unsigned int Widget::getNumChild()
{
	return children.size();
}

void Widget::render()
{
	world_area = getWorldArea();

	renderWidget();


	//stack.translate(area.left, area.top, 0.0);

	for( std::list<Widget*>::iterator it = children.begin(); it != children.end(); it++)
	{
		if ((*it)->isEnabled())
			(*it)->render();
	}
}

void Widget::update(float elapsed_time_in_ms)
{
	updateWidget(elapsed_time_in_ms);

	std::vector<Widget*> v = getChildVector();
	for(size_t i = 0; i < v.size(); i++)
		if (v[i]->isEnabled())
			v[i]->update(elapsed_time_in_ms);
}

void Widget::updateWidget(float elapsed_time_in_ms)
{
	if ( world_area.isInside( mouse_position ) )
		over = true;
	else
		over = false;

	//clicked = false;
}

void Widget::renderWidget()
{
	//renderArea(area, bg_color, bg_color);
}

void Widget::renderArea(const Area& area, vector4f front_color, vector4f back_color, float shadow)
{
	//shadow
	if (false && shadow > 0.0)
	{
		float shadow_dist = 5;
		Painter::setFillColor( 0,0,0, back_color.w * shadow);
		//Painter::setFillColor( 1.0, 1.0, 1.0, 0.5 );
		Painter::drawQuad(current_mvp , area.left + shadow_dist, area.top + shadow_dist, area.width, area.height, true );
	}

	Painter::setFillColor( back_color.x, back_color.y, back_color.z, back_color.w);
	//Painter::setFillColor( 1.0, 1.0, 1.0, 0.5 );

	if (blend)
		glBlendFunc( GL_SRC_ALPHA,GL_ONE );
	else
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	if (bg_texture)
		Painter::drawQuad(bg_texture, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight(), area.left, area.top, area.width, area.height, NULL, true );
	else
		Painter::drawQuad(current_mvp , area.left, area.top, area.width, area.height );

	//border
	if (false)
	{
		glDisable( GL_LINE_SMOOTH );
		glDisable( GL_BLEND );
		glHint(GL_LINE_SMOOTH_HINT,GL_FASTEST);
		glHint(GL_POLYGON_SMOOTH_HINT,GL_FASTEST);
		Painter::setLineWidth(1);

		matrix44 rot;
		rot.setRotation(45,vector3f(0,0,1));
		//current_mvp = rot * current_mvp;

		std::vector<vector3f> lines;
		lines.resize(5);
		lines[0].set(area.left,area.top,0.0);
		lines[1].set(area.left + area.width, area.top,0.0);
		lines[2].set(area.left + area.width, area.top + area.height,0.0);
		lines[3].set(area.left, area.top + area.height,0.0);
		lines[4].set(area.left, area.top,0.0);

		Painter::setFillColor( front_color.x, front_color.y, front_color.z, front_color.w);
		Painter::drawLineStrip(current_mvp,&lines[0], lines.size() - 1);
	}

	//glow
	if (render_border && bg_texture == NULL)
	{
		float border = 4;
		Painter::setFillColor( 0,0,0,bg_color.w * 0.25);
		Painter::drawQuad(current_mvp , area.left - border, area.top - border, area.width + border*2, area.height + border*2);
	}
}



BitmapFont* Widget::getFont() const
{
	Widget* aux = (Widget*)this;
	while(aux)
	{
		if (aux->font)
		{
			return aux->font;
			break;
		}
		aux = aux->parent;
	}
	return NULL;
}

unsigned int Widget::getFontSize() const
{
	Widget* aux = (Widget*)this;
	while(aux)
	{
		if (aux->font_size > 0)
		{
			return aux->font_size;
			break;
		}
		aux = aux->parent;
	}
	return 0;
}

bool Widget::isOnFocus()
{ 
	Widget* aux = (Widget*)widget_on_focus;
	while(aux)
	{
		if (this == aux)
			return true;
		aux = aux->parent;
	}
	return false;
}

void Widget::renderText(const std::string &text, vector2f pos, vector4f color, float scale) const
{
	if (text.empty())
		return;

	float width_end = world_area.left + world_area.width;

	pos.x += world_area.left;
	pos.y += world_area.top;

	Painter::setFillColor( color.x, color.y, color.z, color.w);

	BitmapFont* f = getFont();
	unsigned int f_size = getFontSize();
	assert(f_size > 0);

	if (f == NULL)
		printf2D( vector2f(pos.x / (float)Camera::getApplicationWindowWidth(),pos.y / (float)Camera::getApplicationWindowHeight()),"%s", text.c_str() );
	else
	{
		f->setFontStyle( f_size * scale, color, blend );
		//f->renderText( text.c_str(), pos, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight(), width_end - pos.x );
		
		// autocenter
		gti::vector2f rect;
		f->computeRectangle(text,rect.x,rect.y);
		pos.x = world_area.left + (world_area.width-rect.x)/2;
		pos.y = world_area.top + (world_area.height-rect.y)/2;
		pos.y -= f->getLineHeight();
		f->renderText( text.c_str(), pos, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight(), width_end - pos.x );
	}
}

bool Widget::onMouseButton( const Event& event )
{
	if (!clickable)
		return false;

	if (event.button.button == MOUSE_LEFT)
	{
		if (event.button.state == MOUSE_DOWN)
		{
			is_clicked = true;
			//std::cout << "Clicked!: " << getFullName() << std::endl;
			dispatchEvent(Event("click"));
			bringFront();
		}
		else if (event.button.state == MOUSE_UP)
		{
			is_clicked = false;
			//std::cout << "mouse up!: " << name << std::endl;
		}
	}
	return true;
}

bool Widget::onKeyboard( const Event& event )
{
	return false;
}


bool Widget::propagateEvent( const Event& event)
{
	//first we check the children (because they are on-top)
	std::vector<Widget*> v = getChildVector();
	for (size_t i = 0; i < v.size(); i++)
		if ( v[i]->world_area.isInside( mouse_position ) )
			if (v[i]->propagateEvent(event))
				return true;

	//outside of me?
	if (!world_area.isInside( mouse_position ) )
		return false;

	//process
	switch (event.type)
	{
		case Event::MOUSE_BUTTON:
			if (onMouseButton(event))
			{
				if (event.button.state == MOUSE_DOWN)
					widget_on_focus = this;
				return true;	//if the click was computed inside then dont do more things
			}
			break;
		case Event::MOUSE_MOTION:
			if (onMouseMotion(event))
			{
				return true;	//if the click was computed inside then dont do more things
			}
			break;
		default: break;
	}	

	return false;
}

std::string Widget::getEventDispatcherName() { return getFullName(); }

Area Widget::getWorldArea()
{
	if (parent)
	{
		Area a = parent->getWorldArea();

		a.top += area.top;
		if (y_origin == BOTTOM)
			a.top += parent->area.height;
		else if (y_origin == CENTER)
			a.top += parent->area.height * 0.5;

		a.left += area.left;
		if (x_origin == RIGHT)
			a.left += parent->area.width;
		else if (x_origin == CENTER)
			a.left += parent->area.width * 0.5;
		a.width = area.width;
		a.height = area.height;
		return a;
	}
	return area;
}

float Widget::getGlobalAlpha()
{
	if (parent)
		return parent->getGlobalAlpha() * getAlpha();
	return getAlpha();
}

std::string Widget::getFullName()
{
	if (parent)
		return parent->getFullName() + "/" + name;
	return name;
}

template <class T> void Widget::setExternalVar(T& var)
{
	if (external_var == NULL)
		external_var = new Parameter();
	external_var->set(var);
}

Widget* Widget::getWidget(const char* name)
{
	std::vector<std::string> tokens = tokenize(name,"/");
	return getWidget(tokens,0);
}

Widget* Widget::getWidget(const std::vector<std::string>& names, int iteration)
{
	assert(names.size() > iteration);
	Widget* w = NULL;

	for (tWidgetIterator it = children.begin(); it != children.end(); it++)
	{
		//this son doesnt have a name? then enter
		if ( (*it)->name == "")
		{
			w = (*it)->getWidget(names,iteration); //recursive
			if (w !=  NULL)
				return w;
			continue;
		}
		
		//this son has the same we need?
		if ( (*it)->name == names[iteration] )
		{
			if (iteration == names.size() - 1) //the last one?
				return (*it); //we found it
			
			//we need to search inside
			w = (*it)->getWidget(names,iteration + 1); //recursive
			if (w != NULL)
				return w;
		}
	}

	return NULL;
}

bool Widget::dispatchEvent(Event event)
{
	event.timestamp = Application::application->app_time;
	event.origin = this;
	if (getConnection() != NULL)
		return getConnection()->onEvent(event);
	else
		GUI::instance->onEvent(event);
	return false;
}

bool Widget::dispatchWidgetEvent(unsigned char widget_event, std::string msg)
{
	Event e;
	e.type = Event::WIDGET;
	e.widget.widget_event = widget_event;
	e.text = msg;
	return dispatchEvent(e);
}

bool Widget::configureFromXML(XML::Element* element)
{
	assert(element);
	std::string str;

	element->getAttribute("x",area.left,area.left);
	element->getAttribute("y",area.top,area.top);
	element->getAttribute("width",area.width,area.width);
	element->getAttribute("height",area.height,area.height);
	element->getAttribute("caption",caption,caption.c_str());
	element->getAttribute("name",name,name.c_str());
	element->getAttribute("blend",blend,blend);
	element->getAttribute("enabled",enabled,enabled);

	if (element->getAttribute("font_file",str))
		this->font = BitmapFont::getFont(str.c_str());
	element->getAttribute("font_size",font_size,font_size);
	
	element->getAttribute("padding",str);
	padding.parseFromText(str.c_str());

	if (element->getAttribute("origin",str))
	{
		size_t colon_pos = str.find_first_of(',');
		std::string y = str.substr(0,colon_pos);
		std::string x = str.substr(colon_pos+1, std::string::npos);
		if (x == "center") x_origin = CENTER;
		else if (x == "right") x_origin = RIGHT;
		else x_origin = LEFT;
		if (y == "center") y_origin = CENTER;
		else if (y == "bottom") y_origin = BOTTOM;
		else y_origin = TOP;
	}

	if (element->getAttribute("bg_color",str))
		bg_color.parseFromText(str.c_str());
	if (element->getAttribute("color",str))
		color.parseFromText(str.c_str());

	if (element->getAttribute("bg_texture",str))
		bg_texture = gti::getTextureManager()->add(str);

	for (size_t i = 0; i < element->getNumberOfChildren(); i++)
	{
		XML::Element* aux = element->getChild(i);

		if (aux->id == "data")
		{
			std::string name, value;
			if (aux->getAttribute("name",name) == false)
			{
				gti::error("XML Data tag without name attribute");
				continue;
			}
			if (aux->getAttribute("value",value) == false)
			{
				gti::error("XML Data tag without value attribute");
				continue;
			}

			data[name]=value;
			continue;
		}
		
		//inner widget
		Widget* new_widget = GUI::createWidgetFromXML(aux);
		if (new_widget == NULL)
			continue;

		//it is configured from inside
		//new_widget->configureFromXML(aux);

		addChild(new_widget);
	}
	return true;
}

bool Widget::configureFromXMLFile(const char* filename)
{
	XML::Document doc;
	if (!doc.readFile(filename))
	{
		gti::error("Error: XML not found: %s\n", filename); 
		return false;
	}

	//iterate all elements
	return configureFromXML(doc.root);
}

bool Widget::executeCommand(const std::string& cmd)
{
	std::vector<std::string> tokens;
	splitCommands(cmd.c_str(),tokens);

	if (tokens[0] == "hide_all")
		for (tWidgetIterator it = children.begin(); it != children.end(); it++)
			(*it)->setEnabled(false);
	else if (tokens[0] == "show_all")
		for (tWidgetIterator it = children.begin(); it != children.end(); it++)
			(*it)->setEnabled(true);
	else if (tokens[0] == "show")
		setEnabled(true);
	else if (tokens[0] == "hide")
		setEnabled(false);
	else if (tokens[0] == "set" && tokens.size() > 2 )
	{
		if (tokens[1] == "bg_texture" )
			bg_texture = gti::getTextureManager()->add(tokens[2]);
		else if (tokens[1] == "color" )
			color.parseFromText( tokens[2].c_str() );
		else if (tokens[1] == "bg_color" )
			bg_color.parseFromText( tokens[2].c_str() );
		else if (tokens[1] == "alpha" )
			bg_color.parseFromText( tokens[2].c_str() );
		else if (tokens[1] == "caption" )
			caption = tokens[2].substr(1, tokens[2].size() - 2);
	}
	else
		return false;
	return true;
}

// DIALOG **************************
Dialog::Dialog()
{
	render_border = true;
	padding.set(10,10);

	translation[0] = true;
	translation[1] = true;
}

void Dialog::renderWidget()
{
	//bg
	renderArea(world_area, color, bg_color + (isOnFocus() ? gti::vector4f(0.1,0.1,0.1,0.0) : gti::vector4f(0.0,0.0,0.0,0.0)), 0.1);

	//title
	unsigned int f_size = getFontSize();
	Painter::setFillColor( color.x, color.y, color.z, (over ? color.w * 0.5 + 0.1 : color.w * 0.5));
	if (bg_texture == NULL)
		Painter::drawQuad( current_mvp, world_area.left, world_area.top, area.width, f_size * 2, true );

	renderText(caption.c_str(), vector2f(padding.x,padding.y), color, 2.0);
}

bool Dialog::configureFromXML(XML::Element* element)
{
	Widget::configureFromXML(element);

	translation[0] = element->getBool("tx",true);
	translation[1] = element->getBool("ty",true);
	return true;
}

bool Dialog::onMouseMotion( const Event& event )
{
	if (is_clicked)
	{
		//std::cout << Application::frame << " " << event.motion.dx << std::endl;
		if (translation[0])
		{
			area.left += event.motion.dx;
		}
		if (translation[1])
		{
			area.top += event.motion.dy;
		}
	}
	return true;
}

VerticalArrange::VerticalArrange()
{
	clickable = false;
}

void VerticalArrange::onNewChild(Widget* new_child)
{
	float padding = 4;
	float current_top = 25;

	for (int i = 0; i < getNumChild(); i++)
	{
		Widget* w = getChild(i);
		w->area.left = padding;
		w->area.width = area.width - padding * 2;
		w->area.top = current_top;
		current_top += padding + w->area.height;
	}

	area.height = current_top;
}

void VerticalArrange::onNewParent(Widget* new_parent)
{
	if (new_parent == NULL)
		return;

	area.width = new_parent->area.width;
	area.height = new_parent->area.height;

	onNewChild(NULL);

	new_parent->area.height = area.height;
}

// BUTTON **************************
Static::Static()
{
	clickable = false;
}

void Static::renderWidget()
{
	if (caption.empty() && bg_texture)
		renderArea(world_area, color,bg_color);
	
	renderText(caption.c_str(),vector2f(0, 0), color);
}

//******************

Button::Button()
{
	area.width = 200;
	area.height = 20;
}

void Button::renderWidget()
{
	vector4f c1 = bg_color;//( is_clicked ? bg_color : color );
	vector4f c2 = color;//( is_clicked ? color : color );
	
	//renderArea(world_area, c1, c2);
	renderArea(world_area, c1, c1);

	float padding = 20;
	Painter::setFillColor( c1.x, c1.y, c1.z, (over ? c1.w * 0.5 + 0.2 : c1.w * 0.25) );
	Painter::drawQuad( current_mvp, world_area.left + 3, world_area.top + 3, world_area.width - 6, world_area.height - 6);

	//renderText(caption.c_str(),vector2f( (area.width + padding )* 0.5 - caption.size() * 4, area.height * 0.5 + 4), c2);

	renderText(caption.c_str(),vector2f( (area.width + padding )* 0.5 - caption.size() * 4, area.height * 0.5 + 4), c2);

	/*
	BitmapFont* f = getFont();
	f->setFontStyle( getFontSize(), c2, blend );
	gti::vector2f pos;	
	f->computeRectangle(caption,pos.x,pos.y);
	pos.x += world_area.left;
	pos.y += world_area.top;
	f->renderText( caption.c_str(), pos, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight());
	*/
}

//************************

TextBox::TextBox()
{
}

void TextBox::renderWidget()
{
	bool onfocus = isOnFocus();
	renderArea(world_area, color, bg_color);

	std::string t = caption;
	if (onfocus && ( (int)(Application::application->app_time * 5) % 2))
		t += "|";
	
	renderText(t.c_str(), vector2f( 5, area.height * 0.5 - getFontSize() * 0.5 ), color);
}

bool TextBox::onKeyboard(const Event& event)
{
	if (event.key.state == KEY_DOWN)
		switch(event.key.keycode)
		{
			case gti::GTIK_BACKSPACE: 
				if ( !caption.empty() )
					caption.resize( caption.size() - 1 );
				break;
			case gti::GTIK_RETURN: 
				dispatchWidgetEvent(Widget::INTRO_EVENT, caption );
				caption = "";
				break;
			default: 
				if (event.key.keycode >= 32) //first printable character
				{
					caption += event.key.unicode;
					return true;
				}
				else
					return false;
				break;
		}
	return true;
}

//*************************

Slider::Slider() : actionChangeSlider("change_slider")
{
	actionChangeSlider.connect(this);

	area.height = 40;
	bit_width = 20;
	horizontal = true;
	vertical = false;
	slider_pos.set(0.5,0.5);
	bit_texture = NULL;
}

void Slider::renderWidget()
{
	vector4f color = this->color;
	color.w *= 0.5;
	renderArea(world_area, color, bg_color);

	renderText(caption.c_str(),vector2f( (world_area.width )* 0.5 - caption.size() * 4, world_area.height * 0.5 + 4), color);
	Painter::setFillColor( color.x, color.y, color.z, color.w);

	if (horizontal && !vertical)
		Painter::drawQuad(current_mvp, getBitPos().x - bit_width * 0.5, world_area.top + 3, bit_width, world_area.height - 6);
	else if (!horizontal && vertical)
		Painter::drawQuad( current_mvp, getBitPos().x - bit_width * 0.5, world_area.top + 3, bit_width, world_area.height - 6);
	else if (horizontal && vertical)
	{
		if (bit_texture)
			Painter::drawQuad(bit_texture, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight(), getBitPos().x - bit_width * 0.5, getBitPos().y - bit_width * 0.5, bit_width, bit_width );
		else
			Painter::drawQuad( current_mvp, getBitPos().x - bit_width * 0.5, getBitPos().y - bit_width * 0.5, bit_width, bit_width);
	}
	else
		assert(0); //impossible 
}

vector2f Slider::getBitPos()
{
	vector2f v;
	v.x = getWorldArea().left + 3 + (area.width - bit_width - 6) * slider_pos.x + bit_width * 0.5;
	v.y = getWorldArea().top + 3 + (area.height - bit_width - 6) * slider_pos.y + bit_width * 0.5;
	return v;
}

bool Slider::configureFromXML(XML::Element* element)
{
	element->getAttribute("vertical",vertical,vertical);
	element->getAttribute("horizontal",horizontal,horizontal);

	std::string bit_texture_name;
	if (element->getAttribute("texture",bit_texture_name))
		bit_texture = gti::getTextureManager()->add(bit_texture_name);

	return Widget::configureFromXML(element);
}

void Slider::setSliderPos(float x, float y)
{
	slider_pos.x = x;
	slider_pos.y = y;

	if (slider_pos.x > 1.0) slider_pos.x = 1;
	if (slider_pos.x < 0.0) slider_pos.x = 0;
	if (slider_pos.y > 1.0) slider_pos.y = 1;
	if (slider_pos.y < 0.0) slider_pos.y = 0;

	dispatchEvent(Event("value_changed", vector4f(slider_pos.x, slider_pos.y,0,0) ) );
}

float Slider::getSliderValue() const
{
	if (horizontal) return slider_pos.x;
	return slider_pos.y;
}

bool Slider::onMouseButton( const Event& event)
{
	if (event.button.button == MOUSE_LEFT)
	{
		if (event.button.state == MOUSE_DOWN)
		{
			vector2f pos = slider_pos;

			if (horizontal)
				pos.x += (event.button.x - getBitPos().x) / area.width;

			if (vertical)
				pos.y += (event.button.y - getBitPos().y) / area.width;

			setSliderPos(pos.x,pos.y);

			widget_captured = this;

			return Widget::onMouseButton(event);
		}
		else
		{
			widget_captured = 0;
		}
	}

	return Widget::onMouseButton(event);
}

bool Slider::onEvent(Event& event)
{
	if (event.type == Event::DATA && event.text == "change_value")
	{
		setSliderPos(event.data.data[0],event.data.data[1]);
		return true;
	}
	return Widget::onEvent(event);
}

bool Slider::onMouseMotion( const Event& event )
{
	if (is_clicked)
	{
		vector2f pos = slider_pos;

		if (horizontal)
			pos.x += (event.motion.x - getBitPos().x) / area.width;
		if (vertical)
			pos.y += (event.motion.y - getBitPos().y) / area.width;

		setSliderPos(pos.x,pos.y);
	}
	return true;
}

//********************************************
GUI* GUI::instance = NULL;

GUI::GUI()
{
	area.set(0,0,Camera::getApplicationWindowWidth(),Camera::getApplicationWindowHeight() );
	name = "GUI";
	clickable = false;
	instance = this;
	font_size = 18;
}

void GUI::render()
{
	if ( getAlpha() == 0.0 )
		return;

	current_alpha = getAlpha();

	//glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	//render all
	current_mvp.setOrthoProjection(0, Camera::getApplicationWindowWidth(), Camera::getApplicationWindowHeight(), 0,-1,1);

	Widget::render();

	glDisable( GL_BLEND );
}

void GUI::update(float elapsed_time_in_ms)
{
	instance = this;
	assert( area.width != 0 && area.height != 0 );

	if (getAlpha() == 0.0)
		return;

	mouse_position.set(Application::cursor_x,Application::cursor_y);

	Camera cam2D;
	cam2D.setCameraOrthogonal(0,Application::application->getWidth(),Application::application->getHeight(),0,-1,1);
	cam2D.set();

	Widget::update(elapsed_time_in_ms);

	destroyPendingWidgets();
}

bool GUI::propagateEvent( const Event& event)
{
	instance = this;

	bool result;
	//>> fix to simulate SetCapture/ReleaseCapture for slider mouse-based events
	//result = Widget::propagateEvent(event);
	if (widget_captured!=0)
	{
		if (event.type == Event::MOUSE_BUTTON)
		{
			result = widget_captured->onMouseButton(event);
		}
		else if (event.type == Event::MOUSE_MOTION)
		{
			result = widget_captured->onMouseMotion(event);
		}
		else
		{
			widget_captured = 0;
			result = Widget::propagateEvent(event);
		}
	}
	else
	{
		result = Widget::propagateEvent(event);
	}
	//<<

	if (widget_on_focus)
	{
		if( event.type == Event::MOUSE_BUTTON && event.button.state == MOUSE_UP)
		{
			result |= widget_on_focus->onMouseButton(event);
		}
		else if( event.type == Event::KEYBOARD)
		{
			result |= widget_on_focus->onKeyboard(event);
		}
	}

	return result;
}

bool GUI::loadFromXML(const char *xml_filename)
{

	XML::Document doc;
	if (!doc.readFile(xml_filename))
	{
		gti::error("Error: XML not found: %s\n", xml_filename); 
		return false;
	}

	//iterate all elements
	XML::Element* root_element = doc.root;
	configureFromXML(root_element);

	//this is a good momento to update the size
	area.set(0,0,Camera::getApplicationWindowWidth(),Camera::getApplicationWindowHeight() );
	return true;
}

void GUI::setAllWidgetsStatus(bool v)
{
	for (size_t i = 0; i < children.size(); i++)
		getChild(i)->setEnabled(v);
}

Widget* GUI::createWidget(std::string widget_class_name)
{
	Widget* w = NULL;
	if (widget_class_name == "dialog")
		w = new Dialog();
	else if (widget_class_name == "button")
		w = new Button();
	else if (widget_class_name == "slider")
		w = new Slider();
	else if (widget_class_name == "static")
		w = new Static();
	else if (widget_class_name == "textbox")
		w = new TextBox();
	else if (widget_class_name == "varrange")
		w = new VerticalArrange();
	else if (widget_class_name == "widget")
		w = new Widget();
	return w;
}


Widget* GUI::createWidgetFromXML(XML::Element* element)
{
	assert(element);

	Widget* w = createWidget(element->id);
	
	if (w == NULL)
	{
		gti::warning("Warning: Widget name unknown: %s\n",element->id.c_str());
		return NULL;
	}

	w->configureFromXML(element);
	return w;
}



}
