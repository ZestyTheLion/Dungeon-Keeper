#include <fstream>
#include <sstream>
#include "Core.h"
#include "LoadEnums.h"
#include "Player.h"
#include "Position.h"
#include "Display.h"
#include "SoundManager.h"
#include "TileEnums.h"
#include "Common.h"

namespace game
{
	extern Display game;
	extern UserInterface SlideUI;
	extern UserInterface tileUI;
	extern System system;
	extern SoundManager m_sounds;
}

enum PLAYER_MODE
{
	MODE_MINING, MODE_PLACING, MODE_SHOOTING,
};

Player::Player() : UI(23, 5, 50, 30, 1)
{
    goldAmount_ = 100;
	maxGoldAmount_ = 10000;
    manaAmount_ = 500;
    maxManaAmount_ = 500;
	ammo_ = 0;
	handPos.setX(0);
	handPos.setY(0);
    name_= "None";
	moved_ = true;
	mined_ = false;
	isSoundPlayingM_ = false;

	turret_sound.SetSound("TurretPlayerHit");

	handMode_ = 0;

	UI.push_back("Mining", false, true);
	UI.push_back("Health:", false, true);
	UI.push_back("Gold:", false, true);
	UI.getSectionRef(2).push_backVar(" ");
	UI.getSectionRef(3).push_backVar(" ");
	UI.isHidden(true);
	mineUIPos.setX(0);
	mineUIPos.setY(0);
	spawnPos(0, 0);
}

Player::Player(Player & p)
{
	*this = p;
}

Player::~Player()
{
    //dtor
}

void Player::stopSounds()
{
	game::m_sounds.StopSound("Mining");
	game::m_sounds.StopSound("TurretPlayerHit");
}


/* Getters */
////////////////////////////////////////////////
int Player::getGoldAmount()
{
    return goldAmount_;
}

int Player::getManaAmount()
{
    return manaAmount_;
}

int Player::getMaxGoldAmount()
{
    return maxGoldAmount_;
}

int Player::getMaxManaAmount()
{
    return maxManaAmount_;
}

int Player::getAmmoAmount()
{
	return ammo_;
}

int Player::getHealth()
{
	return health.getHealth();
}

int Player::getMaxHealth()
{
	return health.getMaxHealth();
}

int Player::getLevel()
{
	return stats.getLevel();
}

int Player::getMaxLevel()
{
	return stats.getMaxLevel();
}

string Player::getName()
{
    return name_;
}

Position Player::getSpawnPos()
{
	return spawnPos;
}
UserInterface & Player::getUIRef()
{
	return UI;
}
HealthComponent & Player::getHealthRef()
{
	return health;
}
////////////////////////////////////////////////


/* Setters */
////////////////////////////////////////////////
void Player::setManaAmount(int amount)
{
    manaAmount_=amount;
}

void Player::setMaxManaAmount(int amount)
{
    maxManaAmount_=amount;
    if(manaAmount_>maxManaAmount_)
    {
        manaAmount_=maxManaAmount_;
    }
}

void Player::setAmmoAmount(int amount)
{
	ammo_ = amount;
}

void Player::setName(string name)
{
	game::game.claimNameChange(name_, name);
	name_ = name;
}

void Player::setSpawnPos(Position pos)
{
	spawnPos = pos;
}

void Player::setHandPos(Position pos)
{
	handPos = pos;
}

void Player::setHandPosNoUpdate(Position pos)
{
	handPos = pos;
}

bool Player::damage(int amount, string name, bool server)
{
	health.damage(amount);
	std::stringstream slide;
	slide << name_ << " -" << amount << " Health";
	game::SlideUI.addSlide(slide.str());
	if (health.isDead() == true)
	{
		game::SlideUI.addSlide(name_ + " Died");
		health.reset();
		forceHandPosition(spawnPos, game::game);
		std::stringstream msg;
		msg << SendDefault << EndLine
			<< UpdatePlayerPosition << std::endl
			<< name_ << std::endl
			<< handPos.getX() << std::endl
			<< handPos.getY() << std::endl;
		SendServerLiteral(msg.str());
	}

	stringstream msg;
	msg << SendDefault << EndLine
		<< DamagePlayer << EndLine
		<< name_ << EndLine
		<< amount << EndLine;
	SendServerLiteral(msg.str());
	return true;
}

void Player::damageS(int amount)
{
	health.damage(amount);
	std::stringstream slide;
	slide << " -" << amount << " Health";
	game::SlideUI.addSlide(slide.str());
	if (health.isDead() == true)
	{
		game::SlideUI.addSlide(name_ + " Died");
		health.reset();
		//forceHandPosition(spawnPos, game::game);
	}
}

void Player::heal(int amount)
{
	health.heal(amount);
	std::stringstream healthSlide;
	healthSlide << name_ << " +" << amount << " Health";
	game::SlideUI.addSlide(healthSlide.str());

	stringstream msg;
	msg << SendDefault << EndLine
		<< HealPlayer << EndLine
		<< name_ << EndLine
		<< amount << EndLine;
	SendServerLiteral(msg.str());
}
void Player::healS(int amount)
{
	health.heal(amount);
	std::stringstream healthSlide;
	healthSlide << name_ << " +" << amount << " Health";
	game::SlideUI.addSlide(healthSlide.str());
}
////////////////////////////////////////////////


/* Hand*/
////////////////////////////////////////////////
void Player::moveHand(DIRECTION direction)
{
	if (movementTimer_.Update() == false) return;
	Position newPos = handPos;
	newPos.go(direction);
	/* Collision Checking */
	///////////////////////////////////
	if (game::game.isValidPosition(newPos, true) == false)
		return;
	Entity* entity;
	if (game::system.getEntityAt(newPos, &entity))
	{
		if (entity->hasKeyWord(KEYWORD_BULLET))
		{
			damage(Common::GetBulletDamage(entity));
			forceHandPosition(newPos);
			knockbackTo((DIRECTION)Common::GetBulletDirection(entity), 1);
			entity->kill();
			game::m_sounds.PlaySoundR("Bullet");
			std::stringstream msg;
			msg << SendDefault << EndLine << Sound << EndLine << "Bullet" << EndLine;
		}
		return;
	}
	///////////////////////////////////
	game::game.removeSelectedAtTile(handPos);
	game::game.setTileAsSelected(newPos);
	handPos = newPos;
	moved_ = true;
	mined_ = false;
	std::stringstream msg;
	msg << SendDefault << EndLine
		<< UpdatePlayerPosition << EndLine
		<< name_ << EndLine
		<< newPos.getX() << EndLine
		<< newPos.getY() << EndLine;
	SendServerLiteral(msg.str());
	return;
}

void Player::moveHandUp(Display& game)
{
	moveHand(DIRECTION_UP);
	return;
}

void Player::moveHandDown(Display& game)
{
	moveHand(DIRECTION_DOWN);
	return;
}

void Player::moveHandLeft(Display& game)
{
	moveHand(DIRECTION_LEFT);
	return;
}

void Player::moveHandRight(Display& game)
{
	moveHand(DIRECTION_RIGHT);
	return;
}

void Player::mineUp(Display& game)
{
	mine(DIRECTION_UP);
}

void Player::mineDown(Display& game)
{
	mine(DIRECTION_DOWN);
}

void Player::mineLeft(Display& game)
{
	mine(DIRECTION_LEFT);
}

void Player::mineRight(Display& game)
{
	mine(DIRECTION_RIGHT);
}

void Player::mine(DIRECTION direction)
{
	if (handMode_ == MODE_MINING)
	{
		Position newPos = handPos;
		newPos.go(direction);
		if (game::system.entityAt(newPos))
		{
			Entity* entity;
			if (game::system.getEntityAt(newPos, &entity))
			{
				double tDamage = 25.0 + (25.0 / (5.0 / stats.getStrength()));
				if (entity->damage((int)tDamage, name_))
				{
					turret_sound.SetTimer(0.200);
				}
			}
		}
		else
		{
			if (game::game.isValidPosition(newPos) == false)
				return;
			if (game::game.getTileRefAt(newPos).isWall() == true)
			{
				double tDamage = 25.0 + (25.0 / (5.0 / stats.getStrength()));
				if (game::game.getTileRefAt(newPos).mine((int)tDamage, *this))
				{
					if (stats.addExp(10))
					{
						game::m_sounds.PlaySoundR("LevelUp");
						health.setMaxHealth(health.getMaxHealth() + 10);
						std::stringstream lvlmsg;
						lvlmsg << "Level Up! << " << stats.getLevel();
						game::SlideUI.addSlide(lvlmsg.str());
					}
				}
			}
			else
				return;
			mineUIPos = newPos;
			mined_ = true;
			moved_ = false;
			if (game::game.getTileRefAt(newPos).isWall() == true)
			{
				if (isSoundPlayingM_ == false)
				{
					game::m_sounds.ResetSound("Mining");
					game::m_sounds.PlaySound("Mining");
					isSoundPlayingM_ = true;
					miningStopSound.StartNewTimer(0.200);
				}
			}
			else
			{
				isSoundPlayingM_ = false;
			}
		}
	}
	else if(handMode_ == MODE_PLACING)
	{
		if (moved_ == true)
		{
			moved_ = false;
			mineProgress_ = 0;
		}
		else
		{
			mineProgress_ += 25;
			if (mineProgress_ >= 100)
			{
				Position newPos = handPos;
				newPos.go(direction);
				if (game::system.entityAt(newPos) == false)
				{
					Tile& tile = game::game.getTileRefAt(newPos);
					if (tile.isWall() == false)
					{
						tile.setGraphic(TG_Stone);
						tile.isWall(true);
						tile.setColor(TGC_Stone);
						tile.setBackground(TGB_Stone);
						tile.isWalkable(false);
						tile.isDestructable(true);
						game::m_sounds.ResetSound("Place");
						game::m_sounds.PlaySound("Place");
						tile.updateServer();
					}
				}
				mineProgress_ = 0;
			}
		}
	}
	else if (handMode_ = MODE_SHOOTING)
	{
		if (ammo_ > 0)
		{
			if (shootTimer_.Update() == true)
			{
				if (Common::ShootFrom(handPos, direction, 50))
				{
					shootTimer_.StartNewTimer(0.35);
					game::m_sounds.PlaySoundR("TurretShoot");
					ammo_--;
				}
			}
			
		}
		else
		{
			game::m_sounds.PlaySoundR("NoAmmo");
		}
	}
}

void Player::attack(DIRECTION direction)
{

}

void Player::forceHandPosition(Position newPos, Display& game)
{
	game.removeSelectedAtTile(handPos);
	handPos = newPos;
	game.setTileAsSelected(newPos);
}

void Player::forceHandPosition(Position newPos)
{
	game::game.removeSelectedAtTile(handPos);
	handPos = newPos;
	game::game.setTileAsSelected(newPos);

}

void Player::claimOnHand()
{
	if (game::game.isClaimedTileNear(handPos, name_))
	{
		game::game.getTileRefAt(handPos).claim(10, name_);
	}
}

void Player::switchMode()
{
	if (handMode_ == 0)
	{
		handMode_ = 1;
		game::tileUI.getSectionRef(6).setVar(1, "Placing");
	}
	else if(handMode_ == 1)
	{
		handMode_ = 2;
		game::tileUI.getSectionRef(6).setVar(1, "Weapon");
	}
	else
	{
		handMode_ = 0;
		game::tileUI.getSectionRef(6).setVar(1, "Mining");
	}
}

void Player::switchModeTo(int mode)
{
	handMode_ = mode;
	switch (mode)
	{
	case 0:game::tileUI.getSectionRef(6).setVar(1, "Mining"); break;
	case 1:game::tileUI.getSectionRef(6).setVar(1, "Placing"); break;
	case 2:game::tileUI.getSectionRef(6).setVar(1, "Weapon"); break;
	}
}

void Player::updateHandPos()
{
	game::game.setTileAsSelectedS(handPos);
}

void Player::disableMovementFor(int time)
{
	movementTimer_.StartNewTimer(time);
}

void Player::knockbackTo(DIRECTION direction, int amount)
{
	Position pPos = handPos; // Previous position
	Position cPos = handPos;
	for (int x = 0; x < amount; x++)
	{
		pPos = cPos;
		cPos.go(direction);
		if (game::game.getTileRefAt(cPos).isWalkable() == false)
		{
			if (x > 0)
			{
				forceHandPosition(pPos);
				return;
			}
			return;
		}
		if (game::system.entityAt(cPos) == true)
		{
			if (x > 0)
			{
				forceHandPosition(pPos);
				return;
			}
			return;
		}
	}
	forceHandPosition(cPos);
	updatePosition();
}

Position Player::getHandPosition()
{
	return handPos;
}
////////////////////////////////////////////////



void Player::placeObject(uint16_t objectID)
{

}

void Player::spawnTurret(Position pos)
{
	shared_ptr<Turret> turret = make_shared<Turret>();
	turret->setPosition(pos);
	turret->setGraphic('+');
	turret->setRange(5);
	turret->setOwner(name_);
	turret->setShootCoolDown(1);
	game::system.addEntity(turret, "Turret");
}

void Player::purchaseTurret()
{
	if (goldAmount_ >= 1000)
	{
		if (game::system.entityAt(handPos) == false)
		{
			shared_ptr<Turret> turret = make_shared<Turret>();
			turret->setPosition(handPos);
			turret->setGraphic('+');
			turret->setRange(8);
			turret->setOwner(name_);
			game::system.addEntity(turret, "Turret");
			Common::SendTurret(turret.get());
			goldAmount_ -= 1000;
			game::m_sounds.PlaySoundR("Money");
		}
		else
		{
			game::SlideUI.addSlide("Turret Place Failed");
		}
	}
}

void Player::purchaseBullet()
{
	if (goldAmount_ >= 100)
	{
		ammo_++;
		goldAmount_ -= 100;
		game::m_sounds.PlaySoundR("Money");
	}
}

void Player::updatePosition()
{
	std::stringstream msg;
	msg << SendDefault << EndLine << UpdatePlayerPosition << EndLine << name_ << EndLine << handPos.serializeR();
	SendServerLiteral(msg.str());
}

void Player::reset()
{
	*this = Player();
}

void Player::updateMiningUI()
{
	if (mined_ == true && moved_ == false)
	{
		stringstream health;
		if (UI.isHidden())
		{
			UI.isHidden(false);
		}
		Tile& tile = game::game.getTileRefAt(mineUIPos);

		health << tile.getHealth() << "/" << tile.getMaxHealth();
		UI.getSectionRef(2).setVar(1, health.str());


		if (tile.hasGold())
		{
			stringstream gold;
			gold << tile.getGold();
			UI.getSectionRef(3).isHidden(false);
			UI.getSectionRef(3).setVar(1, gold.str());
		}
		else
			UI.getSectionRef(3).isHidden(true);
	}
	else
	{
		if (UI.isHidden() == false)
		{
			UI.isHidden(true);
		}
	}
	UI.update();
}

void Player::operator=(Player & player)
{
	goldAmount_ = player.goldAmount_;
	maxGoldAmount_ = player.maxGoldAmount_;
	manaAmount_ = player.manaAmount_;
	maxManaAmount_ = player.maxManaAmount_;
	health.setHealth(player.health.getHealth());
	health.setHealthRegen(player.health.getHealthRegen());
	health.setMaxHealth(player.health.getMaxHealth());
	health.isRegenEnabled(player.health.isRegenEnabled());
	moved_ = player.moved_;
	mined_ = player.mined_;
	isDead_ = player.isDead_;
	isLocal_ = player.isLocal_;
	handPos = player.handPos;
	mineUIPos = player.mineUIPos;
	spawnPos = player.spawnPos;
	name_ = player.name_;
}



/* Gold Related*/
////////////////////////////////////////////////
void Player::addGold(int amount)
{
    goldAmount_+=amount;
	stringstream goldAdd;
	goldAdd << name_ << ":+" << amount << " Gold";
	game::SlideUI.addSlide(goldAdd.str());
    if(goldAmount_>maxGoldAmount_)
    {
        goldAmount_=maxGoldAmount_;
		game::SlideUI.addSlide("Max Gold");
    }
}

void Player::removeGold(int amount)
{
    goldAmount_-=amount;
    if(goldAmount_<0)
        goldAmount_=0;
}

void Player::setGoldAmount(int amount)
{
	goldAmount_ = amount;
}

void Player::setMaxGoldAmount(int amount)
{
	maxGoldAmount_ = amount;
	if (goldAmount_>maxGoldAmount_)
	{
		goldAmount_ = maxGoldAmount_;
	}
}
////////////////////////////////////////////////

/* Serialize - Deserialize */
////////////////////////////////////////////////
void Player::serialize(fstream& file)
{
	file << LOAD::L_Player << endl;
	file << goldAmount_ << endl;
	file << maxGoldAmount_ << endl;
	file << manaAmount_ << endl;
	file << maxManaAmount_ << endl;
	file << ammo_ << endl;
	file << name_ << endl;
	file << handPos.getX() << endl;
	file << handPos.getY() << endl;
	file << spawnPos.getX() << endl;
	file << spawnPos.getY() << endl;
	health.serialize(file);
	stats.serialize(file);
}

/*void Player::serialize(ofstream& file)
{
	file << LOAD::L_Player << endl;
	file << goldAmount_ << endl;
	file << maxGoldAmount_ << endl;
	file << manaAmount_ << endl;
	file << maxManaAmount_ << endl;
	file << name_ << endl;
	file << handPos.getX() << endl;
	file << handPos.getY() << endl;
	file << spawnPos.getX() << endl;
	file << spawnPos.getY() << endl;
}*/

void Player::serialize(stringstream & file)
{
	file << LOAD::L_Player << endl;
	file << goldAmount_ << endl;
	file << maxGoldAmount_ << endl;
	file << manaAmount_ << endl;
	file << maxManaAmount_ << endl;
	file << ammo_ << endl;
	file << name_ << endl;
	file << handPos.getX() << endl;
	file << handPos.getY() << endl;
	file << spawnPos.getX() << endl;
	file << spawnPos.getY() << endl;
	health.serialize(file);
	stats.serialize(file);
}

void Player::deserialize(fstream& file)
{
	int pos_x;
	int pos_y;
	int spos_x;
	int spos_y;
	file >> goldAmount_;
	file >> maxGoldAmount_;
	file >> manaAmount_;
	file >> maxManaAmount_;
	file >> ammo_;
	file >> name_;
	file >> pos_x;
	file >> pos_y;
	file >> spos_x;
	file >> spos_y;
	handPos.setX(pos_x);
	handPos.setY(pos_y);
	spawnPos.setX(spos_x);
	spawnPos.setY(spos_y);
	health.deserialize(file);
	stats.deserialize(file);
}

/*void Player::deserialize(ifstream& file)
{
	int pos_x;
	int pos_y;
	int spos_x;
	int spos_y;
	file >> goldAmount_;
	file >> maxGoldAmount_;
	file >> manaAmount_;
	file >> maxManaAmount_;
	file >> name_;
	file >> pos_x;
	file >> pos_y;
	file >> spos_y;
	file >> spos_x;
	handPos.setX(pos_x);
	handPos.setY(pos_y);
	spawnPos.setX(spos_x);
	spawnPos.setY(spos_y);
}*/

void Player::deserialize(stringstream& file)
{
	file.clear();
	int pos_x;
	int pos_y;
	int spos_x;
	int spos_y;
	file >> goldAmount_;
	file >> maxGoldAmount_;
	file >> manaAmount_;
	file >> maxManaAmount_;
	file >> ammo_;
	file >> name_;
	file >> pos_x;
	file >> pos_y;
	file >> spos_y;
	file >> spos_x;
	handPos.setX(pos_x);
	handPos.setY(pos_y);
	spawnPos.setX(spos_x);
	spawnPos.setY(spos_y);
	health.deserialize(file);
	stats.deserialize(file);
}

void Player::update()
{
	int pHealth = health.getHealth();
	health.update();
	if (health.getHealth() != pHealth)
	{
		std::stringstream msg;
		msg << SendDefault << EndLine
			<< UpdatePlayer << EndLine
			<< Health << EndLine
			<< name_ << EndLine
			<< health.getHealth() << EndLine;
		SendServerLiteral(msg.str());
	}
	if (miningStopSound.Update() == true && isSoundPlayingM_ == true)
	{
		game::m_sounds.StopSound("Mining");
		isSoundPlayingM_ = false;
	}
	turret_sound.Update();
}

bool Player::hasComponent(int id)
{
	switch (id)
	{
	case COMPONENT_HEALTH: return true;
	default: return false;
	}
}

bool Player::isKilled()
{
	return isDead_;
}

void Player::kill()
{
	isDead_ = true;
	clean();

	forceHandPosition(spawnPos, game::game);

	stringstream msg;
	msg << SendDefault << KillPlayer << EndLine << name_ << EndLine;
	SendServerLiteral(msg.str());
}

void Player::killS()
{
	isDead_ = true;
	clean();

	forceHandPosition(spawnPos, game::game);
}

void Player::Log(std::string txt)
{
	std::fstream file("Logs//Log.txt", std::ios::app);

	file << txt << std::endl;
}

void Player::clean()
{
	game::game.removeSelectedAtTile(handPos);
}

void Player::setPos(Position pos)
{
	forceHandPosition(pos);
}

void Player::updateOverlay()
{
	return;
}

void Player::updateID()
{
	return;
}

void Player::send()
{
	return;
}

Position Player::getPos()
{
	return handPos;
}

////////////////////////////////////////////////

Player CreatePlayerAt(Position pos, std::string name) // Creates player, but doesn't add to map
{
	Player NewPlayer;
	NewPlayer.setHandPos(pos);
	NewPlayer.setSpawnPos(pos);
	NewPlayer.setName(name);
	return NewPlayer;
}

Player CreatePlayerAndSendAt(Position pos, std::string name, list<int> toSend) // Creates player, sends to other, but doesn't add to map
{
	Player NewPlayer;
	NewPlayer.setHandPos(pos);
	NewPlayer.setSpawnPos(pos);
	NewPlayer.setName(name);
	std::stringstream msg;
	for (auto& iter : toSend)
	{
		msg << iter << EndLine << AddPlayer << EndLine;
		NewPlayer.serialize(msg);
		SendServerLiteral(msg.str());
		msg.str(string());
	}
	return NewPlayer;
}

Player Common::CreatePlayer(Position pos, std::string name, bool isLocal)
{
	Player NewPlayer;
	NewPlayer.setHandPosNoUpdate(pos);
	NewPlayer.setSpawnPos(pos);
	NewPlayer.setName(name);
	if (isLocal)
	{
		Common::AddLocalPlayer(&NewPlayer);
	}
	else
	{
		Common::AddPlayer(&NewPlayer);

	}
	return NewPlayer;
}
