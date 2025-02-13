//#include <GLFW/glfw3.h>
//#include <iostream>
//#include <cmath>
//#include <vector>
//#include <iostream>
//
//const int SCREEN_WIDTH = 800;
//const int SCREEN_HEIGHT = 600;
//const float GRAVITY = 9.8f;
//
//const float PI = 3.14;
//
//// Tank Structure
//struct Tank {
//	float x, y;
//	float size;
//	bool isAlive = true;
//};
//
//// Game Variables
//std::vector<Tank> tanks;
//Tank playerTank;
//float projectileX, projectileY, velocityX, velocityY;
//bool isShooting = false;
//
//// Convert Game Coordinates to OpenGL
//float NormalizeCoordinates_X(float x)
//{
//	return (x / SCREEN_WIDTH) * 2 - 1;
//}
//
//float NormalizeCoordinates_Y(float y)
//{
//	return 1.0f - (y / SCREEN_HEIGHT) * 2;
//}
//
//// Draw a Circle (Tank)
//void DrawTank(float cx, float cy, float r, int segments = 30) {
//	glBegin(GL_TRIANGLE_FAN);
//	glVertex2f(cx, cy);
//	for (int i = 0; i <= segments; i++) {
//		float theta = 2.0f * PI * i / segments;
//		float x = r * cos(theta);
//		float y = r * sin(theta);
//		glVertex2f(cx + x, cy + y);
//	}
//	glEnd();
//}
//
//// Draw the Projectile
//void DrawProjectile(float x, float y) {
//	glPointSize(5.0f);
//	glBegin(GL_POINTS);
//	glVertex2f(x, y);
//	glEnd();
//}
//
//// Update Projectile Motion
//void updateProjectile(float dt) {
//	if (!isShooting) return;
//
//	projectileX += velocityX * dt;
//	projectileY += velocityY * dt;
//	velocityY -= GRAVITY * dt; // Gravity
//
//	// Stop if projectile hits ground
//	if (projectileY <= -1.0f) {
//		isShooting = false;
//	}
//}
//
//// Handle Input (Press Space to Shoot)
//void keyboardInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//	if (action == GLFW_PRESS && key == GLFW_KEY_SPACE && !isShooting) {
//		float angle = 45.0f * PI / 180.0f; // Example fixed angle
//		float power = 50.0f;
//
//		projectileX = NormalizeCoordinates_X(playerTank.x);
//		projectileY = NormalizeCoordinates_Y(playerTank.y);
//		velocityX = cos(angle) * power * 0.005;
//		velocityY = sin(angle) * power * 0.005;
//		isShooting = true;
//	}
//}
//
//int main() {
//	// Initialize GLFW
//	if (!glfwInit()) return -1;
//
//	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tank Game", NULL, NULL);
//	if (!window) { glfwTerminate(); return -1; }
//	glfwMakeContextCurrent(window);
//	glfwSetKeyCallback(window, keyboardInputCallback);
//
//	// Spawn Tanks
//	playerTank = { 200, 100, 30 }; // Player Tank
//	tanks.push_back(playerTank);
//	tanks.push_back({ 600, 150, 30 });
//
//	// Main Game Loop
//	while (!glfwWindowShouldClose(window)) {
//		glClear(GL_COLOR_BUFFER_BIT);
//
//		// Draw Tanks
//		for (const Tank& tank : tanks) {
//			DrawTank(NormalizeCoordinates_X(tank.x), NormalizeCoordinates_Y(tank.y), 0.05f);
//		}
//
//		// Draw Projectile
//		if (isShooting) {
//			DrawProjectile(projectileX, projectileY);
//			updateProjectile(0.016f); // Assume 60 FPS
//		}
//
//		glfwSwapBuffers(window);
//		glfwPollEvents();
//	}
//
//	glfwTerminate();
//	return 0;
//}
