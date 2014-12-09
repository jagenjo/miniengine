#ifndef CONTROLLER_H
#define CONTROLLER_H

/***
Info: 
The Controller is in charge of controlling the behaviour of an Entity. Once both are linked the controller will be called
everytime the entity needs to recalc.
Is the base class for AIs and User-Controllers.
***/

class Entity;

class Controller
{
public:
	Entity* controlled_entity;

	Controller() : controlled_entity(NULL) {}

	virtual void update(float elapsed) {}

	void setControlledEntity(Entity* entity);
	virtual bool canDelete() { return true; }
	
};

#endif