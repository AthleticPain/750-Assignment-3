#include <GLFW/glfw3.h>
#include<AL/al.h>
#include<AL/alc.h>
#include<AudioFile/audiofile.h>
#include <iostream>
#include<string>

//OpenAL error checking
#define OpenAL_ErrorCheck(message)\
{\
	ALenum error = alGetError();\
	if( error != AL_NO_ERROR)\
	{\
		std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl;\
	}\
}

#define alec(FUNCTION_CALL)\
FUNCTION_CALL;\
OpenAL_ErrorCheck(FUNCTION_CALL)

std::string audioFilePaths[] = { "sounds/cannon.wav", "sounds/Explosion.wav", "sounds/missileGround.wav" };
const int numberOfAudioTracks = sizeof(audioFilePaths) / sizeof(audioFilePaths[0]);

//0 -> cannon, 1 -> explosion with tank, 2->Ground hit
ALuint audioSources[numberOfAudioTracks];

void PlayAudio(int trackIndex)
{
	//0->cannon
	//1->explosion
	//3->ground hit
	alec(alSourcePlay(audioSources[trackIndex]));
}

using namespace std;

const float ACCELERATION_DUE_TO_GRAVITY = 9.8f;
const int SCREENSIZE_X = 1000;
const int SCREENSIZE_Y = 800;
const float PI = 3.14;

int numberOfTanks = 0;
int deathCount = 0;
int currentPlayer = 0;

float projectilePositionX;
float projectilePositionY;
float projectileVelocityX;
float projectileVelocityY;
bool isShooting = false;
bool isTankPoweringUp = false;
vector<float> projectileTrailVertices;

float floorHeight = 100;

const float TankMinPower = 10;
const float TankMaxPower = 100;
const float TankMinAngle = 20;
const float TankMaxAngle = 180 - TankMinAngle;

struct Tank
{
	int xCoordinate = 0;
	int yCoordinate = 0;
	int tankSize = 0; //Radius of tank from center
	bool isAlive = true;
	float angle = TankMinAngle;
	float power = 0;
};

Tank* allTanks;

// Convert Game Coordinates to OpenGL's Coordinate system
float NormalizeCoordinates_X(float x)
{
	return (x / SCREENSIZE_X) * 2 - 1;
}

float NormalizeCoordinates_Y(float y)
{
	return (y / SCREENSIZE_Y) * 2 - 1;
}

// Draw a Tank
void DrawTank(float tankCenter_x, float tankCenter_y, float tankRadius, float cannonAngle, int segments = 30)
{
	glColor3f(0.5, 0.5, 0.5);

	// Scale tank size to OpenGL coordinates
	float normalizedSize = (tankRadius / SCREENSIZE_X) * 2;

	//Draw the Tank
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(tankCenter_x, tankCenter_y);
	for (int i = 0; i <= segments; i++)
	{
		float theta = 2.0f * PI * i / segments;
		float x = normalizedSize * cos(theta);
		float y = normalizedSize * sin(theta);
		glVertex2f(tankCenter_x + x, tankCenter_y + y);
	}
	glEnd();

	glPointSize(10);

	glBegin(GL_POINTS);
	glVertex2f(tankCenter_x, tankCenter_y);
	glEnd();

	glPushMatrix();
	glTranslatef(tankCenter_x, tankCenter_y, 0.0f);
	glRotatef(cannonAngle, 0.0f, 0.0f, 1.0f);

	//Draw the tank's cannon
	glBegin(GL_QUADS);
	glVertex2f(0, -0.25 * normalizedSize);
	glVertex2f(normalizedSize * 1.5, -0.25 * normalizedSize);
	glVertex2f(normalizedSize * 1.5, 0.25 * normalizedSize);
	glVertex2f(0, 0.25 * normalizedSize);
	glEnd();

	glPopMatrix();
}

void DrawPowerBar(Tank currentTank, float horizontalScaleModifier)
{
	glColor3f(1, 0.5, 0);

	glPushMatrix();
	glTranslatef(NormalizeCoordinates_X(currentTank.xCoordinate - currentTank.tankSize), NormalizeCoordinates_Y(currentTank.yCoordinate + currentTank.tankSize + 1), 0);
	glScalef(horizontalScaleModifier, 1, 1);

	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(NormalizeCoordinates_X(currentTank.tankSize * 2) + 1, 0);
	glVertex2f(NormalizeCoordinates_X(currentTank.tankSize * 2) + 1, 0.025);
	glVertex2f(0, 0.025);
	glEnd();

	glPopMatrix();
}

//Draw a point at the projectile's position
void DrawProjectile(float x, float y)
{
	glColor3f(1, 0, 0);
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glVertex2f(x, y);
	glEnd();
}

//Draw a line trail for the projectile's path
void DrawProjectileTrail()
{
	if (isShooting)
	{
		glColor3f(0, 0, 0);

		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < projectileTrailVertices.size() - 1; i = i + 2)
		{
			glVertex2f(projectileTrailVertices[i], projectileTrailVertices[i + 1]);
		}
		glEnd();
	}
}

//Draw the floor
void DrawFloor()
{
	glColor3f(0, 0.55, 0);
	glBegin(GL_QUADS);
	glVertex2f(NormalizeCoordinates_X(0), NormalizeCoordinates_Y(0));
	glVertex2f(NormalizeCoordinates_X(SCREENSIZE_X), NormalizeCoordinates_Y(0));
	glVertex2f(NormalizeCoordinates_X(SCREENSIZE_X), NormalizeCoordinates_Y(floorHeight));
	glVertex2f(NormalizeCoordinates_X(0), NormalizeCoordinates_Y(floorHeight));
	glEnd();
}

void keyboardInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && !isShooting)
	{
		if (action == GLFW_PRESS)
		{
			//Set power to minimum power here
			allTanks[currentPlayer].power = TankMinPower;
			isTankPoweringUp = true;
		}

		if (action == GLFW_REPEAT)
		{
			//Increase power incrementally till max 150
			allTanks[currentPlayer].power += 2;

			if (allTanks[currentPlayer].power >= TankMaxPower)
			{
				allTanks[currentPlayer].power = TankMaxPower;
			}
		}

		if (action == GLFW_RELEASE)
		{
			//Convert angle to radians
			float angleInRadians = allTanks[currentPlayer].angle * PI / 180;

			//Projectile initial positions 
			//Ensuring it does not start exactly on the same position as the tank itself and shoot itself initially
			projectilePositionX = allTanks[currentPlayer].xCoordinate + allTanks[currentPlayer].tankSize * cos(angleInRadians);
			projectilePositionY = allTanks[currentPlayer].yCoordinate + allTanks[currentPlayer].tankSize * sin(angleInRadians);
			projectileVelocityX = cos(angleInRadians) * allTanks[currentPlayer].power;
			projectileVelocityY = sin(angleInRadians) * allTanks[currentPlayer].power;

			PlayAudio(0);
			isTankPoweringUp = false;
			isShooting = true;
		}
	}
	
	if (action == GLFW_PRESS || action == GLFW_REPEAT && !isTankPoweringUp)
	{
		if (key == GLFW_KEY_LEFT && !isShooting)
		{
			allTanks[currentPlayer].angle += 1;
			if (allTanks[currentPlayer].angle > TankMaxAngle)
			{
				allTanks[currentPlayer].angle = TankMaxAngle;
			}
		}

		if (key == GLFW_KEY_RIGHT && !isShooting)
		{
			allTanks[currentPlayer].angle -= 1;
			if (allTanks[currentPlayer].angle < TankMinAngle)
			{
				allTanks[currentPlayer].angle = TankMinAngle;
			}
		}
	}

}

//Returns the index of the player whose turn is next
int GetNextPlayerIndex(int currentPlayerIndex)
{
	int nextPlayerIndex = (currentPlayerIndex + 1) % numberOfTanks;

	while (allTanks[nextPlayerIndex].isAlive == false)
	{
		cout << "Number of tanks is " << numberOfTanks;
		cout << "\nChecking tank status for index " << nextPlayerIndex;
		nextPlayerIndex = (nextPlayerIndex + 1) % numberOfTanks;
	}

	std::cout << "Next player is player " << nextPlayerIndex + 1;
	return nextPlayerIndex;
}

//Updates the position of the projectile once, when called in the main game loop it will update continuosly
void  CalculateProjectileMotion(float timeStep, Tank shooter, Tank* allTanks, int numberOfTanks, int angle, int power)
{
	//Define time step for projectile motion calculations
	float elapsedTime = 0;

	//Update projectile position
	projectilePositionX += projectileVelocityX * timeStep;
	projectilePositionY += projectileVelocityY * timeStep - 0.5 * ACCELERATION_DUE_TO_GRAVITY * timeStep * timeStep;

	projectileTrailVertices.push_back(NormalizeCoordinates_X(projectilePositionX));
	projectileTrailVertices.push_back(NormalizeCoordinates_Y(projectilePositionY));

	elapsedTime += timeStep;

	cout << "\nProjectile position at elapsed time " << elapsedTime << ": (" << projectilePositionX << ", " << projectilePositionY << ")";

	//Update vertical velocity
	projectileVelocityY -= ACCELERATION_DUE_TO_GRAVITY * timeStep;

	for (int i = 0; i < numberOfTanks; i++)
	{
		if (allTanks[i].isAlive == true)
		{
			//Calculate squared distance of projectile to tank
			float distanceToTankSquared = (projectilePositionX - allTanks[i].xCoordinate) * (projectilePositionX - allTanks[i].xCoordinate) + (projectilePositionY - allTanks[i].yCoordinate) * (projectilePositionY - allTanks[i].yCoordinate);

			cout << "\nProjectile distance from Tank " << i + 1 << " of size " << allTanks[i].tankSize << ": " << sqrt(distanceToTankSquared);

			//Check if squared distance is less than squared tank size
			//If true, set isAlive to false and return true
			if (distanceToTankSquared <= (allTanks[i].tankSize * allTanks[i].tankSize))
			{
				allTanks[i].isAlive = false;
				cout << "\n\nProjectile hit Tank " << i + 1 << "! The tank is destroyed!";
				deathCount++;

				PlayAudio(1);
				projectileTrailVertices.clear();
				currentPlayer = GetNextPlayerIndex(currentPlayer);
				isShooting = false;
			}
		}
	}

	if (projectilePositionY < floorHeight || projectilePositionX > SCREENSIZE_X || projectilePositionX < 0)
	{
		PlayAudio(2);
		projectileTrailVertices.clear();
		currentPlayer = GetNextPlayerIndex(currentPlayer);
		isShooting = false;
	}
}

int main()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// find the default audio device
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const ALCchar* defaultDeviceString = alcGetString(/*device*/nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice* device = alcOpenDevice(defaultDeviceString);
	if (!device)
	{
		std::cerr << "failed to get the default device for OpenAL" << std::endl;
		return -1;
	}
	std::cout << "OpenAL Device: " << alcGetString(device, ALC_DEVICE_SPECIFIER) << std::endl;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create an OpenAL audio context from the device
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ALCcontext* context = alcCreateContext(device, /*attrlist*/ nullptr);
	OpenAL_ErrorCheck(context);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Activate this context so that OpenAL state modifications are applied to the context
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (!alcMakeContextCurrent(context))
	{
		std::cerr << "failed to make the OpenAL context the current context" << std::endl;
		return -1;
	}
	OpenAL_ErrorCheck("Make context current");

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create a listener in 3d space (ie the player); (there always exists as listener, you just configure data on it)
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	alec(alListener3f(AL_POSITION, 0.f, 0.f, 0.f));
	alec(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));
	ALfloat forwardAndUpVectors[] = {
		/*forward = */ 1.f, 0.f, 0.f,
		/* up = */ 0.f, 1.f, 0.f
	};
	alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// create a sound source that play's our mono sound (from the sound buffer)
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	for (int i = 0; i < numberOfAudioTracks; i++)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create buffers that hold our sound data; these are shared between contexts and ar defined at a device level
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		AudioFile<float> monoSoundFile;
		std::vector<uint8_t> monoPCMDataBytes;

		if (!monoSoundFile.load(audioFilePaths[i]))
		{
			std::cerr << "failed to load the test mono sound file" << std::endl;
			return -1;
		}
		monoSoundFile.writePCMToBuffer(monoPCMDataBytes); //remember, we added this function to the AudioFile library

		auto convertFileToOpenALFormat = [](const AudioFile<float>& audioFile) {
			int bitDepth = audioFile.getBitDepth();
			if (bitDepth == 16)
				return audioFile.isStereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			else if (bitDepth == 8)
				return audioFile.isStereo() ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
			else
				return -1; // this shouldn't happen!
			};
		ALuint monoSoundBuffer;
		alec(alGenBuffers(1, &monoSoundBuffer));
		alec(alBufferData(monoSoundBuffer, convertFileToOpenALFormat(monoSoundFile), monoPCMDataBytes.data(), monoPCMDataBytes.size(), monoSoundFile.getSampleRate()));

		alec(alGenSources(1, &audioSources[i]));
		alec(alSource3f(audioSources[i], AL_POSITION, 1.f, 0.f, 0.f));
		alec(alSource3f(audioSources[i], AL_VELOCITY, 0.f, 0.f, 0.f));
		alec(alSourcef(audioSources[i], AL_PITCH, 1.f));
		alec(alSourcef(audioSources[i], AL_GAIN, 1.f));
		alec(alSourcei(audioSources[i], AL_LOOPING, AL_FALSE));
		alec(alSourcei(audioSources[i], AL_BUFFER, monoSoundBuffer));

		alec(alDeleteBuffers(1, &monoSoundBuffer));
	}

	// Initialize GLFW
	if (!glfwInit()) return -1;

	GLFWwindow* openGLwindow = glfwCreateWindow(SCREENSIZE_X, SCREENSIZE_Y, "Tank Game", NULL, NULL);
	if (!openGLwindow) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(openGLwindow);
	glfwSetKeyCallback(openGLwindow, keyboardInputCallback);

	while (numberOfTanks < 2 || numberOfTanks > 10)
	{
		cout << "\nEnter the number of tanks (2, 10):";
		cin >> numberOfTanks;

		//If user inputs anything other than an integer, exit
		if (std::cin.fail())
			return -1;
	}

	allTanks = new Tank[numberOfTanks];

	srand(time(0));

	//Spawn all tanks with random details
	for (int i = 0; i < numberOfTanks; i++)
	{
		int newRandomXPos = rand() % SCREENSIZE_X; //Random tank x coordinate

		int newRandomYPos = floorHeight; //Random tank y coordinate
		int randomTankSize = 10 + rand() % 21; //Random Tank size from 10 to 20 pixels

		allTanks[i] = { newRandomXPos, newRandomYPos, randomTankSize };

		cout << "\nTank " << i + 1 << " of size " << randomTankSize << " pixels, spawned at coordinates (" << newRandomXPos << ", " << newRandomYPos << ").";
	}

	//Main game loop. Keeps looping until one tank is left alive.
	while (!glfwWindowShouldClose(openGLwindow))
	{
		glClearColor(1.0, 1.0, 1.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		//If only one remaining tank, exit the main game loop
		if (deathCount >= numberOfTanks - 1)
		{
			break;
		}

		//Draw Power bar
		if (isTankPoweringUp)
		{
			DrawPowerBar(allTanks[currentPlayer], (allTanks[currentPlayer].power - TankMinPower) / (TankMaxPower - TankMinPower));
		}

		// Draw projectile
		if (isShooting)
		{
			CalculateProjectileMotion(0.01f, allTanks[currentPlayer], allTanks, numberOfTanks, allTanks[currentPlayer].angle, allTanks[currentPlayer].power);
			DrawProjectile(NormalizeCoordinates_X(projectilePositionX), NormalizeCoordinates_Y(projectilePositionY));
			DrawProjectileTrail();
		}

		// Draw the tanks
		for (int i = 0; i < numberOfTanks; i++)
		{
			if (allTanks[i].isAlive)
			{
				DrawTank(NormalizeCoordinates_X(allTanks[i].xCoordinate), NormalizeCoordinates_Y(allTanks[i].yCoordinate), allTanks[i].tankSize, allTanks[i].angle);
			}
		}

		//Draw floor
		DrawFloor();

		glfwSwapBuffers(openGLwindow);
		glfwPollEvents();
	}

	//Find which tank is left alive
	int winningTankIndex = -1;
	for (int i = 0; i < numberOfTanks; i++)
	{
		if (allTanks[i].isAlive)
		{
			winningTankIndex = i;
			break;
		}
	}
	cout << "\n\nGame Over! Tank " << winningTankIndex + 1 << " is the winner!\n";

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// clean up our resources!
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < numberOfAudioTracks; i++)
	{
		alec(alDeleteSources(1, &audioSources[i]));
	}
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);

	glfwTerminate();
	return 0;
}