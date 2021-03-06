#pragma once
#include <list>
#include <memory>
#include <map>
#include "Position.h"
#include "NoteUI.h"

class Player;

enum COMPONENT
{
	COMPONENT_HEALTH, COMPONENT_HITTABLE
};

enum KEYWORD
{
	KEYWORD_BULLET,
};

class Component
{
public:
	virtual void update() = 0;
private:

};

class Entity
{
public:
	Entity();
	~Entity();

	virtual void update() = 0;

	void setToUpdate();
	void setToNoUpdate();
	virtual void setID(int id);
	void addKeyWord(KEYWORD key);
	
	bool isSetToUpdate();
	virtual bool hasKeyWord(KEYWORD key);

	virtual int getID();
	virtual int& getIDRef();
	virtual NoteUI getNote();

	virtual bool hasComponent(int component) = 0;
	virtual bool isKilled() = 0;
	virtual void kill() = 0;
	virtual void clean() = 0;
	virtual void serialize(std::fstream&) = 0;
	virtual void deserialize(std::fstream&) = 0;
	virtual bool damage(int damage, std::string name, bool server=false) = 0;
	virtual void setPos(Position) = 0;
	virtual void updateOverlay() = 0;
	virtual void updateID() = 0;
	virtual void send() = 0;
	virtual void render() = 0;
	virtual void activate(Player* player) = 0;
	virtual Position getPos() = 0;
private:
	int id_;
	bool kill_;
	bool isObjectHosted_;
	std::list<KEYWORD> keywords_;
};

class System
{
public:
	System();
	~System();

	void update();

	void cleanAndUpdateOverlays();

	bool entityAt(Position pos);
	
	int addEntity(std::shared_ptr<Entity>, bool send = true);

	int addEntityServer(std::shared_ptr<Entity>);

	int addEntity(std::shared_ptr<Entity>, std::string txt, bool send = true);

	bool getEntityAt(Position pos, Entity** entity);

	bool getEntityServer(int id, Entity** entity);

	bool getEntity(int id, Entity** entity);

	bool deleteEntity(int id);

	bool deleteEntityServer(int id);

	void serialize(std::fstream&);

	void sendAll();

	void clear();
private:
	int id_index;
	std::map<int, std::shared_ptr<Entity>> m_system;
	std::map<int, int> m_server;
};

