/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include <iostream>
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"

#include "Renderer.h"
#include "Scene.h"
#include "Hud.h"
#include "PlayerStatus.h"

const int WINDOW_SIZE_X = 1024;
const int WINDOW_SIZE_Y = 640;

Renderer* g_Renderer = NULL;
Scene g_Scene;
Hud g_Hud;
PlayerStatus g_Status;

bool g_MoveUp = false;
bool g_MoveDown = false;
bool g_MoveLeft = false;
bool g_MoveRight = false;
int g_LastTimeMs = 0;

// Pixels relative to the window center - the same space the player is drawn
// in, since the camera always centers on him.
float g_MouseX = 0.f;
float g_MouseY = 0.f;
bool g_FireLightHeld = false;
bool g_FireDarkHeld = false;

void RenderScene(void)
{
	glClearColor(0.045f, 0.043f, 0.055f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_Scene.Render(g_Renderer, g_Status, WINDOW_SIZE_X, WINDOW_SIZE_Y);
	g_Hud.Render(g_Renderer, g_Status, WINDOW_SIZE_X, WINDOW_SIZE_Y);
	g_Scene.RenderMinimap(g_Renderer, WINDOW_SIZE_X, WINDOW_SIZE_Y);

	glutSwapBuffers();
}

void Idle(void)
{
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (g_LastTimeMs == 0) ? 0.f : (nowMs - g_LastTimeMs) / 1000.f;
	g_LastTimeMs = nowMs;

	float screenRight = (g_MoveRight ? 1.f : 0.f) - (g_MoveLeft ? 1.f : 0.f);
	float screenUp = (g_MoveUp ? 1.f : 0.f) - (g_MoveDown ? 1.f : 0.f);
	g_Scene.Update(screenRight, screenUp, g_FireLightHeld, g_FireDarkHeld, g_MouseX, g_MouseY, deltaTime, g_Status);

	RenderScene();
}

void UpdateMousePosition(int x, int y)
{
	g_MouseX = (float)x - WINDOW_SIZE_X / 2.f;
	g_MouseY = WINDOW_SIZE_Y / 2.f - (float)y;
}

void MouseMotion(int x, int y)
{
	UpdateMousePosition(x, y);
}

void MousePassiveMotion(int x, int y)
{
	UpdateMousePosition(x, y);
}

void MouseInput(int button, int state, int x, int y)
{
	UpdateMousePosition(x, y);

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
	{
		g_FireLightHeld = (state == GLUT_DOWN);
		break;
	}
	case GLUT_RIGHT_BUTTON:
	{
		g_FireDarkHeld = (state == GLUT_DOWN);
		break;
	}
	}
}

void KeyInput(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
	case 'W':
	{
		g_MoveUp = true;
		break;
	}
	case 's':
	case 'S':
	{
		g_MoveDown = true;
		break;
	}
	case 'a':
	case 'A':
	{
		g_MoveLeft = true;
		break;
	}
	case 'd':
	case 'D':
	{
		g_MoveRight = true;
		break;
	}
	// Stand-in until combat can actually hurt the player some other way.
	case 'c':
	case 'C':
	{
		g_Status.Damage(12.f);
		break;
	}
	case 27:
	{
		glutLeaveMainLoop();
		break;
	}
	}
}

void KeyInputUp(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
	case 'W':
	{
		g_MoveUp = false;
		break;
	}
	case 's':
	case 'S':
	{
		g_MoveDown = false;
		break;
	}
	case 'a':
	case 'A':
	{
		g_MoveLeft = false;
		break;
	}
	case 'd':
	case 'D':
	{
		g_MoveRight = false;
		break;
	}
	}
}

void SpecialKeyInput(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
	{
		g_MoveUp = true;
		break;
	}
	case GLUT_KEY_DOWN:
	{
		g_MoveDown = true;
		break;
	}
	case GLUT_KEY_LEFT:
	{
		g_MoveLeft = true;
		break;
	}
	case GLUT_KEY_RIGHT:
	{
		g_MoveRight = true;
		break;
	}
	}
}

void SpecialKeyInputUp(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
	{
		g_MoveUp = false;
		break;
	}
	case GLUT_KEY_DOWN:
	{
		g_MoveDown = false;
		break;
	}
	case GLUT_KEY_LEFT:
	{
		g_MoveLeft = false;
		break;
	}
	case GLUT_KEY_RIGHT:
	{
		g_MoveRight = false;
		break;
	}
	}
}

int main(int argc, char **argv)
{
	// Initialize GL things
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WINDOW_SIZE_X, WINDOW_SIZE_Y);
	glutCreateWindow("The Darkest Light - Prototype");

	glewInit();
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		std::cout << "GLEW Version is 3.0\n";
	}
	else
	{
		std::cout << "GLEW 3.0 not supported\n";
	}

	g_Renderer = new Renderer(WINDOW_SIZE_X, WINDOW_SIZE_Y);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized..\n";
	}

	std::cout << "WASD/Arrow keys: move   Left click: light bolt   Right click: dark bolt   C: take damage   Esc: quit\n";

	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyInput);
	glutKeyboardUpFunc(KeyInputUp);
	glutMouseFunc(MouseInput);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(MousePassiveMotion);
	glutSpecialFunc(SpecialKeyInput);
	glutSpecialUpFunc(SpecialKeyInputUp);

	glutMainLoop();

	delete g_Renderer;

	return 0;
}
