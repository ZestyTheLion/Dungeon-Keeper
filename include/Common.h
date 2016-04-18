#pragma once
#include <string>
#include <Windows.h>
#include <Position.h>

class Entity;

enum COLOR
{
	COLOR_RED = 4, COLOR_GREEN = 2,
};

void SetCursorPosition(int x, int y);

void SendServerLiteral(std::string msg);

void CreateMultiplayerWorld(int player_amount, std::string names[]);

namespace Common
{
	extern void CleanGameOverlay();
	extern bool ShootFrom(Position pos, int direction);
	extern int GetBulletDamage(Entity* entity);
	extern int GetBulletDirection(Entity* entity);
}
