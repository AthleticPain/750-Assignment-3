#include <GLFW/glfw3.h>
#include<AL/al.h>
#include <iostream>

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

float floorHeight = 100;

struct Tank
{
	int xCoordinate = 0;
	int yCoordinate = 0;
	int tankSize = 0; //Radius of tank from center
	bool isAlive = true;
	float angle = 0;
	float power = 100;
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
void DrawTank(float cx, float cy, float r, float cannonAngle, int segments = 30)
{
	glColor3f(0, 0, 1);

	// Scale tank size to OpenGL coordinates
	float normalizedSize = (r / SCREENSIZE_X) * 2;

	//Draw the Tank
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(cx, cy);
	for (int i = 0; i <= segments; i++)
	{
		float theta = 2.0f * PI * i / segments;
		float x = normalizedSize * cos(theta);
		float y = normalizedSize * sin(theta);
		glVertex2f(cx + x, cy + y);
	}
	glEnd();

	glPointSize(10);

	glBegin(GL_POINTS);
	glVertex2f(cx, cy);
	glEnd();

	glPushMatrix();
	glTranslatef(cx, cy, 0.0f);
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

void DrawProjectile(float x, float y)
{
	glColor3f(1, 0, 0);
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glVertex2f(x, y);
	glEnd();
}

void DrawFloor()
{
	glColor3f(0, 1, 0);
	glBegin(GL_QUADS);
	glVertex2f(NormalizeCoordinates_X(0), NormalizeCoordinates_Y(0));
	glVertex2f(NormalizeCoordinates_X(SCREENSIZE_X), NormalizeCoordinates_Y(0));
	glVertex2f(NormalizeCoordinates_X(SCREENSIZE_X), NormalizeCoordinates_Y(floorHeight));
	glVertex2f(NormalizeCoordinates_X(0), NormalizeCoordinates_Y(floorHeight));
	glEnd();
}

void keyboardInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_LEFT && !isShooting)
		{
			allTanks[currentPlayer].angle += 1;
		}
	}

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_RIGHT && !isShooting)
		{
			allTanks[currentPlayer].angle -= 1;
		}
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_SPACE && !isShooting)
	{
		//Convert angle to radians
		float angleInRadians = allTanks[currentPlayer].angle * PI / 180;

		//Projectile initial positions 
		//Ensuring it does not start exactly on the same position as the tank itself and shoot itself initially
		projectilePositionX = allTanks[currentPlayer].xCoordinate + allTanks[currentPlayer].tankSize * cos(angleInRadians);
		projectilePositionY = allTanks[currentPlayer].yCoordinate + allTanks[currentPlayer].tankSize * sin(angleInRadians);
		projectileVelocityX = cos(angleInRadians) * allTanks[currentPlayer].power;
		projectileVelocityY = sin(angleInRadians) * allTanks[currentPlayer].power;
		isShooting = true;
	}
}

bool  CalculateProjectileMotion(Tank shooter, Tank* allTanks, int numberOfTanks, int angle, int power)
{
	//Define time step for projectile motion calculations
	float timeStep = 0.01;
	float elapsedTime = 0;

	//Convert angle to radians
	float angleInRadians = angle * 3.14 / 180;

	//Projectile initial velocities
	projectileVelocityX = power * cos(angleInRadians);
	projectileVelocityY = power * sin(angleInRadians);

	//Projectile initial positions 
	//Ensuring it does not start exactly on the same position as the tank itself and shoot itself initially
	projectilePositionX = shooter.xCoordinate + shooter.tankSize * cos(angleInRadians);
	projectilePositionY = shooter.yCoordinate + shooter.tankSize * sin(angleInRadians);

	while (projectilePositionY >= 0 && projectilePositionX < SCREENSIZE_X && projectilePositionX > 0)
	{
		//Update projectile position
		projectilePositionX += projectileVelocityX * timeStep;
		projectilePositionY += projectileVelocityY - 0.5 * ACCELERATION_DUE_TO_GRAVITY * timeStep * timeStep;

		elapsedTime += timeStep;

		std::cout << "\nProjectile position at elapsed time " << elapsedTime << ": (" << projectilePositionX << ", " << projectilePositionY << ")";

		//Update vertical velocity
		projectileVelocityY -= ACCELERATION_DUE_TO_GRAVITY * timeStep;

		for (int i = 0; i < numberOfTanks; i++)
		{
			if (allTanks[i].isAlive == true)
			{
				//Calculate squared distance of projectile to tank
				float distanceToTankSquared = (projectilePositionX - allTanks[i].xCoordinate) * (projectilePositionX - allTanks[i].xCoordinate) + (projectilePositionY - allTanks[i].yCoordinate) * (projectilePositionY - allTanks[i].yCoordinate);

				std::cout << "\nProjectile distance from Tank " << i + 1 << " of size " << allTanks[i].tankSize << ": " << sqrt(distanceToTankSquared);

				//Check if squared distance is less than squared tank size
				//If true, set isAlive to false and return true
				if (distanceToTankSquared <= (allTanks[i].tankSize * allTanks[i].tankSize))
				{
					allTanks[i].isAlive = false;
					std::cout << "\n\nProjectile hit Tank " << i + 1 << "! The tank is destroyed!";
					return true;
				}
			}
		}
	}

	return false;
}

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

void  CalculateProjectileMotion(float timeStep, Tank shooter, Tank* allTanks, int numberOfTanks, int angle, int power)
{
	//Define time step for projectile motion calculations
	float elapsedTime = 0;

	//Update projectile position
	projectilePositionX += projectileVelocityX * timeStep;
	projectilePositionY += projectileVelocityY * timeStep - 0.5 * ACCELERATION_DUE_TO_GRAVITY * timeStep * timeStep;

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

				currentPlayer = GetNextPlayerIndex(currentPlayer);
				isShooting = false;
			}
		}
	}

	if (projectilePositionY < floorHeight || projectilePositionX > SCREENSIZE_X || projectilePositionX < 0)
	{
		currentPlayer = GetNextPlayerIndex(currentPlayer);

		isShooting = false;
	}
}

int main()
{
	// Initialize GLFW
	if (!glfwInit()) return -1;

	GLFWwindow* openGLwindow = glfwCreateWindow(SCREENSIZE_X, SCREENSIZE_Y, "Tank Game", NULL, NULL);
	if (!openGLwindow) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(openGLwindow);
	glfwSetKeyCallback(openGLwindow, keyboardInputCallback);

	cout << "\nEnter the number of tanks (2, 10):";
	cin >> numberOfTanks;
	//numberOfTanks = 2;

	allTanks = new Tank[numberOfTanks];

	srand(time(0));

	//Spawn all tanks with random details
	for (int i = 0; i < numberOfTanks; i++)
	{
		int newRandomXPos = rand() % SCREENSIZE_X; //Random tank x coordinate

		//Check if x position is within another tanks radius
		//bool isTankOverlapping = true;
		//while (isTankOverlapping = true)
		//{
		//	isTankOverlapping = false;
		//	for (int j = 0; j < i; j++)
		//	{
		//		cout << "Tank distance between " << i << " and " << j << ": " << abs(newRandomXPos - allTanks[j].xCoordinate) << ", Tank size : " << allTanks[j].tankSize;
		//		if (abs(newRandomXPos - allTanks[j].xCoordinate) < allTanks[j].tankSize);
		//		{
		//			isTankOverlapping = true;
		//			newRandomXPos = (newRandomXPos + allTanks[j].tankSize) % SCREENSIZE_X;
		//			break;
		//		}
		//	}
		//}

		int newRandomYPos = floorHeight; //Random tank y coordinate
		int randomTankSize = 10 + rand() % 21; //Random Tank size from 10 to 20 pixels
		//int randomTankSize = 1; //Tank size is 1
		allTanks[i] = { newRandomXPos, newRandomYPos, randomTankSize };

		cout << "\nTank " << i + 1 << " of size " << randomTankSize << " pixels, spawned at coordinates (" << newRandomXPos << ", " << newRandomYPos << ").";
	}

	//Main game loop. Keeps looping until one tank is left alive.
	while (!glfwWindowShouldClose(openGLwindow))
	{
		glClearColor(1.0, 1.0, 1.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		if (deathCount >= numberOfTanks - 1)
		{
			break;
		}

		DrawFloor();

		// Draw Projectile
		if (isShooting)
		{
			CalculateProjectileMotion(0.01f, allTanks[currentPlayer], allTanks, numberOfTanks, allTanks[currentPlayer].angle, allTanks[currentPlayer].power);
			DrawProjectile(NormalizeCoordinates_X(projectilePositionX), NormalizeCoordinates_Y(projectilePositionY));
		}

		// Draw Tanks
		for (int i = 0; i < numberOfTanks; i++)
		{
			if (allTanks[i].isAlive)
			{
				DrawTank(NormalizeCoordinates_X(allTanks[i].xCoordinate), NormalizeCoordinates_Y(allTanks[i].yCoordinate), allTanks[i].tankSize, allTanks[i].angle);
			}
		}

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

	glfwTerminate();
	return 0;
}