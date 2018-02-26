/************************************************************************************

	AstroMenace (Hardcore 3D space shooter with spaceship upgrade possibilities)
	Copyright (c) 2006-2018 Mikhail Kurinnoi, Viewizard


	AstroMenace is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	AstroMenace is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with AstroMenace. If not, see <http://www.gnu.org/licenses/>.


	Web Site: http://www.viewizard.com/
	Project: https://github.com/viewizard/astromenace
	E-mail: viewizard@viewizard.com

*************************************************************************************/


#include "game.h"
#include <sys/stat.h> /* stat */


#if defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <pwd.h>
#endif // unix



//------------------------------------------------------------------------------------
// настройки
//------------------------------------------------------------------------------------
// настройки игры
GameSetup Setup;


//------------------------------------------------------------------------------------
// общие состояния и статусы
//------------------------------------------------------------------------------------
// текущий статус (текущее меню)
eGameStatus GameStatus;
// защелка на выход, когда нужно перегрузить, а когда просто поменять режим
bool Quit = false;
bool NeedReCreate = false;
// выходим или нет
bool CanQuit = true;
// для выбора в списке режимов разрешения
sVideoModes *VideoModes = nullptr;
int VideoModesNum = 0;
// текущие параметры десктопа
sVideoModes CurrentVideoMode;
// статус загрузки, должно быть ОбщееКол-во Миссий+1(для меню)
bool LoadedTypes[1000];
// данные для определения папки пользователя
#ifdef WIN32
#define SD_APPDATA                   0x001a
#define SD_DESKTOPDIRECTORY          0x0010        // <user name>\Desktop
typedef bool (WINAPI *SHGETSPECIALFOLDERPATH)(HWND hwndOwner, LPTSTR lpszPath, int nFolder, bool fCreate);
SHGETSPECIALFOLDERPATH pSHGetSpecialFolderPath = 0;
#endif // WIN32



//------------------------------------------------------------------------------------
// пути к файлам
//------------------------------------------------------------------------------------
// полное путь к программе
char ProgrammDir[MAX_PATH];
std::string VFSFileNamePath;
// полное имя для файла конфигурации игры
char ConfigFileName[MAX_PATH];
// для сохранения скриншотов
char ScreenshotDir[MAX_PATH];


//------------------------------------------------------------------------------------
// управление
//------------------------------------------------------------------------------------
// состояние кнопок мышки
bool SDL_MouseCurrentStatus[8];
// джойстик
SDL_Joystick *Joystick = nullptr;
// базовые параметры, или параметры блокировки управления (нет сигнала, нули)
int JoystickAxisX = 0;
int JoystickAxisY = 0;
// кнопки джойстика (текущего)
bool JoysticButtons[100];


//------------------------------------------------------------------------------------
// камера
//------------------------------------------------------------------------------------
// ближайшая точка, основная точка отсчета локальной системы координат игры
VECTOR3D GamePoint(0.0f, 0.0f, 0.0f);
// направление движения камеры
VECTOR3D GameCameraMovement(0.0f, 0.0f, 1.0f);


//------------------------------------------------------------------------------------
// графическая часть
//------------------------------------------------------------------------------------
// прорисовка динамической части курсора
eParticleSystem2D *CursorParticleSystem2D = nullptr;
// статические частицы - звезды на скайбоксе
CSpaceStars *psSpaceStatic = nullptr;






#ifdef fontconfig

#include <fontconfig/fontconfig.h>

int FontQuantity = 0;
sFontList *FontList = 0;


int InitFontConfig()
{
	FcPattern *pat;
	FcFontSet *fs;
	FcObjectSet *os;
	FcChar8 *data;
	FcConfig *config;
	FcLangSet *langset;

	if (!FcInit()) {
		fprintf(stderr, "Couldn't init FontConfig.\n");
		return -1;
	}
	config = FcConfigGetCurrent();
	FcConfigSetRescanInterval(config, 0);

	pat = FcPatternCreate();
	os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, FC_FONTFORMAT, (char *) 0);
	fs = FcFontList(config, pat, os);

	printf("\nTotal fonts installed: %d\n", fs->nfont);
	int AppropriateFontsCount = 0;

	// первый проход, считаем кол-во подходящих шрифтов
	for (int i = 0; fs && (i < fs->nfont); i++) {
		FcPattern *font = fs->fonts[i];


		// 1) Работаем только с TrueType, CFF и Type 1
		if (FcPatternGetString(font, FC_FONTFORMAT, 0, &data) == FcResultMatch) {
			if (strcmp((const char *)data, "TrueType") &&
			    strcmp((const char *)data, "CFF") &&
			    strcmp((const char *)data, "Type 1")) {
				continue;
			}
		} else
			continue;

		// 2) берем только болд
		if (FcPatternGetString(font, FC_STYLE, 0, &data) == FcResultMatch) {
			if (strcmp((const char *)data, "Bold"))
				continue;
		} else
			continue;

		// 3) нужна поддержка всех языков меню en|de|ru
		if (FcPatternGetLangSet(font, FC_LANG, 0, &langset) == FcResultMatch) {
			int FontLangCount = 0;
			for (int i = 0; i < vw_GetLanguageListCount(); i++) {
				const FcChar8 *lang = (const FcChar8*) vw_GetLanguageList()[i].code;

				if (lang) {
					FcLangResult langRes = FcLangSetHasLang(langset, lang);
					if (langRes != FcLangDifferentLang)
						FontLangCount++;
				}
			}

			if (FontLangCount < vw_GetLanguageListCount())
				continue;
		} else
			continue;


		AppropriateFontsCount++;
	}
	printf("Appropriate fonts detected: %i\n\n", AppropriateFontsCount);
	if (AppropriateFontsCount == 0) {
		fprintf(stderr, "Couldn't find any appropriate fonts installed in your system.\n");
		fprintf(stderr, "Please, check your fonts or install TrueType, bold style font\n");
		fprintf(stderr, "with ");
		for (int i = 0; i < vw_GetLanguageListCount()-1; i++) {
			fprintf(stderr, "%s, ", vw_GetLanguageList()[i].code);
		}
		fprintf(stderr, "and %s languages support.\n", vw_GetLanguageList()[vw_GetLanguageListCount()-1].code);
		return -1;
	}



	// второй проход, формируем таблицу
	FontQuantity = 0;
	FontList = new sFontList[AppropriateFontsCount];
	for (int i = 0; fs && (i < fs->nfont); i++) {
		FcPattern *font = fs->fonts[i];


		// 1) Работаем только с TrueType, CFF и Type 1
		if (FcPatternGetString(font, FC_FONTFORMAT, 0, &data) == FcResultMatch) {
			if (strcmp((const char *)data, "TrueType") &&
			    strcmp((const char *)data, "CFF") &&
			    strcmp((const char *)data, "Type 1")) {
				continue;
			}
		} else
			continue;

		// 2) берем только болд
		if (FcPatternGetString(font, FC_STYLE, 0, &data) == FcResultMatch) {
			if (strcmp((const char *)data, "Bold"))
				continue;
		} else
			continue;

		// 3) нужна поддержка всех языков меню en|de|ru
		if (FcPatternGetLangSet(font, FC_LANG, 0, &langset) == FcResultMatch) {
			int FontLangCount = 0;
			for (int i = 0; i < vw_GetLanguageListCount(); i++) {
				const FcChar8 *lang = (const FcChar8*) vw_GetLanguageList()[i].code;

				if (lang) {
					FcLangResult langRes = FcLangSetHasLang(langset, lang);
					if (langRes != FcLangDifferentLang)
						FontLangCount++;
				}
			}

			if (FontLangCount < vw_GetLanguageListCount())
				continue;
		} else
			continue;




		if (FcPatternGetString(font, FC_FAMILY, 0, &data) == FcResultMatch) {
			FontList[FontQuantity].FontTitle = new char[strlen((const char*)data)+1];
			strcpy(FontList[FontQuantity].FontTitle, (const char *)data);
			printf("Family: %s\n", FontList[FontQuantity].FontTitle);
		}
		if (FcPatternGetString(font, FC_FILE, 0, &data) == FcResultMatch) {
			FontList[FontQuantity].FontFileName = new char[strlen((const char*)data)+1];
			strcpy(FontList[FontQuantity].FontFileName, (const char *)data);
			printf(" Font Filename: %s\n", FontList[FontQuantity].FontFileName);
		}

		FontQuantity++;
	}
	printf("\n");



	// fontconfig выдает все шрифты подряд, нужна сортировка по названию
	for (int j=0; j<FontQuantity-1; j++) {
		for (int i=0; i<FontQuantity-1; i++) {
			unsigned int CurrentSymbol = 0;

			while ((CurrentSymbol < strlen(FontList[i].FontTitle)) &&
			       (CurrentSymbol < strlen(FontList[i+1].FontTitle))) {
				if (FontList[i].FontTitle[CurrentSymbol] > FontList[i+1].FontTitle[CurrentSymbol]) {
					char *Tmp = FontList[i+1].FontTitle;
					FontList[i+1].FontTitle = FontList[i].FontTitle;
					FontList[i].FontTitle = Tmp;

					Tmp = FontList[i+1].FontFileName;
					FontList[i+1].FontFileName = FontList[i].FontFileName;
					FontList[i].FontFileName = Tmp;

					break;
				} else {
					if (FontList[i].FontTitle[CurrentSymbol] < FontList[i+1].FontTitle[CurrentSymbol])
						break;
				}

				CurrentSymbol++;
			}
		}
	}


	// по названию, находим текущий номер шрифта в таблице
	Setup.FontNumber = -1;
	for (int i=0; i<FontQuantity; i++) {
		if (!strcmp(Setup.FontName, FontList[i].FontTitle)) {
			Setup.FontNumber = i;
			break;
		}
	}
	// если его нет - ставим
	if (Setup.FontNumber == -1) {
		Setup.FontNumber = 0;
		strcpy(Setup.FontName, FontList[Setup.FontNumber].FontTitle);
	}



	FcFontSetDestroy(fs);
	return 0;
}


void ReleaseFontConfig()
{
	for (int i=0; i<FontQuantity; i++) {
		delete [] FontList[i].FontTitle;
		delete [] FontList[i].FontFileName;
	}
	delete [] FontList;
	FontList = 0;
	FontQuantity = 0;
}

#endif // fontconfig




//------------------------------------------------------------------------------------
// основная процедура...
//------------------------------------------------------------------------------------
int main( int argc, char **argv )
{

	// флаг отображать ли системный курсор
	bool NeedShowSystemCursor = false;
	// флаг нужно ли сбрасывать настройки игры при старте
	bool NeedSafeMode = false;
	// флаг перевода игры в режим упаковки gamedata.vfs файла
	bool NeedPack = false;

	for (int i=1; i<argc; i++) {
		// проверка ключа "--help"
		if (!strcmp(argv[i], "--help")) {
			printf("AstroMenace launch options:\n\n");

			printf("--dir=/game/data/folder/ - folder with gamedata.vfs file (Linux only)\n");
			printf("--mouse - launch the game without system cursor hiding.\n");
			printf("--safe-mode - reset all settings not connected to Pilots Profiles at the game launch.\n");
			printf("--pack - pack data to gamedata.vfs file\n");
			printf("--rawdata=/game/rawdata/folder/ - folder with game raw data for gamedata.vfs.\n");
			printf("--help - info about all game launch options.\n");

			return 0;
		}

		// проверка ключа "--mouse"
		if (!strcmp(argv[i], "--mouse"))
			NeedShowSystemCursor = true;

		// проверка ключа "--safe-mode"
		if (!strcmp(argv[i], "--safe-mode"))
			NeedSafeMode = true;

		// проверка ключа "--pack"
		if (!strcmp(argv[i], "--pack"))
			NeedPack = true;
	}



#ifdef WIN32
	// иним пути для винды
	ZeroMemory(ProgrammDir, sizeof(ProgrammDir));
	GetModuleFileName(nullptr, ProgrammDir, MAX_PATH);
	char* s = strrchr(ProgrammDir,'\\');
	if (s) s[0]=0x0;
	const char *Fi = "\\";
	strcat( ProgrammDir, Fi );

	ZeroMemory(ConfigFileName, sizeof(ConfigFileName));
	ZeroMemory(VFSFileNamePath, sizeof(VFSFileNamePath));
	ZeroMemory(ScreenshotDir, sizeof(ScreenshotDir));

	// Получаем данные, где папка пользователя
	bool InitWithoutDLL = true;
	bool InitScrWithoutDLL = true;
	HMODULE hShellDLL = LoadLibrary("shell32.dll");
	if (hShellDLL) {
		pSHGetSpecialFolderPath = (SHGETSPECIALFOLDERPATH) GetProcAddress(hShellDLL, "SHGetSpecialFolderPathA");
		char UserPath[MAX_PATH];
		ZeroMemory(UserPath, sizeof(UserPath));

		if (pSHGetSpecialFolderPath != nullptr) {
			if(SUCCEEDED(pSHGetSpecialFolderPath(nullptr,
							     UserPath,
							     SD_APPDATA, //CSIDL_APPDATA
							     true))) {
				strcat(UserPath, "\\AstroMenace\\");
				CreateDirectory(UserPath, nullptr);

				strcpy(ConfigFileName, UserPath);
				strcat(ConfigFileName, "amconfig.xml");

				// уже проинили, дальше не нужно
				InitWithoutDLL = false;
			}

			if(SUCCEEDED(pSHGetSpecialFolderPath(nullptr,
							     ScreenshotDir,
							     SD_DESKTOPDIRECTORY, // SD_DESKTOPDIRECTORY
							     true))) {

				strcat(ScreenshotDir, "\\AstroMenaceScreenshot");

				// уже проинили, дальше не нужно
				InitScrWithoutDLL = false;
			}

		}

	}
	// освобождаем библиотеку
	FreeLibrary(hShellDLL);


	// иним, если старая винда, или была ошибка
	if (InitWithoutDLL) {
		strcpy(ConfigFileName, ProgrammDir);
		strcat(ConfigFileName, "amconfig.xml");
	}
	if (InitScrWithoutDLL) {
		strcpy(ScreenshotDir, ProgrammDir);
		strcat(ScreenshotDir, "AstroMenaceScreenshot");
	}

	strcpy(VFSFileNamePath, ProgrammDir);
	strcat(VFSFileNamePath, "gamedata.vfs");
#elif defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
	// иним пути для юникса-линукса
	// если передали параметр-путь

	const char* HomeEnv = getenv("HOME");
	if (HomeEnv == nullptr) {
		fprintf(stderr, "$HOME is not set, will use getpwuid() and getuid() for home folder detection.\n");
		struct passwd *pw = getpwuid(getuid());
		if (pw != nullptr)
			HomeEnv = pw->pw_dir;
		else
			fprintf(stderr, "Can't detect home folder. Note, this could occur segfault issue, if yours distro don't support XDG Base Directory Specification.\n");
	}

	bool dirpresent = false;
	for (int i=1; i<argc; i++) {
		if (!strncmp(argv[i], "--dir=", sizeof("--dir"))) {
			dirpresent = true;
			// если передали относительный путь в папку пользователя с тильдой
			if ((argv[i][sizeof("--dir")] != '~') || (HomeEnv == nullptr)) {
				strcpy(ProgrammDir, argv[i]+strlen("--dir="));
			} else {
				strcpy(ProgrammDir, HomeEnv);
				strcat(ProgrammDir, argv[i]+strlen("--dir=~"));
			}
			// если в конце нет слеша - ставим его
			if (ProgrammDir[strlen(ProgrammDir)-1] != '/')
				strncat(ProgrammDir, "/", strlen("/"));

		}
	}
	if (!dirpresent) {
#ifdef DATADIR
		strcpy(ProgrammDir, DATADIR "/");
#else
		strcpy(ProgrammDir, argv[0]);
		char* s = strrchr(ProgrammDir,'/');
		if (s) s[0]=0x0;
		const char *Fi = "/";
		strcat( ProgrammDir, Fi );
#endif // DATADIR
	}


	VFSFileNamePath = ProgrammDir;
	VFSFileNamePath += "gamedata.vfs";


	if (!NeedPack) {
		// first at all we need check XDG_DESKTOP_DIR environment variable
		const char* DesktopDirEnv = getenv("XDG_DESKTOP_DIR");
		if (DesktopDirEnv != nullptr) {
			strcpy(ScreenshotDir, DesktopDirEnv);
		} else {
			strcpy(ScreenshotDir, HomeEnv);
			strcat(ScreenshotDir, "/Desktop/");
		}


		// first at all we need check XDG_CONFIG_HOME environment variable
		const char* ConfigHomeEnv = getenv("XDG_CONFIG_HOME");
		if (ConfigHomeEnv != nullptr) {
			strcpy(ConfigFileName, ConfigHomeEnv);
			strcat(ConfigFileName, "/astromenace");
		} else {
			// game config file will be stored in "~/.config/astromenace" folder
			// if system have "~/.config" folder, otherwise in "~/.astromenace" folder
			strcpy(ConfigFileName, HomeEnv);
			char ConfigDirCheck[MAX_PATH];
			strcpy(ConfigDirCheck, HomeEnv);
			strcat(ConfigDirCheck, "/.config");
			struct stat st;
			if (stat(ConfigDirCheck,&st) == 0)
				strcat(ConfigFileName, "/.config/astromenace");
			else
				strcat(ConfigFileName, "/.astromenace");
		}

		mkdir(ConfigFileName, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		strcat(ConfigFileName, "/amconfig.xml");
	}

#endif // unix


#ifdef portable
	strcpy(ConfigFileName, ProgrammDir);
	strcat(ConfigFileName, "/amconfig.xml");
#endif // portable




	// версия
	printf("AstroMenace %s %i\n\n", GAME_VERSION, GAME_BUILD);




	SDL_version compiled;
	SDL_version linked;

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	printf("Compiled against SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	printf("Linking against SDL version %d.%d.%d\n\n", linked.major, linked.minor, linked.patch);




	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// переводим в режим генерации gamedata.vfs файла
	// генерируем файл данный gamedata.vfs учитывая текущее его расположение
	// !!! всегда делаем только с одним открытым на запись VFS
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (NeedPack) {
		char RawDataDir[MAX_PATH];
		// по умолчанию, считаем что рав данные прямо с нами лежат
		strcpy(RawDataDir, ProgrammDir);
		strcat(RawDataDir, "gamedata/");


		// ищем, если передали ключем его расположение
		for (int i=1; i<argc; i++) {
			if (!strncmp(argv[i], "--rawdata=", sizeof("--rawdata"))) {
#if defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
				// если передали относительный путь в папку пользователя с тильдой
				if ((argv[i][sizeof("--rawdata")] != '~') || (HomeEnv == nullptr))
					strcpy(RawDataDir, argv[i]+strlen("--rawdata="));
				else {
					strcpy(RawDataDir, HomeEnv);
					strcat(RawDataDir, argv[i]+strlen("--rawdata=~"));
				}
#elif defined(WIN32)
				// если есть двоеточия после второго символа - это полный путь с указанием девайса
				if (argv[i][sizeof("--rawdata=")] == ':') {
					strcpy(RawDataDir, argv[i]+strlen("--rawdata="));
				} else {
					strcpy(RawDataDir, ProgrammDir);
					strcat(RawDataDir, argv[i]+strlen("--rawdata="));
				}
#endif // WIN32
				// если в конце нет слеша - ставим его
				if (RawDataDir[strlen(RawDataDir)-1] != '/')
					strncat(RawDataDir, "/", strlen("/"));
			}
		}

		printf("Source Raw Folder: %s\n", RawDataDir);
		return ConvertFS2VFS(RawDataDir, VFSFileNamePath);
	}







	// инициализация счета времени
	vw_InitTime();


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// подключаем VFS
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (vw_OpenVFS(VFSFileNamePath, GAME_BUILD) != 0) {
		fprintf(stderr, "gamedata.vfs file not found or corrupted.\n");
		return 0;
	}
	printf("\n");


	// загружаем все текстовые данные до инициализации шрифтов, т.к. нам нужен перечень языков в процессе инициализации fontconfig
	if (vw_InitText("lang/text.csv", ';', '\n') != 0) {
		fprintf(stderr, "lang/text.csv file not found or corrupted.\n");
		return 0;
	}

	// работа с файлом данных... передаем базовый режим окна (обязательно после инициализации языков!)
	bool FirstStart = LoadXMLSetupFile(NeedSafeMode);

#ifdef fontconfig
	// инициализация fontconfig
	if (InitFontConfig() != 0) {
		vw_ReleaseText();
		return -1;
	}
#endif // fontconfig
	// дополнительная проверка, если использовали fontconfig, а потом вернулись на версию со встроенными шрифтами
	if (Setup.FontNumber > FontQuantity) Setup.FontNumber = 0;

	// иним фонт
	vw_InitFont(FontList[Setup.FontNumber].FontFileName);
	vw_SetFontSize(Setup.FontSize);
	// ставим доп смещение при прорисовке в 2 ("привет" от старого шрифта из первых версий игры)
	vw_SetFontOffsetY(8 - ceil(Setup.FontSize/2.0f) + 2);


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// установка звука, всегда до LoadGameData
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (!InitAudio()) {
		Setup.Music_check = false;
		Setup.Sound_check = false;
		fprintf(stderr, "Unable to open audio!\n");
		printf("\n");
	}



ReCreate:


#ifdef __unix
	// для TwinView и Xinerama выбираем нулевой, но не меняем если передали
	setenv("SDL_VIDEO_FULLSCREEN_DISPLAY","0",0);
#endif //unix

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// иним SDL
	// это нужно сделать сразу, чтобы правильно поставить разрешение
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
		return 1;
	}



	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// сбрасываем все, ничего не загружали
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	for (int i=0; i<1000; i++)
		LoadedTypes[i] = false;



	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// получаем текущее разрешение экрана
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// предварительно ставим по умолчанию
	CurrentVideoMode.W = 1024;
	CurrentVideoMode.H = 768;
	CurrentVideoMode.BPP = 16;
	// пытаемся получить данные
	int iScreen = 0;
#ifdef __unix
	// смотрим, если передали параметр SDL_VIDEO_FULLSCREEN_DISPLAY, берем параметры нужного дисплея
	char * pScreen;
	pScreen = getenv("SDL_VIDEO_FULLSCREEN_DISPLAY");
	if (pScreen != nullptr)
		iScreen = atoi(pScreen);
	if (iScreen > SDL_GetNumVideoDisplays() - 1)
		iScreen = 0;
#endif // unix
	int CurrentVideoModeX = SDL_WINDOWPOS_CENTERED_DISPLAY(iScreen);
	int CurrentVideoModeY = SDL_WINDOWPOS_CENTERED_DISPLAY(iScreen);
	SDL_DisplayMode CurrentDisplayMode;
	SDL_GetDesktopDisplayMode(iScreen, &CurrentDisplayMode);
	CurrentVideoMode.BPP = SDL_BITSPERPIXEL(CurrentDisplayMode.format);
	CurrentVideoMode.W = CurrentDisplayMode.w;
	CurrentVideoMode.H = CurrentDisplayMode.h;
	printf("Current Video Mode: %ix%i %ibit \n", CurrentVideoMode.W, CurrentVideoMode.H, CurrentVideoMode.BPP);

	printf("Screen count: %i\n", SDL_GetNumVideoDisplays());
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		SDL_DisplayMode testDisplayMode;
		SDL_GetDesktopDisplayMode(i, &testDisplayMode);
		printf("Screen #%i: %i x %i\n", i, testDisplayMode.w, testDisplayMode.h);
	}





	// детектим и составляем перечень всех возможных разрешений самостоятельно
	sVideoModes AllSupportedModes[] = {
		{640, 480, -1},
		{768, 480, -1},
		{800, 480, -1},
		{800, 600, -1},
		{832, 624, -1},
		{854, 480, -1},
		{960, 540, -1},
		{960, 544, -1},
		{960, 640, -1},
		{960, 720, -1},
		{1024, 576, -1},
		{1024, 600, -1},
		{1024, 640, -1},
		{1024, 768, -1},
		{1024, 800, -1},
		{1120, 832, -1},
		{1152, 720, -1},
		{1152, 768, -1},
		{1152, 864, -1},
		{1152, 900, -1},
		{1280, 720, -1},
		{1280, 768, -1},
		{1280, 800, -1},
		{1280, 854, -1},
		{1280, 960, -1},
		{1280, 1024, -1},
		{1366, 768, -1},
		{1400, 1050, -1},
		{1440, 900, -1},
		{1440, 960, -1},
		{1440, 1024, -1},
		{1440, 1080, -1},
		{1600, 900, -1},
		{1600, 1024, -1},
		{1600, 1200, -1},
		{1680, 1050, -1},
		{1792, 1344, -1},
		{1856, 1392, -1},
		{1800, 1440, -1},
		{1920, 1080, -1},
		{1920, 1200, -1},
		{1920, 1400, -1},
		{1920, 1440, -1},
		{2048, 1280, -1},
		{2048, 1536, -1},
		{2304, 1440, -1},
		{2304, 1728, -1},
		{2560, 1440, -1},
		{2560, 1600, -1},
		{2560, 1920, -1},
		{2560, 2048, -1},
		{2800, 2100, -1},
		{2880, 1800, -1},
		{3200, 2048, -1},
		{3200, 2400, -1},
		{3840, 2160, -1},
		{3840, 2400, -1},
		{4096, 2304, -1},
		{4096, 3072, -1},
		{5120, 3200, -1},
		{5120, 4096, -1},
		{6400, 4096, -1},
		{6400, 4800, -1},
		{7680, 4320, -1},
		{7680, 4800, -1},
	};
#define AllSupportedModesCount sizeof(AllSupportedModes)/sizeof(AllSupportedModes[0])

	// если списка еще нет - создаем его
	if (VideoModes == nullptr) {
		VideoModesNum = 0;

		for(unsigned int i = 0; i < AllSupportedModesCount; i++)
			if ((AllSupportedModes[i].W <= CurrentVideoMode.W) &&
			    (AllSupportedModes[i].H <= CurrentVideoMode.H)) {
				// если мы подходим по размерам экрана для оконного режима - просто добавляем его
				VideoModesNum++;

				// пропускаем разрешение с одинаковыми параметрами и разной частотой
				int SkipPrevH = -1;
				int SkipPrevW = -1;
				// добавляем к полноэкранным режимам, если входит в список
				for (int j = 0; j < SDL_GetNumDisplayModes(iScreen); j++) {
					SDL_DisplayMode testDisplayMode;
					SDL_GetDisplayMode(iScreen, j, &testDisplayMode);
					if (SkipPrevH == -1) {
						SkipPrevH = testDisplayMode.h;
						SkipPrevW = testDisplayMode.w;
						if ((testDisplayMode.w == AllSupportedModes[i].W) &&
						    (testDisplayMode.h == AllSupportedModes[i].H))
							VideoModesNum++;
					} else {
						if ((SkipPrevH != testDisplayMode.h) ||
						    (SkipPrevW != testDisplayMode.w)) {
							if ((testDisplayMode.w == AllSupportedModes[i].W) &&
							    (testDisplayMode.h == AllSupportedModes[i].H))
								VideoModesNum++;
							SkipPrevH = testDisplayMode.h;
							SkipPrevW = testDisplayMode.w;
						}
					}
				}
			}


		VideoModes = new sVideoModes[VideoModesNum];

		int k=0;
		for(unsigned int i = 0; i < AllSupportedModesCount; i++)
			if ((AllSupportedModes[i].W <= CurrentVideoMode.W) &&
			    (AllSupportedModes[i].H <= CurrentVideoMode.H)) {
				// если мы подходим по размерам экрана для оконного режима - просто добавляем его
				VideoModes[k].W = AllSupportedModes[i].W;
				VideoModes[k].H = AllSupportedModes[i].H;
				VideoModes[k].BPP = 0;
				k++;

				// пропускаем разрешение с одинаковыми параметрами и разной частотой
				int SkipPrevH = -1;
				int SkipPrevW = -1;
				// добавляем к полноэкранным режимам, если входит в список
				for (int j = 0; j < SDL_GetNumDisplayModes(iScreen); j++) {
					SDL_DisplayMode testDisplayMode;
					SDL_GetDisplayMode(iScreen, j, &testDisplayMode);
					if (SkipPrevH == -1) {
						SkipPrevH = testDisplayMode.h;
						SkipPrevW = testDisplayMode.w;
						if ((testDisplayMode.w == AllSupportedModes[i].W) &&
						    (testDisplayMode.h == AllSupportedModes[i].H)) {
							VideoModes[k].W = AllSupportedModes[i].W;
							VideoModes[k].H = AllSupportedModes[i].H;
							VideoModes[k].BPP = CurrentVideoMode.BPP;
							k++;
						}
					} else {
						if ((SkipPrevH != testDisplayMode.h) ||
						    (SkipPrevW != testDisplayMode.w)) {
							if ((testDisplayMode.w == AllSupportedModes[i].W) &&
							    (testDisplayMode.h == AllSupportedModes[i].H)) {
								VideoModes[k].W = AllSupportedModes[i].W;
								VideoModes[k].H = AllSupportedModes[i].H;
								VideoModes[k].BPP = CurrentVideoMode.BPP;
								k++;
							}
							SkipPrevH = testDisplayMode.h;
							SkipPrevW = testDisplayMode.w;
						}
					}
				}
			}
		// выводим список поддерживаемых разрешений
		printf("\n");
		printf("Supported resolutions list:\n");
		for(int i = 0; i < VideoModesNum; i++) {
			printf("%ix%i %ibit \n", VideoModes[i].W, VideoModes[i].H, VideoModes[i].BPP);
		}
		printf("\n");

	}





	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// устанавливаем и корректируем текущие настройки окна
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (FirstStart) {
		Setup.Width = CurrentVideoMode.W;
		Setup.Height = CurrentVideoMode.H;
		Setup.BPP = CurrentVideoMode.BPP;
	}
	// если загруженные параметры, больше чем максимальные, ставим максимальные (если Xinerama, например)
	if ((VideoModes[VideoModesNum-1].W < Setup.Width) ||
	    (VideoModes[VideoModesNum-1].H < Setup.Height)) {
		Setup.Width = VideoModes[VideoModesNum-1].W;
		Setup.Height = VideoModes[VideoModesNum-1].H;
		Setup.BPP = CurrentVideoMode.BPP;
	}
	// делаем проверку по листу разрешений экрана, если входит - все ок, если нет - ставим оконный режим принудительно
	bool NeedResetToWindowedMode = true;
	for(int i = 0; i < VideoModesNum; i++) {
		if ((VideoModes[i].W == Setup.Width) &&
		    (VideoModes[i].H == Setup.Height) &&
		    (VideoModes[i].BPP == Setup.BPP)) {
			NeedResetToWindowedMode = false;
			break;
		}
	}
	if (NeedResetToWindowedMode)
		Setup.BPP = 0;








	// устанавливаем язык текста
	vw_SetTextLanguage(Setup.MenuLanguage);







	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// проверяем и иним джойстик
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef joystick
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0) {
		if (SDL_NumJoysticks()>0) {
			printf("Found Joystick(s):\n");
			for (int i=0; i<SDL_NumJoysticks(); i++) {
				printf("Joystick Name %i: %s\n", i, SDL_JoystickNameForIndex(i));
			}

			Joystick = SDL_JoystickOpen(Setup.JoystickNum);

			if(Joystick) {
				printf("Opened Joystick %i\n", Setup.JoystickNum);
				printf("Joystick Name: %s\n", SDL_JoystickNameForIndex(Setup.JoystickNum));
				printf("Joystick Number of Axes: %d\n", SDL_JoystickNumAxes(Joystick));
				printf("Joystick Number of Buttons: %d\n", SDL_JoystickNumButtons(Joystick));
				printf("Joystick Number of Balls: %d\n\n", SDL_JoystickNumBalls(Joystick));
			} else {
				fprintf(stderr, "Couldn't open Joystick 0\n\n");
			}
		}
	} else {
		fprintf(stderr, "Can't init Joystick, SDL Error: %s\n", SDL_GetError());
	}
#endif





	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// создаем окно и базовые опенжл контекст
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	bool Fullscreen = (Setup.BPP != 0);
	int InitStatus = vw_InitWindow("AstroMenace", Setup.Width, Setup.Height, &Setup.BPP, Fullscreen, CurrentVideoModeX, CurrentVideoModeY, Setup.VSync);

	// ошибка окна (размеры)
	if (InitStatus == 1) {
		// не можем создать окно или включить полноэкранный режим - ставим минимальный оконный режим
		if ((640 != Setup.Width) ||
		    (480 != Setup.Height) ||
		    (0 != Setup.BPP)) {
			Setup.Width = 640;
			Setup.Height = 480;
			Setup.BPP = 0;
			Setup.fAspectRatioWidth = 1024.0f;
			Setup.fAspectRatioHeight = 768.0f;
			Setup.iAspectRatioWidth = 1024;
			Setup.iAspectRatioHeight = 768;
			Setup.MSAA = 0;
			Setup.CSAA = 0;
			SaveXMLSetupFile();
			SDL_Quit();
			FirstStart = false;
			goto ReCreate;
		}

		fprintf(stderr, "Wrong resolution. Fatal error.\n");
#ifdef WIN32
		MessageBox(nullptr,"Wrong resolution. Please, install the newest video drivers from your video card vendor.", "Render system - Fatal Error", MB_OK|MB_APPLMODAL|MB_ICONERROR);
#endif // WIN32
		SDL_Quit();
		return 0;									// Quit If Window Was Not Created
	}
	if (!Fullscreen)
		Setup.BPP = 0;





	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// проверяем возможности железа
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// если нужно, устанавливаем перерытие значений внутри движка, може только выключить - включить то чего нет нельзя
	if (Setup.VBOCoreMode == 0)
		vw_GetDevCaps()->VBOSupported = false;

	// работаем только если есть VBO
	if ((Setup.VAOCoreMode == 0) ||
	    (Setup.VBOCoreMode == 0) ||
	    (!vw_GetDevCaps()->VBOSupported))
		vw_GetDevCaps()->VAOSupported = false;

	if (Setup.FBOCoreMode == 0)
		vw_GetDevCaps()->FramebufferObject = false;

	// проверка поддержки шейдеров (нужна 100% поддержка GLSL 1.0)
	if (Setup.UseGLSL &&
	    (!vw_GetDevCaps()->GLSL100Supported || vw_GetDevCaps()->ShaderModel < 3.0f))
		Setup.UseGLSL = false;

	// управление генерации мипмеп уровней- можем только выключить, нельзя включить если его нет
	if (!Setup.HardwareMipMapGeneration)
		vw_GetDevCaps()->HardwareMipMapGeneration = false;

	// анализ системы только если это первый запуск
	if (FirstStart) {
		// если шейдерная модель 3-я или выше
		if (vw_GetDevCaps()->ShaderModel >= 3.0f) {
			// памяти достаточно, включаем другой режим загрузки
			Setup.EqualOrMore128MBVideoRAM = true;
		}
		// если шейдерная модель 4-я или выше
		if (vw_GetDevCaps()->ShaderModel >= 4.0f) {
			// 100% держит наши шейдеры
			Setup.UseGLSL = true;
			Setup.ShadowMap = 2;
			// 100% больше чем нужно памяти и не надо сжимать текстуры (ув. качество и скорость загрузки)
			Setup.TexturesCompressionType = 0;
			// немного больше ставим другие опции
			Setup.MSAA = 2;
			Setup.CSAA = 2;
			Setup.AnisotropyLevel = vw_GetDevCaps()->MaxAnisotropyLevel;
			Setup.MaxPointLights = 4;
		}
		// если шейдерная модель 4.2-я или выше
		if (vw_GetDevCaps()->ShaderModel >= 4.2f) {
			// немного больше ставим другие опции
			Setup.ShadowMap = 5;
			Setup.MSAA = 4;
			Setup.CSAA = 4;
			Setup.MaxPointLights = 6;
		}

		// если поддерживаем сторедж - выключаем поддержку сжатия, 100% у нас достаточно видео памяти
		if (vw_GetDevCaps()->TextureStorage)
			Setup.TexturesCompressionType = 0;

#if defined(__APPLE__) && defined(__MACH__)
		// для маков по умолчанию выключаем сглаживание, тени и шейдеры, т.к. там может все софтово эмулироваться и жутко тормозить
		Setup.MSAA = 0;
		Setup.CSAA = 0;
		Setup.UseGLSL = false;
		Setup.ShadowMap = 0;
#endif

		// устанавливаем соотношение сторон по установленному разрешению экрана
		if ((Setup.Width*1.0f)/(Setup.Height*1.0f) < 1.4f) {
			Setup.fAspectRatioWidth = 1024.0f;
			Setup.fAspectRatioHeight = 768.0f;
			Setup.iAspectRatioWidth = 1024;
			Setup.iAspectRatioHeight = 768;
		} else {
			Setup.fAspectRatioWidth = 1228.0f;
			Setup.fAspectRatioHeight = 768.0f;
			Setup.iAspectRatioWidth = 1228;
			Setup.iAspectRatioHeight = 768;
		}
	}

	// если не поддерживает железо фбо или шейдеры, выключаем шадовмеп
	if (!vw_GetDevCaps()->FramebufferObject ||
	    !vw_GetDevCaps()->GLSL100Supported ||
	    (vw_GetDevCaps()->ShaderModel < 3.0f))
		Setup.ShadowMap = 0;

	if (Setup.MSAA > vw_GetDevCaps()->MaxSamples) Setup.MSAA = Setup.CSAA = vw_GetDevCaps()->MaxSamples;
	// на всякий случай проверяем, входит ли текущее сглаживание в список доступных
	int CurrentAAMode = -1;
	if (Setup.MSAA != 0) {
		for (int i = 0; i < vw_GetDevCaps()->MaxMultisampleCoverageModes; i++) {
			if ((vw_GetDevCaps()->MultisampleCoverageModes[i].ColorSamples == Setup.MSAA) &&
			    (vw_GetDevCaps()->MultisampleCoverageModes[i].CoverageSamples == Setup.CSAA)) {
				CurrentAAMode = i;
				break;
			}
		}
	}
	// если ничего не нашли, сбрасываем в нули
	if (CurrentAAMode == -1)
		Setup.MSAA = Setup.CSAA = 0;

	// проверка режима сжатия текстур
	if (!vw_GetDevCaps()->TexturesCompression && (Setup.TexturesCompressionType > 0))
		Setup.TexturesCompressionType = 0;
	if (!vw_GetDevCaps()->TexturesCompressionBPTC && (Setup.TexturesCompressionType > 1))
		Setup.TexturesCompressionType = 1;






	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// завершаем инициализацию
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	vw_InitOpenGL(Setup.Width, Setup.Height, &Setup.MSAA, &Setup.CSAA);


	// вторичная работа с настройками
	// в процессе инициализации opengl контекста мы подключаем указатели на функции, и данные могут измениться


	// если не поддерживаем как минимум 2 текстуры, железо очень слабое - не запустимся
	if (vw_GetDevCaps()->MaxMultTextures < 2) {
		SDL_Quit();
		fprintf(stderr, "The Multi Textures feature unsupported by hardware. Fatal error.\n");
#ifdef WIN32
		MessageBox(nullptr,"OpenGL 1.3 required. Please, install the newest video drivers from your video card vendor.", "Render system - Fatal Error", MB_OK|MB_APPLMODAL|MB_ICONERROR);
#endif // WIN32
		return 0;
	}

	// сохраняем данные во время первого старта сразу после инициализации "железа"
	// в случае некорректного завершения игры, файл настроек будет содержать актуальные параметры
	if (FirstStart)
		SaveXMLSetupFile();








	// ставим данные к которым приводить, плюс они же являются соотношением сторон
	vw_SetAspectRatio(Setup.fAspectRatioWidth, Setup.fAspectRatioHeight, true);





	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// скрываем системный курсор
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if (!NeedShowSystemCursor)
		SDL_ShowCursor(SDL_DISABLE);



	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// загрузка текстур согласно моделе загрузки
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	LoadGameData(-1);
	// загрузка списка миссий
	MissionsListInit();




	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// установка процедуры... основного цикла и переход на нее
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	CanQuit = true;


	// первоначальная установка курсора
	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	vw_SetMousePos(mouse_x, mouse_y);

	// первоначальный сброс кнопок мышки
	for (int i = 0; i < 8; i++) {
		SDL_MouseCurrentStatus[i] = false;
	}



	// если первый запуск игры - предлогаем выбор языка
	if (FirstStart)
		SetCurrentDialogBox(16);



loop:


	Quit = false;
	NeedReCreate = false;
	bool NeedLoop = true;


	// начальные установки джойстика
	JoystickAxisX = 0;
	JoystickAxisY = 0;
	float JoystickCurentTime = vw_GetTime();
	float JoystickTimeDelta = 0.0f;
#ifdef joystick
	if (Joystick != nullptr) {
		JoystickAxisX = SDL_JoystickGetAxis(Joystick, 0);
		JoystickAxisY = SDL_JoystickGetAxis(Joystick, 1);

		for (int i=0; i<100; i++) {
			JoysticButtons[i] = false;
		}
	}
#endif

	while(!Quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			// если нажали закрыть окно
			case SDL_QUIT:
				Quit = true;
				goto GotoQuit;

			// работаем с движением мышки
			case SDL_MOUSEMOTION:
				vw_SetMousePos(event.motion.x, event.motion.y);
				// если есть перемещение мышкой - сразу убираем управление клавиатурой
				CurrentKeyboardSelectMenuElement = 0;
				break;

			case SDL_MOUSEWHEEL:
				vw_ChangeWheelStatus(-1*event.wheel.y);
				break;
			// обрабатываем кнопки мыши
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					vw_SetWindowLBMouse(true);
					vw_SetWindowLBDoubleMouse(true);
				}
				if (event.button.button ==  SDL_BUTTON_RIGHT)
					vw_SetWindowRBMouse(true);
				if (event.button.button < 8) // на всякий случай небольшая проверка
					SDL_MouseCurrentStatus[event.button.button] = true;
				break;
			case SDL_JOYBUTTONDOWN:
				vw_SetWindowLBMouse(true);
				vw_SetWindowLBDoubleMouse(true);
				JoysticButtons[event.jbutton.button] = true;
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button ==  SDL_BUTTON_LEFT)
					vw_SetWindowLBMouse(false);
				if (event.button.button ==  SDL_BUTTON_RIGHT)
					vw_SetWindowRBMouse(false);
				if (event.button.button < 8) // на всякий случай небольшая проверка
					SDL_MouseCurrentStatus[event.button.button] = false;
				break;
			case SDL_JOYBUTTONUP:
				vw_SetWindowLBMouse(false);
				JoysticButtons[event.jbutton.button] = false;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_FOCUS_LOST:
				case SDL_WINDOWEVENT_MINIMIZED:
					NeedLoop = false;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				case SDL_WINDOWEVENT_RESTORED:
					NeedLoop = true;
					break;
				}
				break;
			case SDL_TEXTINPUT:
				// устанавливаем текущий юникод нажатой клавиши
				vw_SetCurrentUnicodeChar(event.text.text);
#ifdef gamedebug
				printf("TextInput, Unicode: %s \n", event.text.text);
#endif // gamedebug
				break;
			default:
				break;
			}
		}

		// если есть джойстик, считаем для него время
		if (Joystick != nullptr) {
			JoystickTimeDelta = vw_GetTime() - JoystickCurentTime;
			JoystickCurentTime = vw_GetTime();
		}

		// если окно видемое - рисуем
		if (NeedLoop) {

#ifdef joystick
			// управление джойстиком
			if (Joystick != nullptr) {
				int X = SDL_JoystickGetAxis(Joystick, 0);
				int Y = SDL_JoystickGetAxis(Joystick, 1);

				// учитываем "мертвую зону" хода ручки джойстика
				if (abs(X) < Setup.JoystickDeadZone*3000) X = 0;
				if (abs(Y) < Setup.JoystickDeadZone*3000) Y = 0;

				if ((JoystickAxisX != X) || (JoystickAxisY != Y)) {
					JoystickAxisX = 0;
					JoystickAxisY = 0;

					int Xsm = (int)(1000*(X/32768.0f)*JoystickTimeDelta);
					int Ysm = (int)(1000*(Y/32768.0f)*JoystickTimeDelta);

					vw_SetMousePosRel(Xsm, Ysm);
				}
			}
#endif

			// всегда включаем счет времени
			vw_StartTime();
			// переход на основную программу
			Loop_Proc();

		} else {
			// всегда выключаем счет времени и считаем новый лаг
			vw_StopTime();
			// выключаем музыку
			if (vw_GetMusicIsPlaying()) vw_ReleaseAllMusic();

			// если в игре, ставим паузу, т.е. открываем меню мгновенно
			if (GameStatus == GAME && (GameContentTransp < 1.0f)) {
				NeedShowGameMenu = true;
				NeedHideGameMenu = false;
				GameContentTransp = 1.0f;
				vw_SetTimeThreadSpeed(1, 0.0f);
				DrawGameCursor = true;
			}

			// ждем пока не поступит команда
			SDL_WaitEvent(nullptr);
		}
	}
GotoQuit:

	// если не выходим...
	if (!NeedReCreate && !CanQuit && Quit) {
		SetCurrentDialogBox(0);
		goto loop;
	}



	if (!NeedShowSystemCursor)
		SDL_ShowCursor(SDL_ENABLE);

	// завершение, и освобождение памяти...
	if (Script != nullptr) {
		delete Script;
		Script = nullptr;
	}

	RealExitGame();// удаляем объекты, они могут быть...
	MissionsListRelease();
	if (CursorParticleSystem2D != nullptr) {
		delete CursorParticleSystem2D;
		CursorParticleSystem2D = nullptr;
	}
	DestroyInfoObject();
	WorkshopDestroyData();

	if (psSpaceStatic != nullptr) {
		delete psSpaceStatic;
		psSpaceStatic = nullptr;
	}

	ReleaseAllObject3D();
	vw_ReleaseAllModel3D();
	vw_ReleaseAllParticleSystems();
	vw_ReleaseAllLights();
	ReleaseAllGameLvlText();

	vw_ReleaseAllFontChars(); /* call before vw_ReleaseAllTextures() */
	vw_ReleaseAllTextures();
	ShadowMap_Release();
	vw_ShutdownRenderer();

#ifdef joystick
	// закрываем джойстик, если он был
	if ((SDL_NumJoysticks() > 0) &&
	    (Joystick != nullptr) &&
	    (SDL_JoystickGetAttached(Joystick)))
		SDL_JoystickClose(Joystick);
#endif //joystick

	// полностью выходим из SDL
	SDL_Quit();
	// сохраняем настройки игры
	SaveXMLSetupFile();

	// если нужно перезагрузить игру с новыми параметрами
	if (NeedReCreate) {
		FirstStart = false;

		// убираем все голосовые файлы и звуки (для изменения языка голоса)
		// при vw_ShutdownSound освободим все, сейчас только речевые, музыка должна играть
		vw_ReleaseAllBuffers();

		// пересоздаем окно
		goto ReCreate;
	}


	if (VideoModes != nullptr) {
		delete [] VideoModes;
		VideoModes = nullptr;
	}


	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// завершение, освобождение памяти...
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// завершаем работу со звуком
	if (Setup.Music_check || Setup.Sound_check) ShutdownAudio();
	// освобождаем память выделенную под ттф шрифт
	vw_ShutdownFont();
	// освобождаем весь подготовленный текст из языкового файла
	vw_ReleaseText();
	// закрываем файловую систему
	vw_ShutdownVFS();
#ifdef fontconfig
	// освобождаем список шрифтов
	ReleaseFontConfig();
#endif // fontconfig

	// уходим из программы...
	return 0;

}