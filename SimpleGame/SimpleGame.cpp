/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include <iostream>
#include <cctype>
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"

#include "Renderer.h"
#include "Scene.h"
#include "Hud.h"
#include "PlayerStatus.h"
#include "Profiler.h"
#include "InputMap.h"

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
bool g_AttackHeld = false;
bool g_RunHeld = false;

// Set by the key callback, consumed by the next frame, so a press is seen
// exactly once however long the key is held.
bool g_SwitchFormPressed = false;
bool g_DodgePressed = false;
bool g_SkillPressed = false;
bool g_SpecialPressed = false;

void RenderScene(void)
{
	g_Renderer->ResetCounts();

	glClearColor(0.045f, 0.043f, 0.055f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_Scene.Render(g_Renderer, g_Status);

	{
		ProfileScope scope("hud");
		g_Hud.Render(g_Renderer, g_Status, WINDOW_SIZE_X, WINDOW_SIZE_Y);
		g_Scene.RenderMinimap(g_Renderer);
	}

	// present: the final flush (upload + instanced draw of the queued world)
	// plus the buffer swap. With vsync on, the swap will dominate this number.
	{
		ProfileScope scope("present");
		g_Renderer->EndFrame();
		glutSwapBuffers();
	}
}

void Idle(void)
{
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (g_LastTimeMs == 0) ? 0.f : (nowMs - g_LastTimeMs) / 1000.f;
	g_LastTimeMs = nowMs;

	InputState input;
	input.moveRight = (g_MoveRight ? 1.f : 0.f) - (g_MoveLeft ? 1.f : 0.f);
	input.moveUp = (g_MoveUp ? 1.f : 0.f) - (g_MoveDown ? 1.f : 0.f);
	input.attack = g_AttackHeld;
	input.run = g_RunHeld;
	input.switchForm = g_SwitchFormPressed;
	input.dodge = g_DodgePressed;
	input.skill = g_SkillPressed;
	input.special = g_SpecialPressed;
	input.aimX = g_MouseX;
	input.aimY = g_MouseY;

	g_SwitchFormPressed = false;
	g_DodgePressed = false;
	g_SkillPressed = false;
	g_SpecialPressed = false;

	g_Scene.Update(input, deltaTime, g_Status);

	RenderScene();

	Profiler::Get().AddFrame(deltaTime, g_Renderer->DrawCallCount(), g_Renderer->QuadCount());
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
	case InputMap::kAttackButton:
	{
		g_AttackHeld = (state == GLUT_DOWN);
		break;
	}
	case InputMap::kDodgeButton:
	{
		if (state == GLUT_DOWN)
		{
			g_DodgePressed = true;
		}
		break;
	}
	}
}

void KeyInput(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case InputMap::kMoveUp:
	{
		g_MoveUp = true;
		break;
	}
	case InputMap::kMoveDown:
	{
		g_MoveDown = true;
		break;
	}
	case InputMap::kMoveLeft:
	{
		g_MoveLeft = true;
		break;
	}
	case InputMap::kMoveRight:
	{
		g_MoveRight = true;
		break;
	}
	case InputMap::kSwitchForm:
	{
		g_SwitchFormPressed = true;
		break;
	}
	case InputMap::kSkill:
	{
		g_SkillPressed = true;
		break;
	}
	case InputMap::kSpecial:
	{
		g_SpecialPressed = true;
		break;
	}
	// Stand-in until combat can actually hurt the player some other way.
	case InputMap::kDebugDamage:
	{
		g_Status.TryDamage(12.f);
		break;
	}
	case InputMap::kQuit:
	{
		glutLeaveMainLoop();
		break;
	}
	}
}

void KeyInputUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case InputMap::kMoveUp:
	{
		g_MoveUp = false;
		break;
	}
	case InputMap::kMoveDown:
	{
		g_MoveDown = false;
		break;
	}
	case InputMap::kMoveLeft:
	{
		g_MoveLeft = false;
		break;
	}
	case InputMap::kMoveRight:
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
	case InputMap::kRunKeyLeft:
	case InputMap::kRunKeyRight:
	{
		g_RunHeld = true;
		break;
	}
	}
}

void SpecialKeyInputUp(int key, int x, int y)
{
	switch (key)
	{
	case InputMap::kRunKeyLeft:
	case InputMap::kRunKeyRight:
	{
		g_RunHeld = false;
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
	// Instanced drawing (glDrawArraysInstanced, glVertexAttribDivisor) is core
	// in 3.3, which the #version 330 shaders already require anyway.
	if (glewIsSupported("GL_VERSION_3_3"))
	{
		std::cout << "OpenGL 3.3 available\n";
	}
	else
	{
		std::cout << "OpenGL 3.3 not supported - instanced rendering will not work\n";
	}

	g_Renderer = new Renderer(WINDOW_SIZE_X, WINDOW_SIZE_Y);
	if (!g_Renderer->IsInitialized())
	{
		std::cout << "Renderer could not be initialized..\n";
	}

	g_Scene.SetViewport(WINDOW_SIZE_X, WINDOW_SIZE_Y);

	std::cout << "WASD: move   Shift: run   Left click: attack   Right click: dodge   Space: switch form   E/Q: skills   C: take damage   Esc: quit\n";

	glutDisplayFunc(RenderScene);
	glutIdleFunc(Idle);
	// Held keys are tracked by their down/up pair; repeats would only re-fire
	// single-press actions like the form switch.
	glutIgnoreKeyRepeat(1);
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
