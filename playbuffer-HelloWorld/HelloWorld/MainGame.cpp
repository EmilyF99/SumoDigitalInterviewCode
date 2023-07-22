#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

//Game Display Window Parameters
int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
};

//Struct for overall state of the game
struct GameState
{
	int score = 0;
	Agent8State agentState = STATE_APPEAR;
};

GameState gameState;

//Type that defines behaviour or GameObjects, links to a struct
enum GameObjectType
{
	// -1 represents unitialised objects, next objects are auto assigned
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};

//Function Declarations
void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinsAndStars();
void UpdateLasers();
void UpdateDestroyed();
void UpdateAgent8();

// The entry point for a PlayBuffer program used instead of Main
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	//Sets Window Size
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	//Moves local origins to centre
	Play::CentreAllSpriteOrigins();
	
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");

	//Creates Player Object, inital position, collision and what sprite to use
	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");

	//Creates Fan Object, inital position, no collision, and sprite name
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");

	Play::GetGameObject(id_fan).velocity = { 0, 3 };
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	//Draws are added in layers, so background must be first.
	Play::DrawBackground();
	UpdateAgent8(); // Replaces HandlePlayerControls() in MainGameUpdate(
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateDestroyed();
	Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE",
		{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score),
		{ DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	//Displays current Drawings on Screen
	Play::PresentDrawingBuffer();
	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

//Payer Controls with Keys 
void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	if (Play::KeyDown(VK_UP))
	{
		obj_agent8.velocity = { 0, -4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN))
	{
		obj_agent8.acceleration = { 0, 1 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	else
	{
		if (obj_agent8.velocity.y > 5)
		{
			gameState.agentState = STATE_HALT;
			Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
			obj_agent8.acceleration = { 0, 0 };
		}
		else
		{
			Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
			obj_agent8.velocity *= 0.5f;
			obj_agent8.acceleration = { 0, 0 };
		}
	}
	if (Play::KeyPressed(VK_SPACE))
	{
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");
		Play::GetGameObject(id).velocity = { 32, 0 };
		Play::PlayAudio("shoot");
	}
	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8))
		obj_agent8.pos = obj_agent8.oldPos;

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}

//Creates Fan object, position is moved on random dice roll
void UpdateFan()
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	//Roll of a 50 on a 50 dice
	if (Play::RandomRoll(50) == 50)
	{
		//Creates a tool object for a Screwdriver
		//Created at fans position, collision of 50 and driver sprite
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);
		//Sets the direction of the tool and then times by 6 to set Y Axis Velocity
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);

		//Gives chance for tool to turn into spanner
		if (Play::RandomRoll(2) == 1)
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			//Updates the radius, speed and sets to rotate
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}
		//spawning sound
		Play::PlayAudio("tool");
	}
	if (Play::RandomRoll(150) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}
	Play::UpdateGameObject(obj_fan);

	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}
	Play::DrawObject(obj_fan);
}

//Used for attacking tools and collision
void UpdateTools()
{
	//Needs to get agent8 to detect collisions
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	
	//Stores the ID of all the tool objects within a Vector
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	//For Loop assigns values to each item in the vector
	for (int id : vTools)
	{
		GameObject& obj_tool = Play::GetGameObject(id);
		//If Agent is Not Dead and there is collision
		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8))
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			//Hides rather than destroyed as needed for restart
			gameState.agentState = STATE_DEAD;
		}
		Play::UpdateGameObject(obj_tool);

		//Checker for if object is leaving
		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}
		Play::DrawObjectRotated(obj_tool);
		
		//If object is not in play then destory it 
		if (!Play::IsVisible(obj_tool))
			Play::DestroyGameObject(id);

	}

}

//Used for collectables
void UpdateCoinsAndStars()
{
	//Agent 8 needed here for collision
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	
	//All collectables ID's are stored in a vector
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);
	
	//For all within  the vector
	for (int id_coin : vCoins)
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);
		//sets collision to false
		bool hasCollided = false;
		//Then if the player is coliding 
		if (Play::IsColliding(obj_coin, obj_agent8))
		{
			//show rotating stars 
			//Creates four stars in the corner of the coin 
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}
			//Colliision marker set to true 
			//Not destroyed straight away as would crash due to still trying to access the object
			hasCollided = true;
			
			//Increase score and play sound
			gameState.score += 500;
			Play::PlayAudio("collect");
		}
		//Update GameObjects and Position (Outside of IF)
		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);
		//If the object is no longer visible or has previously collided
		//Then destroy
		if (!Play::IsVisible(obj_coin) || hasCollided)
			Play::DestroyGameObject(id_coin);
	}
	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);
	for (int id_star : vStars)
	{
		GameObject& obj_star = Play::GetGameObject(id_star);
		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);
		//Once no longer visible, destroy star
		if (!Play::IsVisible(obj_star))
			Play::DestroyGameObject(id_star);
	}
}

//Lasers for Agent 8
void UpdateLasers()
{
	//Needed for collision on objects
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	//for all the laser objects
	for (int id_laser : vLasers)
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		//Collision set to false
		bool hasCollided = false;
		
		//for all the tools
		for (int id_tool : vTools)
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			//If colliding between laser and tool
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				//Collison is set to true, type set to destroy so it fades, points incremented
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score += 100;
			}
		}
		//for coins
		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			//If colliding between laser and coin
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				//Collision set to true, set to destroy so it fades, points decremented
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
			}
		}
		//Checker that stops game score dropping below 0
		if (gameState.score < 0)
			gameState.score = 0;

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);
		//If laser is no longer on screen or has collided then the laser is destroyed.
		if (!Play::IsVisible(obj_laser) || hasCollided)
			Play::DestroyGameObject(id_laser);
	}
}

//For all objects that are waiting to be destroyed
//Allows existance until they become invisble or are dead for 10 frames or more
//Allows the rest of the code relating to the object to be completed before the object is destroyed
//Stops errors from accessing a destroyed object.
void UpdateDestroyed()
{
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);
	for (int id_dead : vDead)
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);
		if (obj_dead.frame % 2)
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);
		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
			Play::DestroyGameObject(id_dead);
	}
}

//Improving game smoothness by having custom states
//Allows for a set starting position and set behaviour 
void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	switch (gameState.agentState)
	{
	case STATE_APPEAR:
		obj_agent8.velocity = { 0, 12 };
		obj_agent8.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
		obj_agent8.rotation = 0;
		if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3)
			gameState.agentState = STATE_PLAY;
		break;
	case STATE_HALT:
		obj_agent8.velocity *= 0.9f;
		if (Play::IsAnimationComplete(obj_agent8))
			gameState.agentState = STATE_PLAY;
		break;
	case STATE_PLAY:
		HandlePlayerControls();
		break;
	case STATE_DEAD:
		obj_agent8.acceleration = { -0.3f , 0.5f };
		obj_agent8.rotation += 0.25f;
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115, 0 };
			obj_agent8.velocity = { 0, 0 };
			obj_agent8.frame = 0;
			Play::StartAudioLoop("music");
			gameState.score = 0;
			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
		}
		break;
	} // End of switch on Agent8State
	Play::UpdateGameObject(obj_agent8);
	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
		obj_agent8.pos = obj_agent8.oldPos;
	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}