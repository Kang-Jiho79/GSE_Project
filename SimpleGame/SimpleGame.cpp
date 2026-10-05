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
#include "Palette.h"
#include "PostProcess.h"
#include "TextRenderer.h"
#include "WorldMapScreen.h"
#include "TemplePanel.h"
#include "DialogueBox.h"
#include "DialogueLog.h"
#include "TitleScreen.h"
#include "NameEntryScreen.h"
#include "SaveGame.h"
#include "EndingScreen.h"
#include "Growth.h"
#include <string>
#include "Regions.h"
#include "Progress.h"

const int WINDOW_SIZE_X = 1024;
const int WINDOW_SIZE_Y = 640;

Renderer* g_Renderer = NULL;
PostProcess* g_PostProcess = NULL;
TextRenderer* g_Text = NULL;
PlayerStatus g_Status;
Progress g_Progress;
Scene g_Scene(g_Progress);
Hud g_Hud;
WorldMapScreen g_WorldMap;
TemplePanel g_TemplePanel;
DialogueBox g_Dialogue;
DialogueLog g_DialogueLog;
TitleScreen g_Title;
NameEntryScreen g_NameEntry;
EndingScreen g_Ending;

// Real seconds since launch, for anything on a menu that breathes or blinks.
float g_MenuTime = 0.f;

// Outside the game proper: the title, naming him, or how it ended.
bool IsInMenu()
{
	return g_Title.IsOpen() || g_NameEntry.IsOpen() || g_Ending.IsOpen();
}

// A screen over the world that holds it still while it is open.
bool IsOverlayOpen()
{
	return IsInMenu() || g_WorldMap.IsOpen() || g_TemplePanel.IsOpen() || g_Dialogue.IsOpen() || g_DialogueLog.IsOpen();
}

bool g_MoveUp = false;
bool g_MoveDown = false;
bool g_MoveLeft = false;
bool g_MoveRight = false;
bool g_AttackHeld = false;
bool g_RunHeld = false;

// What was last written to the save, so an autosave happens only when there
// is something new: another region, or another step in the story.
// The last upgrade said aloud, so each new one is announced once.
int g_SeenUpgrades = 0;

int g_SavedRegion = -1;
int g_SavedMilestones = -1;

// What the newest upgrade brings, across the top.
void AnnounceGrowth()
{
	if (g_Status.upgrades <= g_SeenUpgrades)
	{
		return;
	}
	g_SeenUpgrades = g_Status.upgrades;
	const char* gift = Growth::GiftOf(g_Status.upgrades);
	std::string notice = "강화 " + std::to_string(g_Status.upgrades);
	if (gift != nullptr)
	{
		notice += "  ·  ";
		notice += gift;
	}
	g_Hud.Notify(notice);
}

void MarkSaved()
{
	g_SavedRegion = g_Scene.CurrentRegion();
	g_SavedMilestones = SaveGame::Milestones(g_Progress);
}

void SaveJourney(const char* notice)
{
	if (SaveGame::Write(g_Status, g_Progress, g_Scene.CurrentRegion()))
	{
		MarkSaved();
		g_Hud.Notify(notice);
	}
	else
	{
		g_Hud.Notify("기록하지 못했다");
	}
}

// Saved on its own whenever he reaches another region or the story moves on -
// but never mid-conversation, mid-duel, or fallen.
void AutoSave()
{
	if (IsOverlayOpen() || g_Status.formLocked || g_Status.health <= 0.f || g_Scene.HoldsSave())
	{
		return;
	}
	if (g_Scene.CurrentRegion() == g_SavedRegion && SaveGame::Milestones(g_Progress) == g_SavedMilestones)
	{
		return;
	}
	SaveJourney("자동 기록");
}

void ResetInput()
{
	g_MoveUp = false;
	g_MoveDown = false;
	g_MoveLeft = false;
	g_MoveRight = false;
	g_AttackHeld = false;
}

// From the beginning, under the name he chose. Nothing is saved until the
// story first moves on, so quitting at once leaves an old save as it was.
void StartNewGame(const std::string& name)
{
	g_Status = PlayerStatus();
	g_Progress = Progress();
	g_Progress.heroName = name;
	g_DialogueLog.Clear();
	g_Dialogue.Close();
	g_NameEntry.Close();
	g_Title.Close();
	ResetInput();

	g_Scene.NewGame();
	MarkSaved();
	g_SeenUpgrades = g_Status.upgrades;
}

void ContinueGame()
{
	PlayerStatus status;
	Progress progress;
	int region;
	if (!SaveGame::Read(status, progress, region))
	{
		g_Title.Open(SaveGame::Exists(), SaveGame::EndingsSeen());
		g_Hud.Notify("기록을 읽지 못했다");
		return;
	}

	g_Status = status;
	g_Progress = progress;
	g_DialogueLog.Clear();
	g_Dialogue.Close();
	g_Title.Close();
	ResetInput();

	g_Scene.Continue(region);
	MarkSaved();
	g_SeenUpgrades = g_Status.upgrades;
}

// On through the ending, and back to the title at the end of it.
void AdvanceEnding()
{
	g_Ending.Advance();
	if (g_Ending.IsFinished())
	{
		g_Ending.Close();
		g_Title.Open(SaveGame::Exists(), SaveGame::EndingsSeen());
	}
}

void ConfirmTitle()
{
	switch (g_Title.Confirm())
	{
	case TitleScreen::Pick::NewGame:
	{
		g_Title.Close();
		g_NameEntry.Open(SaveGame::Exists());
		break;
	}
	case TitleScreen::Pick::Continue:
	{
		ContinueGame();
		break;
	}
	case TitleScreen::Pick::Quit:
	{
		glutLeaveMainLoop();
		break;
	}
	default:
	{
		break;
	}
	}
}

// Everything but a conversation, which only ends by being read through.
void CloseOverlays()
{
	g_WorldMap.Close();
	g_TemplePanel.Close();
	g_DialogueLog.Close();
}

// Keys while a screen is open. A conversation takes F, Space and Enter to go
// on and W/S to choose; the log takes W/S to look back; any other screen
// closes on F or Esc.
void OverlayKey(int lowered)
{
	// The ending reads on with F, Space or Enter.
	if (g_Ending.IsOpen())
	{
		if (lowered == InputMap::kInteract || lowered == InputMap::kSwitchForm || lowered == InputMap::kConfirm)
		{
			AdvanceEnding();
		}
		return;
	}

	if (g_Title.IsOpen())
	{
		switch (lowered)
		{
		case InputMap::kMoveUp:
		{
			g_Title.Move(-1);
			break;
		}
		case InputMap::kMoveDown:
		{
			g_Title.Move(1);
			break;
		}
		case InputMap::kInteract:
		case InputMap::kSwitchForm:
		case InputMap::kConfirm:
		{
			ConfirmTitle();
			break;
		}
		case InputMap::kQuit:
		{
			glutLeaveMainLoop();
			break;
		}
		}
		return;
	}

	if (g_Dialogue.IsOpen())
	{
		switch (lowered)
		{
		case InputMap::kInteract:
		case InputMap::kSwitchForm:
		case InputMap::kConfirm:
		{
			g_Dialogue.Advance(g_Progress, g_DialogueLog);
			break;
		}
		case InputMap::kMoveUp:
		{
			g_Dialogue.MoveChoice(-1);
			break;
		}
		case InputMap::kMoveDown:
		{
			g_Dialogue.MoveChoice(1);
			break;
		}
		}
		return;
	}

	if (g_DialogueLog.IsOpen())
	{
		switch (lowered)
		{
		case InputMap::kMoveUp:
		{
			g_DialogueLog.Scroll(1);
			break;
		}
		case InputMap::kMoveDown:
		{
			g_DialogueLog.Scroll(-1);
			break;
		}
		case InputMap::kDialogueLog:
		case InputMap::kInteract:
		case InputMap::kQuit:
		{
			g_DialogueLog.Close();
			break;
		}
		}
		return;
	}

	if (lowered == InputMap::kInteract || lowered == InputMap::kQuit)
	{
		CloseOverlays();
	}
}

int g_LastTimeMs = 0;

// Pixels relative to the window center - the same space the player is drawn
// in, since the camera always centers on him.
float g_MouseX = 0.f;
float g_MouseY = 0.f;

// Set by the key callback, consumed by the next frame, so a press is seen
// exactly once however long the key is held.
bool g_SwitchFormPressed = false;
bool g_DodgePressed = false;
bool g_SkillPressed = false;
bool g_SpecialPressed = false;
bool g_InteractPressed = false;

void RenderScene(void)
{
	g_Renderer->ResetCounts();

	// The region sets the dark behind the world and the grade over it.
	const Regions::Atmosphere& mood = g_Scene.CurrentAtmosphere();
	PostProcess::Grade grade;
	grade.saturation = mood.saturation;
	grade.contrast = mood.contrast;
	grade.tintR = mood.tint.r;
	grade.tintG = mood.tint.g;
	grade.tintB = mood.tint.b;
	g_PostProcess->SetGrade(grade);

	g_PostProcess->BeginScene(mood.fog.r, mood.fog.g, mood.fog.b);
	g_Scene.Render(g_Renderer, g_Status);

	// The world has to reach the GPU before it can be post-processed.
	g_Renderer->EndFrame();
	{
		ProfileScope scope("post");
		g_PostProcess->EndScene();
	}

	// On the menus the village shows behind them, but nothing of the game.
	if (!IsInMenu())
	{
		ProfileScope scope("hud");
		g_Hud.Render(g_Renderer, g_Text, g_Status, WINDOW_SIZE_X, WINDOW_SIZE_Y);
		g_Scene.RenderMinimap(g_Renderer);
		g_Scene.RenderBossBar(g_Renderer, g_Text);
		g_Scene.RenderText(g_Text);

		g_Text->Draw(TextRenderer::Face::Small,
			"WASD 이동 · Shift 달리기 · 좌클릭 공격 · 우클릭 구르기 · Space 폼 전환 · E/Q 스킬 · F 상호작용 · L 대화 기록",
			0.f, -WINDOW_SIZE_Y / 2.f + 12.f, Palette::kUiBone, 0.6f, TextRenderer::Align::Center);
	}

	// present: the final flushes (the HUD's shapes, then its text on top; the
	// world map, if open, over all of it the same way) plus the buffer swap.
	// With vsync on, the swap will dominate this number.
	{
		ProfileScope scope("present");
		g_Renderer->EndFrame();
		g_Text->Flush();

		if (IsOverlayOpen())
		{
			g_WorldMap.Render(g_Renderer, g_Text, WINDOW_SIZE_X, WINDOW_SIZE_Y, g_MouseX, g_MouseY);
			g_TemplePanel.Render(g_Renderer, g_Text, g_Status, WINDOW_SIZE_X, WINDOW_SIZE_Y);
			g_Dialogue.Render(g_Renderer, g_Text, g_Progress, WINDOW_SIZE_X, WINDOW_SIZE_Y);
			g_DialogueLog.Render(g_Renderer, g_Text, WINDOW_SIZE_X, WINDOW_SIZE_Y);
			g_Title.Render(g_Renderer, g_Text, WINDOW_SIZE_X, WINDOW_SIZE_Y, g_MenuTime);
			g_NameEntry.Render(g_Renderer, g_Text, WINDOW_SIZE_X, WINDOW_SIZE_Y, g_MenuTime);
			g_Ending.Render(g_Renderer, g_Text, WINDOW_SIZE_X, WINDOW_SIZE_Y);
			g_Renderer->EndFrame();
			g_Text->Flush();
		}

		glutSwapBuffers();
	}
}

void Idle(void)
{
	int nowMs = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (g_LastTimeMs == 0) ? 0.f : (nowMs - g_LastTimeMs) / 1000.f;
	g_LastTimeMs = nowMs;
	g_MenuTime += deltaTime;

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

	bool interact = g_InteractPressed;

	g_SwitchFormPressed = false;
	g_DodgePressed = false;
	g_SkillPressed = false;
	g_SpecialPressed = false;
	g_InteractPressed = false;

	// The world holds still while a screen is open over it.
	if (!IsOverlayOpen())
	{
		g_Scene.Update(input, deltaTime, g_Status);

		// The story has ended.
		int ending = g_Scene.TakeEnding();
		if (ending >= 0)
		{
			bool firstTime = !SaveGame::HasSeenEnding(ending);
			SaveGame::RecordEnding(ending);
			g_Ending.Open(ending, firstTime);
			ResetInput();
			interact = false;
		}

		// The story wants a word.
		int conversation = g_Scene.TakeConversation();
		if (conversation >= 0)
		{
			g_Dialogue.Open(conversation, g_Progress);
			g_AttackHeld = false;
			interact = false;
		}

		if (interact)
		{
			switch (g_Scene.AvailableInteraction())
			{
			case Scene::Interaction::WorldMap:
			{
				g_WorldMap.Open(g_Scene.CurrentRegion(), g_Scene.VisitedRegions());
				g_AttackHeld = false;
				break;
			}
			case Scene::Interaction::Temple:
			{
				const Regions::Info& region = Regions::Get(g_Scene.CurrentRegion());
				g_Scene.UseTemple(g_Status);
				SaveJourney("여정을 기록했다");
				g_TemplePanel.Open(region.name, region.style == Regions::Style::Ashen);
				g_AttackHeld = false;
				break;
			}
			case Scene::Interaction::Duel:
			{
				g_Scene.BeginDuel(g_Status);
				break;
			}
			case Scene::Interaction::Talk:
			{
				g_Dialogue.Open(g_Scene.TalkScript(), g_Progress);
				g_AttackHeld = false;
				break;
			}
			default:
			{
				break;
			}
			}
		}
	}
	AnnounceGrowth();
	AutoSave();
	g_Hud.Tick(deltaTime);
	g_Dialogue.Update(deltaTime);
	g_Ending.Update(deltaTime);

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

	// With a screen open the mouse belongs to it. In a conversation a click
	// goes on, or takes the choice under it; on the map, a click on a place
	// he can reach sets out for it; anything else is ignored.
	if (IsOverlayOpen())
	{
		if (g_Ending.IsOpen())
		{
			if (button == InputMap::kAttackButton && state == GLUT_DOWN)
			{
				AdvanceEnding();
			}
			return;
		}
		if (g_Title.IsOpen())
		{
			if (button == InputMap::kAttackButton && state == GLUT_DOWN && g_Title.SelectAt(g_MouseX, g_MouseY))
			{
				ConfirmTitle();
			}
			return;
		}
		if (g_Dialogue.IsOpen())
		{
			if (button == InputMap::kAttackButton && state == GLUT_DOWN
				&& (!g_Dialogue.IsChoosing() || g_Dialogue.SelectChoiceAt(g_MouseX, g_MouseY, WINDOW_SIZE_Y)))
			{
				g_Dialogue.Advance(g_Progress, g_DialogueLog);
			}
			return;
		}
		if (!g_WorldMap.IsOpen())
		{
			return;
		}

		if (button == InputMap::kAttackButton && state == GLUT_DOWN)
		{
			int region = g_WorldMap.RegionAt(g_MouseX, g_MouseY);
			if (region >= 0)
			{
				g_WorldMap.Close();
				g_Scene.BeginTravel(region);
			}
		}
		return;
	}

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
	// Naming him takes the keys as typed - Shift makes the doubled jamo -
	// and Esc goes back to the title.
	if (g_NameEntry.IsOpen())
	{
		if (key == InputMap::kQuit)
		{
			g_NameEntry.Close();
			g_Title.Open(SaveGame::Exists(), SaveGame::EndingsSeen());
		}
		else if (g_NameEntry.Key(key))
		{
			StartNewGame(g_NameEntry.Name());
		}
		return;
	}

	// With a screen open, nothing reaches the game.
	if (IsOverlayOpen())
	{
		OverlayKey(tolower(key));
		return;
	}

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
	case InputMap::kInteract:
	{
		g_InteractPressed = true;
		break;
	}
	case InputMap::kSpecial:
	{
		g_SpecialPressed = true;
		break;
	}
	case InputMap::kDebugDamage:
	{
		g_Status.TryDamage(12.f);
		break;
	}
	// One kill's worth - a dying monster scatters five motes of one each.
	case InputMap::kDebugAddLight:
	{
		g_Status.AddLight(5.f);
		break;
	}
	case InputMap::kDebugAddDark:
	{
		g_Status.AddDark(5.f);
		break;
	}
	case InputMap::kDebugToLumen:
	{
		g_Scene.BeginTravel(Regions::Lumen);
		break;
	}
	case InputMap::kDebugToTenebrae:
	{
		g_Scene.BeginTravel(Regions::Tenebrae);
		break;
	}
	case InputMap::kDebugEscapeGod:
	{
		bool& escaped = g_Progress.story[Story::EscapedGodInfluence];
		escaped = !escaped;
		g_Hud.Notify(escaped ? "신의 영향 이탈 켬" : "신의 영향 이탈 끔");
		break;
	}
	case InputMap::kDebugOpenWorld:
	{
		for (int i = 0; i < Regions::Count; i++)
		{
			g_Progress.visited[i] = true;
		}
		g_Progress.recognizedByLucius = true;
		g_Progress.recognizedByNoctis = true;
		g_Status.lightBlessing = true;
		g_Status.darkBlessing = true;
		g_Hud.Notify("모든 지역 개방 · 두 왕 인정");
		break;
	}
	case InputMap::kDebugAddBoth:
	{
		g_Status.AddLight(50.f);
		g_Status.AddDark(50.f);
		g_Hud.Notify("빛 +50 · 어둠 +50");
		break;
	}
	case InputMap::kDebugAsh:
	{
		g_Status.ashUnlocked = !g_Status.ashUnlocked;
		if (!g_Status.ashUnlocked && g_Status.form == Form::Ash)
		{
			g_Status.form = Form::Light;
		}
		g_Hud.Notify(g_Status.ashUnlocked ? "잿빛폼 켬" : "잿빛폼 끔");
		break;
	}
	case InputMap::kDebugToRealm:
	{
		g_Progress.Set(Story::AcceptAsh);
		g_Status.ashUnlocked = true;
		g_Scene.BeginTravel(Regions::Realm);
		break;
	}
	case InputMap::kDialogueLog:
	{
		g_DialogueLog.Open();
		g_AttackHeld = false;
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
	// The arrows stand in for W and S on a screen that takes them.
	if (IsOverlayOpen())
	{
		if (key == InputMap::kMenuUp)
		{
			OverlayKey(InputMap::kMoveUp);
		}
		else if (key == InputMap::kMenuDown)
		{
			OverlayKey(InputMap::kMoveDown);
		}
	}

	switch (key)
	{
	case InputMap::kTogglePostProcess:
	{
		g_PostProcess->SetEnabled(!g_PostProcess->IsEnabled());
		g_Hud.Notify(g_PostProcess->IsEnabled() ? "후처리 켬" : "후처리 끔");
		break;
	}
	case InputMap::kToggleOutline:
	{
		g_PostProcess->SetOutlineEnabled(!g_PostProcess->IsOutlineEnabled());
		g_Hud.Notify(g_PostProcess->IsOutlineEnabled() ? "외곽선 켬" : "외곽선 끔");
		break;
	}
	case InputMap::kToggleEffects:
	{
		g_Scene.SetEffectsEnabled(!g_Scene.AreEffectsEnabled());
		g_Hud.Notify(g_Scene.AreEffectsEnabled() ? "타격 효과 켬" : "타격 효과 끔");
		break;
	}
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

	g_PostProcess = new PostProcess();
	g_PostProcess->Initialize(WINDOW_SIZE_X, WINDOW_SIZE_Y);

	g_Text = new TextRenderer();
	g_Text->Initialize(WINDOW_SIZE_X, WINDOW_SIZE_Y);

	g_Scene.SetViewport(WINDOW_SIZE_X, WINDOW_SIZE_Y);
	// The hidden village, behind the title.
	g_Scene.NewGame();
	g_Title.Open(SaveGame::Exists(), SaveGame::EndingsSeen());

	std::cout << "WASD: move   Shift: run   Left click: attack   Right click: dodge   Space: switch form   E/Q: skills   C: take damage   1/2: +light/+dark   F1: post-processing   F2: outlines   F3: hit effects   Esc: quit\n";

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

	delete g_Text;
	delete g_PostProcess;
	delete g_Renderer;

	return 0;
}
