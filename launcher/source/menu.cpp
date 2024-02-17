/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "launcher.h"

GuiTrigger Trigger[Triggers::Down + 1];

GuiImageData* Pointers[4];
GuiImage* Background;
GuiImageData* BackgroundImage;
GuiSound* Music;
GuiWindow* Window;
GuiText* Title;
GuiText* Subtitle;

extern "C" {
	extern u8 background2_png[];
}

static lwp_t guithread = LWP_THREAD_NULL;
static volatile bool guiHalt = true;
static volatile int exitRequested = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

void ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread(guithread);
}
void HaltGui()
{
	guiHalt = true;

	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}
static void* UpdateGUI(void* arg);
void InitGUIThreads()
{
	LWP_CreateThread(&guithread, UpdateGUI, NULL, NULL, 0, 70);
}
void RequestExit()
{
	exitRequested = 1;
}

static void* UpdateGUI(void* arg)
{
	int i;

	while (true) {
		if(guiHalt)
			LWP_SuspendThread(guithread);
		else {
			UpdatePads();
			Window->Draw();

			for (i = 3; i >= 0; i--) {
				if(userInput[i].wpad->ir.valid)
					Menu_DrawImg(userInput[i].wpad->ir.x - 48, userInput[i].wpad->ir.y - 48, 96, 96, Pointers[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
				DoRumble(i);
			}

			Menu_Render();

			for(i = 0; i < 4; i++)
				Window->Update(&userInput[i]);

			if (exitRequested) {
				for(i = 0; i < 255; i += 15) {
					Window->Draw();
					Menu_DrawRectangle(0, 0, screenwidth, screenheight, (GXColor){0, 0, 0, (u8)i}, 1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}

	return NULL;
}

void MainMenu(Menus::Enum menu)
{
	Pointers[0] = new GuiImageData(player1_point_png);
	Pointers[1] = new GuiImageData(player2_point_png);
	Pointers[2] = new GuiImageData(player3_point_png);
	Pointers[3] = new GuiImageData(player4_point_png);

	BackgroundImage = new GuiImageData(background2_png);

	Window = new GuiWindow(screenwidth, screenheight);
	Window->SetPosition(0, 0);
	Window->SetAlignment(ALIGN_LEFT, ALIGN_TOP);

	Background = new GuiImage(BackgroundImage);
	Background->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Background->SetPosition(0, 0);
	Window->SetFocus(true);
	Window->Append(Background);

	Title = new GuiText(RIIVOLUTION_TITLE, 32, (GXColor){255, 255, 255, 255});
	Title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Title->SetPosition(56, 32);
	// TODO: Title->SetItalic(true);
	Window->Append(Title);

	Subtitle = new GuiText("Loading... This might take a bit!", 18, (GXColor){255, 255, 255, 255});
	Subtitle->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle->SetPosition(227, 253);
	Window->Append(Subtitle);

	Trigger[Triggers::Select].SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	Trigger[Triggers::Back].SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);
	Trigger[Triggers::Home].SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME | WPAD_GUITAR_HERO_3_BUTTON_ORANGE, PAD_BUTTON_START);
	Trigger[Triggers::PageLeft].SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_MINUS, PAD_TRIGGER_L);
	Trigger[Triggers::PageRight].SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_PLUS, PAD_TRIGGER_R);

	ResumeGui();
	/*
	Music = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	Music->SetVolume(50);
	Music->Play();
	*/
	while (menu != Menus::Exit) {
		switch (menu) {
			case Menus::Main:
				menu = MenuMain();
				break;
			case Menus::Mount:
				menu = MenuMount();
				break;
			case Menus::Init:
				menu = MenuInit();
				break;
			case Menus::Options:
				menu = MenuSelectOptions();
				break;
			case Menus::Confirm:
				menu = MenuUpdateConfirm();
				break;
			case Menus::Accepted:
				menu = MenuConfirmination();
				break;
			case Menus::Settings:
				menu = MenuSettings();
				break;
			case Menus::Connect:
				menu = MenuConnect();
				break;
			case Menus::Launch:
				menu = MenuLaunch();
				break;
			default:
				break;
		}
	}

	ShutoffRumble();

	ResumeGui();
	RequestExit();
	while (true)
		usleep(THREAD_SLEEP);

	HaltGui();

	Music->Stop();
	delete Music;
	delete Background;
	delete Window;

	delete Pointers[0];
	delete Pointers[1];
	delete Pointers[2];
	delete Pointers[3];
}