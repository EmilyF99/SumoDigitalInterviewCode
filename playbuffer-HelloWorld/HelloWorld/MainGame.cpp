#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include <Windows.h>

//Game Display Window Parameters
int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

//Globals 

bool gameStarted = false;
bool isAudioPlaying = true;
bool extraLifeText = false;
float extraLifeTextTimer = 0.0f;
bool bonusRoundText = false;
float bonusRoundTimer = 0.0f;
bool isPlayerInvincible = false;
float invincibilityTimer = 0.0f;
bool speedUpText = false;
bool speedUpTextTimer = 0.0f;



//Float Timers
const float EXTRA_LIFE_TEXT_DISPLAY_TIME = 2.5f;
const float BONUS_ROUND_TIME = 10.0F;
const float INVINCIBILITY_TIME = 1.0F;


std::vector<int> vBonusRoundObjectsIDs;

enum GameScreen
{
	STATE_START,
	STATE_OPTIONS,
	STATE_AUDIO,
	STATE_CONTROLS,
	STATE_MAIN_GAME,
	STATE_GAME_OVER
};

GameScreen currentGameScreen = STATE_START;

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
	int lives = 3;
	int scoreForNextLife = 9000;
	int scoreForBonusRound = 20000;
	int gameTimer = 0;
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
void DrawStartScreen();
void DrawGameOverScreen();
void DrawExtraLifeText();
void DrawBonusLevelText();
//void DrawSpeedUpText();
void ResetGameState();
void DestroyAllObjects();

// The entry point for a PlayBuffer program used instead of Main
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	//Sets Window Size
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	//Moves local origins to centre
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");

	currentGameScreen = STATE_START;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	switch (currentGameScreen)
	{
	//First Screen that player sees
	case STATE_START:

		DrawStartScreen();
		//Game Starts when player hits space
		if (Play::KeyDown(VK_SPACE)) {
			currentGameScreen = STATE_MAIN_GAME;
			//Resets all game details, like score and lives
			ResetGameState();
			gameStarted = false;
		}
		return Play::KeyDown(VK_ESCAPE);
		break;

	case STATE_MAIN_GAME:
		if (!gameStarted) {
			// The game has just started, do the initialization here.
			Play::CentreAllSpriteOrigins();
			Play::LoadBackground("Data\\Backgrounds\\background.png");

			if (isAudioPlaying) {
				Play::StartAudioLoop("music");
			}
	
			Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
			int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");
			Play::GetGameObject(id_fan).velocity = { 0, 3 };
			Play::GetGameObject(id_fan).animSpeed = 1.0f;

			// Set the gameStarted to true so this initialization won't run again.
			gameStarted = true;
		}

		//Added in Layers so Background Must be drawn first
		Play::DrawBackground();
		UpdateAgent8(); // Replaces HandlePlayerControls() in MainGameUpdate(
		UpdateFan();
		UpdateTools();
		UpdateCoinsAndStars();
		UpdateLasers();
		UpdateDestroyed();

		//Play instructions and controls
		Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE",
			{ DISPLAY_WIDTH / 2, 30 }, Play::CENTRE);
		Play::DrawFontText("64px", "TOGGLE M TO MUTE / UNMUTE",
			{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);

		//Life and score HUD
		Play::DrawFontText("64px", "SCORE: " + std::to_string(gameState.score),
			{ 100, 50 }, Play::CENTRE);
		Play::DrawFontText("64px", "Lives: " + std::to_string(gameState.lives), { 100, 100 }, Play::CENTRE);

		
		//If the extra life boolean is true
		if (extraLifeText)
		{
			//Start the timer for how long the text stays on screen
			extraLifeTextTimer -= elapsedTime;
			//If the timer is above 0
			if (extraLifeTextTimer > 0.0f)
			{
				//Draw extra life text, middle of screen
				Play::DrawFontText("64px", "Extra Life!", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
			}
			else
			{
				//else set boolean to false
				extraLifeText = false;
			}
		}

		//If the bonus round boolean is true
		if (bonusRoundText)
		{
			//Start the timer for how long text stays on screen
			bonusRoundTimer -= elapsedTime;
			//If the timer is greater than 0
			if (bonusRoundTimer > 0.0f)
			{
				//Draw Bonus Round text, middle of screen
				Play::DrawFontText("64px", "Bonus Round!", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
			}
			else 
			{
				//else set boolean to false
				bonusRoundText = false;
			}
		}
		
		//Displays current Drawings on Screen
		Play::PresentDrawingBuffer();

		//If the user presses M Stop and Start audio
		if (Play::KeyPressed('M')) {
			//If audio is already playing stop the audio
			if (isAudioPlaying) {
				Play::StopAudioLoop("music");
			}
			else {
				//If audio is not playing then start the audio
				Play::PlayAudio("music");
			}
			isAudioPlaying = !isAudioPlaying;
		}


		return Play::KeyDown(VK_ESCAPE);
		break;

	case STATE_GAME_OVER:
		//Draw the Game Over Screen, replaces current 
		DrawGameOverScreen();
		//If space is pressed retart the game
		if (Play::KeyPressed(VK_SPACE))
		{
			currentGameScreen = STATE_MAIN_GAME;
			gameStarted = false;
			ResetGameState();
			//All previous objects need to be destroyed before game can be restarted
			//Other wise errors occur due to having duplicate objects. 
			DestroyAllObjects();
		}
		return Play::KeyDown(VK_ESCAPE);
		break;
	}

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
		//If the bonus round is showing if statement is called
		if (bonusRoundText)
		{
			//All weapons are changed to coins for the bonuse round
			int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin_resize");
			GameObject& obj_coin = Play::GetGameObject(id);
			obj_coin.velocity = { -3, 0 };
			obj_coin.rotSpeed = 0.1f;
			Play::PlayAudio("collect");
		}
		else {
			//Creates a tool object for a Screwdriver
			//Created at fans position, collision of 50 and driver sprite
			int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver_resize");
			GameObject& obj_tool = Play::GetGameObject(id);
			//Sets the direction of the tool and then times by 6 to set Y Axis Velocity
			obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);

			//Gives chance for tool to turn into spanner
			if (Play::RandomRoll(2) == 1)
			{
				Play::SetSprite(obj_tool, "spanner_resize", 0);
				//Updates the radius, speed and sets to rotate
				obj_tool.radius = 75;
				obj_tool.velocity.x = -4;
				obj_tool.rotSpeed = 0.1f;
			}
			//spawning sound
			Play::PlayAudio("tool");
		}
	
	}
	//If roll successful, coin is generated rather than a tool
	if (Play::RandomRoll(150) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin_resize");
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
		//If Agent is Not Dead and they are not currently in invincibility frames and there is a collision
		if (gameState.agentState != STATE_DEAD && !isPlayerInvincible && Play::IsColliding(obj_tool, obj_agent8))
		{
			Play::PlayAudio("die");

			//Lives are reduced
			gameState.lives--;
			//Set invincibility frames to true and set invinciblity timer to the constant set earlier
			isPlayerInvincible = true;
			invincibilityTimer = INVINCIBILITY_TIME;


			//If the player has move than 0 lives, position of agent resets
			if (gameState.lives > 0)
			{
				gameState.agentState = STATE_APPEAR;
				obj_agent8.pos = { 115, 0 };
				obj_agent8.velocity = { 0, 0 };
				obj_agent8.frame = 0;
			}

			//If no lifes left, play explode sound and start game over screen
			else 
			{
				Play::PlayAudio("explode");
				gameState.agentState = STATE_DEAD;
				currentGameScreen = STATE_GAME_OVER;
			}
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

		//If the current score is bigger than or equal to the next score needed for an extra life
		if (gameState.score >= gameState.scoreForNextLife)
		{
			//increase the score needed by 9000
			//prevents the player from life farming by dropping below 8000 then back up to over 9000 on purpose
			gameState.scoreForNextLife += 9000; 
			//add one extra life
			gameState.lives += 1;
			//Draw the extra life text
			DrawExtraLifeText();
		}

		//If current score is bigger or equal to the next score needed for the bonus round to start
		if (gameState.score >= gameState.scoreForBonusRound) {
			//prevents the player from bonus level farming in same way as life farming
			gameState.scoreForBonusRound += 20000;
			DrawBonusLevelText();
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
				gameState.score -= 200;
			}
		}
		//Checker that stops game score dropping below 0
		if (gameState.score < 0)
			gameState.score = 0;
		
		if (gameState.lives <= 0) {
			currentGameScreen = STATE_GAME_OVER;
		}

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
		
		// If the player has just been hit
		//Start timer
		if (isPlayerInvincible)
		{
			//Timer linked to frames as a work around of not being able to pass elapsed timer 
			invincibilityTimer -= 1.0f / 60.0f; 
			if (invincibilityTimer <= 0.0f)
			{
				//Gets rid of invincibility frames
				isPlayerInvincible = false; 
			}
		}
	
		break;
	case STATE_DEAD:
		obj_agent8.acceleration = { -0.3f , 0.5f };
		obj_agent8.rotation += 0.25f;

		//If spaceBar pressed game is restarted and score is set to 0
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115, 0 };
			obj_agent8.velocity = { 0, 0 };
			obj_agent8.frame = 0;

			if (isAudioPlaying) {
				Play::StartAudioLoop("music");
			}
			
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


//Used to draw the starting screen
void DrawStartScreen() {
	Play::ClearDrawingBuffer(Play::cBlack);
	Play::DrawFontText("64px", "HIT SPACE TO START GAME",
		{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 100 }, Play::CENTRE);
	Play::DrawFontText("64px", "Bonus Life Every: 9000 Points", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 30 }, Play::CENTRE);
	Play::DrawFontText("64px", "Bonus Round Every: 20000 Points", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - -10 }, Play::CENTRE);
	Play::PresentDrawingBuffer();
}

//Used to draw the game over screen
void DrawGameOverScreen() {
	Play::ClearDrawingBuffer(Play::cBlack);
	Play::DrawFontText("64px", "Game Over",{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 100 }, Play::CENTRE);
	Play::DrawFontText("64px", "Your Score: " + std::to_string(gameState.score), { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 50 }, Play::CENTRE);
	Play::DrawFontText("64px", "Press Space To Restart", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 5 }, Play::CENTRE);
	Play::PresentDrawingBuffer();
}

//Sets boolean to true
//Plays Extra Life Audio
//Sets extra life timer to constant value
void DrawExtraLifeText() {
	extraLifeText = true;
	Play::PlayAudio("lifeUp");
	extraLifeTextTimer = EXTRA_LIFE_TEXT_DISPLAY_TIME;
}

//Sets boolean to true
//Plays Bonus Round Audio
//Sets Bonus Round to constant value
void DrawBonusLevelText() {
	bonusRoundText = true;
	Play::PlayAudio("bonus");
	bonusRoundTimer = BONUS_ROUND_TIME;
}


//Resets the game to initial starting values
void ResetGameState() {
	gameState.score = 0;
	gameState.lives = 3; 
	gameState.scoreForNextLife = 8000;
	gameState.scoreForBonusRound = 15000;
	gameState.agentState = STATE_APPEAR;
}

//Used to ensure all objects are destroyed when game is restarted.
void DestroyAllObjects() {
	Play::DestroyGameObjectsByType(TYPE_AGENT8);
	Play::DestroyGameObjectsByType(TYPE_FAN);
	Play::DestroyGameObjectsByType(TYPE_TOOL);
	Play::DestroyGameObjectsByType(TYPE_COIN);
	Play::DestroyGameObjectsByType(TYPE_STAR);
	Play::DestroyGameObjectsByType(TYPE_LASER);
	Play::DestroyGameObjectsByType(TYPE_DESTROYED);
}