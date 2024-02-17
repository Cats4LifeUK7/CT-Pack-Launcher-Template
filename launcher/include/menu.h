#pragma once

#include <ogcsys.h>
#include "libwiigui/gui.h"
#include "menu.h"
#define THREAD_SLEEP 100

#define RIIVOLUTION_TITLE ""

namespace Menus { enum Enum
{
	Exit = 0,
	Mount,
	Init,
	Main,
	Options,
	Confirm,
	Accepted,
	Settings,
	Connect,
	Launch,
}; }

namespace Triggers { enum Enum
{
	Select = 0,
	Back,
	Home,
	PageLeft,
	PageRight,
	Left,
	Right,
	Up,
	Down
}; }

void InitGUIThreads();
void MainMenu(Menus::Enum menu);

void ResumeGui();
void HaltGui();

Menus::Enum MenuMount();
Menus::Enum MenuInit();
Menus::Enum MenuMain();
Menus::Enum MenuCredits();
Menus::Enum MenuUpdateConfirm();
Menus::Enum MenuConfirmination();
Menus::Enum MenuSelectOptions();
Menus::Enum MenuLaunch();
Menus::Enum MenuSettings();
Menus::Enum MenuConnect();

extern GuiTrigger Trigger[];

extern GuiImageData* Pointers[];
extern GuiImageData* BackgroundImage;
extern GuiImage* Background;
extern GuiSound* Music;
extern GuiWindow* Window;
extern GuiText* Title;
extern GuiText* TimeText;
extern GuiText* ProgressText;
extern GuiText* Subtitle;