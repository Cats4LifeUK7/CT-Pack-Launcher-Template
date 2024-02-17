#include "menu.h"
#include "launcher.h"
#include "haxx.h"
#include "riivolution_config.h"
#include "init.h"
#include "update.h"
#include <unistd.h>
#include <files.h>
#include <time.h>

using std::vector;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define OPTIONS_PER_PAGE 12
#define OPTION_FONT_SIZE 45
#define OPTION_FONT_HEIGHT 28
#define OPTION_ARROW_OFFSET 0

GuiText* TimeText;
GuiText* ProgressText;

static volatile int exitRequested2 = 0;

using std::string;

struct Page {
	string Name;
	vector<RiiOption*> Options;
};

RiiDisc Disc;
vector<int> Mounted;
vector<int> ToMount;

static vector<Page> Pages;


extern "C" {
	extern u8 arrow_left_png[];
	extern u8 arrow_active_left_png[];
	extern u8 arrow_right_png[];
	extern u8 arrow_active_right_png[];
	extern u8 optionsover_png[];
	extern u8 optionsover2_png[];
	extern u8 exitover_png[];
	extern u8 exitover2_png[];
	extern u8 launchover_png[];
	extern u8 launchover2_png[];
	extern u8 installover_png[];
	extern u8 installover2_png[];
	extern u8 uninstallover_png[];
	extern u8 uninstallover2_png[];
	extern u8 updateover_png[];
	extern u8 updateover2_png[];
	extern u8 back_png[];
	extern u8 backover_png[];
	extern u8 updateconfirm_png[];
	extern u8 yes_png[];
	extern u8 no_png[];
	extern u8 yesselect_png[];
	extern u8 noselect_png[];
	extern u8 updating_png[];
	extern u8 credit1_png[];
	extern u8 credit2_png[];
	extern u8 credit3_png[];
	extern u8 credit4_png[];
	extern u8 credit5_png[];
	extern u8 rebooting_png[];
}

#define UNSELECT_ALL() { \
	for (int unselect = Window->GetSelected(); unselect >= 0; unselect = Window->GetSelected()) \
		Window->GetGuiElementAt(unselect)->ResetState(); \
}

struct PageViewer {
	u32 PageNumber;
	Page* Current;

	GuiText** Title;
	GuiText** ChoiceText;
	GuiText** ChoiceOverText;
	GuiButton** Choice;
	GuiImage** LeftArrowImage;
	GuiImage** LeftArrowOverImage;
	GuiImage** RightArrowImage;
	GuiImage** RightArrowOverImage;
	GuiButton** LeftArrow;
	GuiButton** RightArrow;

	GuiText* PageText;
	GuiText* PageNumberText;
	GuiImage* LeftButtonImage;
	GuiImage* LeftButtonOverImage;
	GuiImage* RightButtonImage;
	GuiImage* RightButtonOverImage;
	GuiButton* LeftButton;
	GuiButton* RightButton;

	GuiImageData* LeftArrowImageData;
	GuiImageData* LeftArrowOverImageData;
	GuiImageData* RightArrowImageData;
	GuiImageData* RightArrowOverImageData;

	PageViewer()
	{
		Current = NULL;

		Subtitle->SetText(RIIVOLUTION_TITLE);

		LeftArrowImageData = new GuiImageData(arrow_left_png);
		LeftArrowOverImageData = new GuiImageData(arrow_active_left_png);
		RightArrowImageData = new GuiImageData(arrow_right_png);
		RightArrowOverImageData = new GuiImageData(arrow_active_right_png);

		if (Pages.size() > 1) {
			PageText = new GuiText("", 18, (GXColor){255, 255, 255, 255});
			PageText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			PageText->SetPosition(372, 67);
			Window->Append(PageText);

			PageNumberText = new GuiText(" ", 26, (GXColor){255, 255, 255, 255});
			PageNumberText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			PageNumberText->SetPosition(400 + 67 / 2, 48);
			Window->Append(PageNumberText);

			LeftButtonImage = new GuiImage(LeftArrowImageData);
			LeftButtonOverImage = new GuiImage(LeftArrowOverImageData);
			RightButtonImage = new GuiImage(RightArrowImageData);
			RightButtonOverImage = new GuiImage(RightArrowOverImageData);

			LeftButton = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight());
			LeftButton->SetImage(LeftButtonImage);
			LeftButton->SetImageOver(LeftButtonOverImage);
			LeftButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			LeftButton->SetPosition(419, 72);
			LeftButton->SetTrigger(&Trigger[Triggers::Select]);
			LeftButton->SetTrigger(&Trigger[Triggers::PageLeft]);
			Window->Append(LeftButton);

			RightButton = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight());
			RightButton->SetImage(RightButtonImage);
			RightButton->SetImageOver(RightButtonOverImage);
			RightButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			RightButton->SetPosition(604, 72);
			RightButton->SetTrigger(&Trigger[Triggers::Select]);
			RightButton->SetTrigger(&Trigger[Triggers::PageRight]);
			Window->Append(RightButton);
		}

		if (Pages.size())
			SetPage(0);
	}

	~PageViewer()
	{
		if (Pages.size() > 1) {
			Window->Remove(RightButton);
			Window->Remove(LeftButton);
			Window->Remove(PageText);
			Window->Remove(PageNumberText);

			delete RightButton;
			delete LeftButton;
			delete RightButtonOverImage;
			delete RightButtonImage;
			delete LeftButtonOverImage;
			delete LeftButtonImage;
			delete PageNumberText;
			delete PageText;
			delete LeftArrowImageData;
			delete LeftArrowOverImageData;
			delete RightArrowImageData;
			delete RightArrowOverImageData;
		}

		CleanPage();
	}

	const char* GetChoiceText(RiiOption* option)
	{
		if (option->Default > option->Choices.size())
			option->Default = 0; // NOTE: Should this really happen here? Probably not

		if (option->Default == 0)
			return "Disabled";

		return option->Choices[option->Default - 1].Name.c_str();
	}

	void CleanPage()
	{
		if (!Current)
			return;

		HaltGui();

		for (u32 i = 0; i < Current->Options.size(); i++) {
			Window->Remove(Title[i]);
			Window->Remove(Choice[i]);
			Window->Remove(LeftArrow[i]);
			Window->Remove(RightArrow[i]);

			delete Title[i];
			delete Choice[i];
			delete ChoiceText[i];
			delete ChoiceOverText[i];
			delete LeftArrowImage[i];
			delete LeftArrowOverImage[i];
			delete RightArrowImage[i];
			delete RightArrowOverImage[i];
			delete RightArrow[i];
			delete LeftArrow[i];
		}

		delete[] Title;
		delete[] ChoiceText;
		delete[] ChoiceOverText;
		delete[] Choice;
		delete[] LeftArrowImage;
		delete[] LeftArrowOverImage;
		delete[] RightArrowImage;
		delete[] RightArrowOverImage;
		delete[] LeftArrow;
		delete[] RightArrow;

		Current = NULL;

		ResumeGui();
	}

	void SetPage(u32 page)
	{
		CleanPage();

		if (page >= Pages.size())
			return;

		HaltGui();

		PageNumber = page;
		Current = &Pages[PageNumber];

		Subtitle->SetText(Current->Name.c_str());

		if (Pages.size() > 1) {
			char pagenum[0x10];
			sprintf(pagenum, "", PageNumber + 1);
			PageNumberText->SetText(pagenum);
		}

		int options = Current->Options.size();
		Title = new GuiText*[options];
		ChoiceText = new GuiText*[options];
		ChoiceOverText = new GuiText*[options];
		Choice = new GuiButton*[options];
		LeftArrowImage = new GuiImage*[options];
		LeftArrowOverImage = new GuiImage*[options];
		RightArrowImage = new GuiImage*[options];
		RightArrowOverImage = new GuiImage*[options];
		LeftArrow = new GuiButton*[options];
		RightArrow = new GuiButton*[options];


		u32 i = 0;
		for (vector<RiiOption*>::iterator iter = Current->Options.begin(); iter != Current->Options.end(); iter++, i++) {
			int y = 72 + i * OPTION_FONT_HEIGHT;
			RiiOption* option = *iter;

			GuiText* title = new GuiText(option->Name.c_str(), OPTION_FONT_SIZE, (GXColor){255, 255, 255, 255}); Title[i] = title;
			title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			title->SetPosition(39, y);
			Window->Append(title);

			GuiText* choiceText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){255, 255, 255, 255}); ChoiceText[i] = choiceText;
			choiceText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			choiceText->SetPosition(0, 0);
			GuiText* choiceOverText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){0, 140, 255, 255}); ChoiceOverText[i] = choiceOverText;
			choiceOverText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			choiceOverText->SetPosition(0, 0);

			GuiButton* choice = new GuiButton(152, OPTION_FONT_HEIGHT); Choice[i] = choice;
			choice->SetLabel(choiceText);
			choice->SetLabelOver(choiceOverText);
			choice->SetTrigger(&Trigger[Triggers::Select]);
			choice->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			choice->SetPosition(450, y);
			Window->Append(choice);

			GuiImage* leftArrowImage = new GuiImage(LeftArrowImageData); LeftArrowImage[i] = leftArrowImage;
			GuiImage* leftArrowOverImage = new GuiImage(LeftArrowOverImageData); LeftArrowOverImage[i] = leftArrowOverImage;
			GuiImage* rightArrowImage = new GuiImage(RightArrowImageData); RightArrowImage[i] = rightArrowImage;
			GuiImage* rightArrowOverImage = new GuiImage(RightArrowOverImageData); RightArrowOverImage[i] = rightArrowOverImage;

			GuiButton* leftArrow = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight()); LeftArrow[i] = leftArrow;
			leftArrow->SetImage(leftArrowImage);
			leftArrow->SetImageOver(leftArrowOverImage);
			leftArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			leftArrow->SetPosition(419, y + OPTION_ARROW_OFFSET);
			leftArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(leftArrow);

			GuiButton* rightArrow = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight()); RightArrow[i] = rightArrow;
			rightArrow->SetImage(rightArrowImage);
			rightArrow->SetImageOver(rightArrowOverImage);
			rightArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			rightArrow->SetPosition(604, y + OPTION_ARROW_OFFSET);
			rightArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(rightArrow);
		}

		ResumeGui();
	}

	void Update()
	{
		if (Pages.size() > 1) {
			if (LeftButton->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				LeftButton->SetState(STATE_SELECTED, LeftButton->GetStateChan());
				SetPage(Wrap(PageNumber - 1, Pages.size()));
			}
			if (RightButton->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				RightButton->SetState(STATE_SELECTED, RightButton->GetStateChan());
				SetPage(Wrap(PageNumber + 1, Pages.size()));
			}
		}

		if (!Current)
			return;

		for (u32 i = 0; i < Current->Options.size(); i++) {
			if (RightArrow[i]->GetState() == STATE_CLICKED || Choice[i]->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				if (RightArrow[i]->GetState() == STATE_CLICKED)
					RightArrow[i]->SetState(STATE_SELECTED, RightArrow[i]->GetStateChan());
				if (Choice[i]->GetState() == STATE_CLICKED)
					Choice[i]->SetState(STATE_SELECTED, Choice[i]->GetStateChan());
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default + 1, Current->Options[i]->Choices.size() + 1));
			}
			if (LeftArrow[i]->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				LeftArrow[i]->SetState(STATE_SELECTED, LeftArrow[i]->GetStateChan());
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default - 1, Current->Options[i]->Choices.size() + 1));
			}
		}
	}

	void SetOption(int index, RiiOption* option, u32 choice)
	{
		if (choice >= option->Choices.size() + 1)
			return;

		option->Default = choice;

		ChoiceText[index]->SetText(GetChoiceText(option));
		ChoiceOverText[index]->SetText(GetChoiceText(option));
	}

	int Wrap(int page, int pages)
	{
		if (page < 0)
			page = pages - 1;
		if (page >= pages)
			page = 0;

		return page;
	}
};

Menus::Enum MenuMount()
{

	Haxx_Mount(&Mounted);
	Launcher_ScrubPlaytimeEntry();

	while (!Mounted.size()) {
		HaltGui(); Subtitle->SetText("Insert SD/USB..."); ResumeGui();
		Haxx_Mount(&Mounted);
	}
	HaltGui(); Subtitle->SetText(""); ResumeGui();

	LauncherStatus::Enum status;

	do {
		if (RVL_Initialize() < 0 || (status = Launcher_Init()) == LauncherStatus::IosError) {
			HaltGui(); Subtitle->SetText("IOS Error!"); ResumeGui();
			status = LauncherStatus::IosError;
		}
	} while (status != LauncherStatus::OK);

	return Menus::Init;
}

Menus::Enum MenuInit()
{

	RVL_SetFST(NULL, 0);

	LauncherStatus::Enum status = LauncherStatus::NoDisc;
	HaltGui(); Subtitle->SetText(""); ResumeGui();

	do {
		status = Launcher_ReadDisc();
		switch (status) {
			case LauncherStatus::NoDisc:
				HaltGui(); Subtitle->SetText("Insert Disc..."); ResumeGui();
				break;
			case LauncherStatus::ReadError:
				HaltGui(); Subtitle->SetText("Disc Read Error!"); ResumeGui();
				sleep(3); // TODO: Repeatedly MENUINIT_CHECKBUTTONS during this sleep
				HaltGui(); Subtitle->SetText(""); ResumeGui();
				break;
			default:
				break;
		}
	} while (status != LauncherStatus::OK);

	vector<RiiDisc> discs;
	for (vector<int>::iterator mount = Mounted.begin(); mount != Mounted.end(); mount++) {
		ParseXMLs(*mount, &discs);
	}
	for (vector<int>::iterator tomount = ToMount.begin(); tomount != ToMount.end(); tomount++) {
		bool found = false;
		for (vector<int>::iterator mount = Mounted.begin(); mount != Mounted.end(); mount++) {
			if (*tomount == *mount) {
				found = true;
				break;
			}
		}
		if (!found)
			Mounted.push_back(*tomount);
	}
	Disc = CombineDiscs(&discs);
	ParseConfigXMLs(&Disc);

	Launcher_RVL();

#if 0
	// TODO: Put this somewhere sensible, make auto-launching happen without showing the GUI
	int fd = File_Open("/mnt/isfs/title/00010001/52494956/data/disc.sys", O_RDONLY);
	if (fd>=0) {
		u64 old_disc=0;
		u64 *old_disc_buf = (u64*)memalign(32, 32);
		if (old_disc_buf) {
			memset(old_disc_buf, 0, sizeof(u64));
			File_Read(fd, old_disc_buf, sizeof(u64));
			old_disc = *old_disc_buf;
			free(old_disc_buf);
		}
		File_Close(fd);
		File_Delete("/mnt/isfs/title/00010001/52494956/data/disc.sys");
		if ((u32)old_disc == *(u32*)MEM_BASE)
			return Menus::Launch;
	}
#endif

	return Menus::Main;
}

#define PAGENUM(page, pagenum) { \
	char num[0x10]; sprintf(num, "", pagenum); \
	page.Name = section->Name + " [" + num + "]"; \
}

static void PreparePages()
{
	Pages.clear();
	for (vector<RiiSection>::iterator section = Disc.Sections.begin(); section != Disc.Sections.end(); section++) {
		Page page;
		int pagenum = 1;
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			if (page.Options.size() == OPTIONS_PER_PAGE) {
				PAGENUM(page, pagenum++);
				Pages.push_back(page);
				page = Page();
			}

			page.Options.push_back(&*option);
		}

		if (pagenum > 1) {
			PAGENUM(page, pagenum);
		} else
			page.Name = section->Name;
		Pages.push_back(page);
	}
}

#define BANNER_TICKET_PATH "/ticket/00010001/52494956.tik"
#define BANNER_CONTENT_PATH "/title/00010001/52494956"


Menus::Enum MenuConfirmination()
{
	GuiImageData* UpdateAcceptedTestaImageData;
	GuiImage* UpdateAcceptedTestaImage;
	UpdateAcceptedTestaImageData = new GuiImageData(updating_png);
	UpdateAcceptedTestaImage = new GuiImage(UpdateAcceptedTestaImageData);

	GuiImageData* UpdateDoneTestaImageData;
	GuiImage* UpdateDoneTestaImage;
	UpdateDoneTestaImageData = new GuiImageData(rebooting_png);
	UpdateDoneTestaImage = new GuiImage(UpdateDoneTestaImageData);

	GuiButton* UpdateAcceptedTesta = new GuiButton(UpdateAcceptedTestaImageData->GetWidth(), UpdateAcceptedTestaImageData->GetHeight());
	UpdateAcceptedTesta->SetImage(UpdateAcceptedTestaImage);
	UpdateAcceptedTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	UpdateAcceptedTesta->SetPosition(0, 0);
	Window->Append(UpdateAcceptedTesta);

	ProgressText = new GuiText("", 30, (GXColor){255, 255, 255, 255});
	ProgressText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	ProgressText->SetPosition(242, 307);
	Window->Append(ProgressText);

	UpdateIsConfirmed();
	sleep(3);

	Window->Remove(UpdateAcceptedTesta);
	GuiButton* UpdateDoneTesta = new GuiButton(UpdateDoneTestaImageData->GetWidth(), UpdateDoneTestaImageData->GetHeight());
	UpdateDoneTesta->SetImage(UpdateDoneTestaImage);
	UpdateDoneTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	UpdateDoneTesta->SetPosition(0, 0);
	Window->Append(UpdateDoneTesta);
	sleep(3);
	return Menus::Exit;
}

Menus::Enum MenuUpdateConfirm()
{
	HaltGui();

	GuiImageData* UpdateConfirmTestaImageData;
	GuiImageData* YesTestaImageData;
	GuiImageData* YesTestaOverImageData;
	GuiImageData* NoTestaImageData;
	GuiImageData* NoTestaOverImageData;

	GuiImage* UpdateConfirmTestaImage;
	GuiImage* YesTestaImage;
	GuiImage* YesTestaOverImage;
	GuiImage* NoTestaImage;
	GuiImage* NoTestaOverImage;

	UpdateConfirmTestaImageData = new GuiImageData(updateconfirm_png);
	YesTestaImageData = new GuiImageData(yes_png);
	YesTestaOverImageData = new GuiImageData(yesselect_png);
	NoTestaImageData = new GuiImageData(no_png);
	NoTestaOverImageData = new GuiImageData(noselect_png);

	UpdateConfirmTestaImage = new GuiImage(UpdateConfirmTestaImageData);
	NoTestaImage = new GuiImage(NoTestaImageData);
	NoTestaOverImage = new GuiImage(NoTestaOverImageData);
	YesTestaImage = new GuiImage(YesTestaImageData);
	YesTestaOverImage = new GuiImage(YesTestaOverImageData);

	GuiButton* UpdateConfirmTesta = new GuiButton(UpdateConfirmTestaImageData->GetWidth(), UpdateConfirmTestaImageData->GetHeight());
	UpdateConfirmTesta->SetImage(UpdateConfirmTestaImage);
	UpdateConfirmTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	UpdateConfirmTesta->SetPosition(0, 0);
	Window->Append(UpdateConfirmTesta);

	GuiButton* YesTesta = new GuiButton(YesTestaImageData->GetWidth(), YesTestaImageData->GetHeight());
	YesTesta->SetImage(YesTestaImage);
	YesTesta->SetImageOver(YesTestaOverImage);
	YesTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	YesTesta->SetPosition(127, 335);
	YesTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(YesTesta);

	GuiButton* NoTesta = new GuiButton(NoTestaImageData->GetWidth(), NoTestaImageData->GetHeight());
	NoTesta->SetImage(NoTestaImage);
	NoTesta->SetImageOver(NoTestaOverImage);
	NoTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	NoTesta->SetPosition(405, 335);
	NoTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(NoTesta);

	ResumeGui();

	while (true) {

		if (YesTesta->GetState() == STATE_CLICKED) {
			Window->Remove(NoTesta);
			Window->Remove(YesTesta);
			Window->Remove(UpdateConfirmTesta);
			delete NoTesta;
			delete YesTesta;
			delete UpdateConfirmTesta;
			return Menus::Accepted;
		} 
		else if (NoTesta->GetState() == STATE_CLICKED) {
			Window->Remove(NoTesta);
			Window->Remove(YesTesta);
			Window->Remove(UpdateConfirmTesta);
			delete NoTesta;
			delete YesTesta;
			delete UpdateConfirmTesta;
			return Menus::Main;
		} 

		CheckShutdown();

		if (!Launcher_DiscInserted()) {
			return Menus::Init;
		}
	}
}

Menus::Enum MenuMain()
{
	HaltGui();

	GuiImageData* OptionsTestaImageData;
	GuiImageData* OptionsTestaOverImageData;
	GuiImageData* UpdateTestaImageData;
	GuiImageData* UpdateTestaOverImageData;
	GuiImageData* LaunchTestaImageData;
	GuiImageData* LaunchTestaOverImageData;
	GuiImageData* ExitTestaImageData;
	GuiImageData* ExitTestaOverImageData;

	GuiImage* OptionsTestaImage;
	GuiImage* OptionsTestaOverImage;
	GuiImage* UpdateTestaImage;
	GuiImage* UpdateTestaOverImage;
	GuiImage* LaunchTestaImage;
	GuiImage* LaunchTestaOverImage;
	GuiImage* ExitTestaImage;
	GuiImage* ExitTestaOverImage;

	UpdateTestaImageData = new GuiImageData(updateover_png);
	UpdateTestaOverImageData = new GuiImageData(updateover2_png);
	LaunchTestaImageData = new GuiImageData(launchover_png);
	LaunchTestaOverImageData = new GuiImageData(launchover2_png);
	ExitTestaImageData = new GuiImageData(exitover_png);
	ExitTestaOverImageData = new GuiImageData(exitover2_png);
	OptionsTestaImageData = new GuiImageData(optionsover_png);
	OptionsTestaOverImageData = new GuiImageData(optionsover2_png);

	UpdateTestaImage = new GuiImage(UpdateTestaImageData);
	UpdateTestaOverImage = new GuiImage(UpdateTestaOverImageData);
	LaunchTestaImage = new GuiImage(LaunchTestaImageData);
	LaunchTestaOverImage = new GuiImage(LaunchTestaOverImageData);
	ExitTestaImage = new GuiImage(ExitTestaImageData);
	ExitTestaOverImage = new GuiImage(ExitTestaOverImageData);
	OptionsTestaImage = new GuiImage(OptionsTestaImageData);
	OptionsTestaOverImage = new GuiImage(OptionsTestaOverImageData);

	GuiButton* OptionsTesta = new GuiButton(OptionsTestaImageData->GetWidth(), OptionsTestaImageData->GetHeight());
	OptionsTesta->SetImage(OptionsTestaImage);
	OptionsTesta->SetImageOver(OptionsTestaOverImage);
	OptionsTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	OptionsTesta->SetPosition(348, 138);
	OptionsTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(OptionsTesta);

	GuiButton* UpdateTesta = new GuiButton(UpdateTestaImageData->GetWidth(), UpdateTestaImageData->GetHeight());
	UpdateTesta->SetImage(UpdateTestaImage);
	UpdateTesta->SetImageOver(UpdateTestaOverImage);
	UpdateTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	UpdateTesta->SetPosition(25, 271);
	UpdateTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(UpdateTesta);

	GuiButton* LaunchTesta = new GuiButton(LaunchTestaImageData->GetWidth(), LaunchTestaImageData->GetHeight());
	LaunchTesta->SetImage(LaunchTestaImage);
	LaunchTesta->SetImageOver(LaunchTestaOverImage);
	LaunchTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	LaunchTesta->SetTrigger(&Trigger[Triggers::Select]);
	LaunchTesta->SetPosition(25, 138);
	Window->Append(LaunchTesta);

	GuiButton* ExitTesta = new GuiButton(ExitTestaImageData->GetWidth(), ExitTestaImageData->GetHeight());
	ExitTesta->SetImage(ExitTestaImage);
	ExitTesta->SetImageOver(ExitTestaOverImage);
	ExitTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	ExitTesta->SetPosition(348, 271);
	ExitTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(ExitTesta);

	TimeText = new GuiText("", 35, (GXColor){255, 255, 255, 255});
	TimeText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	TimeText->SetPosition(18, 420);
	Window->Append(TimeText);

	ResumeGui();

	while(1) {

		time_t rawtime;
		struct tm * timeinfo;

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

        // Set the text in the GuiText object
        TimeText->SetText(asctime (timeinfo) );

		if (OptionsTesta->GetState() == STATE_CLICKED) {
			Window->Remove(OptionsTesta);
			Window->Remove(ExitTesta);
			Window->Remove(LaunchTesta);
			Window->Remove(UpdateTesta);
			Window->Remove(TimeText);
			delete OptionsTesta;
			delete ExitTesta;
			delete LaunchTesta;
			delete UpdateTesta;
			delete TimeText;
			return Menus::Options;
		} 
		if (LaunchTesta->GetState() == STATE_CLICKED) {
			return Menus::Launch;
		} 
		else if (UpdateTesta->GetState() == STATE_CLICKED) {
			Window->Remove(OptionsTesta);
			Window->Remove(ExitTesta);
			Window->Remove(LaunchTesta);
			Window->Remove(UpdateTesta);
			Window->Remove(TimeText);
			delete OptionsTesta;
			delete ExitTesta;
			delete LaunchTesta;
			delete UpdateTesta;
			delete TimeText;
			return Menus::Confirm;
		} 
		else if (ExitTesta->GetState() == STATE_CLICKED) {
			return Menus::Exit;
		} 

		CheckShutdown();

		if (!Launcher_DiscInserted()) {
			return Menus::Init;
		}
	}
	Window->Remove(ExitTesta);
	Window->Remove(LaunchTesta);
	Window->Remove(UpdateTesta);
	delete ExitTesta;
	delete LaunchTesta;
	delete UpdateTesta;
}

Menus::Enum MenuSelectOptions()
{

	HaltGui();

	GuiImageData* BackTestaImageData;
	GuiImageData* BackTestaOverImageData;
	GuiImage* BackTestaImage;
	GuiImage* BackTestaOverImage;

	BackTestaImageData = new GuiImageData(back_png);
	BackTestaOverImageData = new GuiImageData(backover_png);

	BackTestaImage = new GuiImage(BackTestaImageData);
	BackTestaOverImage = new GuiImage(BackTestaOverImageData);

	GuiButton* BackTesta = new GuiButton(BackTestaImageData->GetWidth(), BackTestaImageData->GetHeight());
	BackTesta->SetImage(BackTestaImage);
	BackTesta->SetImageOver(BackTestaOverImage);
	BackTesta->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	BackTesta->SetPosition(400, 338);
	BackTesta->SetTrigger(&Trigger[Triggers::Select]);
	Window->Append(BackTesta);

	ResumeGui();

	PreparePages();

	HaltGui();
	PageViewer viewer;
	ResumeGui();

	while (true) {
		viewer.Update();

		if (BackTesta->GetState() == STATE_CLICKED) {
			Window->Remove(BackTesta);
			delete BackTesta;
			return Menus::Main;
		} 

		CheckShutdown();

		if (!Launcher_DiscInserted()) {
			return Menus::Init;
		}
	}
	Window->Remove(BackTesta);
	delete BackTesta;
}

Menus::Enum MenuLaunch()
{
	HaltGui(); Subtitle->SetText("Loading..."); ResumeGui();

	usleep(1000);
	HaltGui();

	ShutoffRumble();

	SaveConfigXML(&Disc);

	RVL_Patch(&Disc);

	// Launcher_CommitRVL(true); // TODO: CommitRVL properly?

	Launcher_RunApploader();

	Launcher_CommitRVL(false);

	Launcher_AddPlaytimeEntry();

	Launcher_SetVideoMode();

	RVL_PatchMemory(&Disc);

	RVL_Unmount();

	if (File_GetLogFS()<0)
		File_Deinit();

	Launcher_Launch();

	return Menus::Exit;
}
