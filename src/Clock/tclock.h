//===============================================================================
//--+++--> tclock.h - KAZUBON  1997-1999 =========================================
//=================== Last Modified by Stoic Joker: Sunday, 03/13/2011 @ 11:54:05am
//-------------------------{ Stoic Joker 2006-2010 }-------------------------+++-->
#pragma once

#include "../common/globals.h"
#include "../common/resource.h"

#include <stdio.h>//sprintf
#include <uxtheme.h>//SetWindowTheme
#include <shlwapi.h>//PathFileExists
#include <psapi.h>//EmptyWorkingSet
#include "../common/newapi.h"
#include "../common/utl.h"
#include "../common/control_extensions.h"

// IDs for timer
#define IDTIMER_MAIN				3
#define IDTIMER_MOUSE				4
#define IDTIMER_DEKSTOPICON			5
#define IDTIMER_DESKTOPICONSTYLE	6

// for mouse.c and pagemouse.c
#define MOUSEFUNC_NONE           0
#define MOUSEFUNC_MENU           1
#define MOUSEFUNC_TIMER          5
#define MOUSEFUNC_CLIPBOARD      6
#define MOUSEFUNC_SCREENSAVER    7
#define MOUSEFUNC_SHOWCALENDER   8
#define MOUSEFUNC_SHOWPROPERTY   9

#define MOUSEFUNCEXTRA_BEGIN     -1
#define MOUSEFUNCEXTRAFILE_BEGIN -100
#define MOUSEFUNC_EXEC           -100

//--+++--> main.c - Application Global Values:
extern HWND g_hwndTClockMain; /**< our main window for hotkeys, menus and sounds */
extern HWND g_hwndClock;      /**< the clock hwnd */
extern HWND g_hDlgTimer;      /**< timer dialog handle */
extern HWND g_hDlgStopWatch;  /**< stopwatch dialog handle */
extern HWND g_hDlgTimerWatch; /**< timer watch dialog handle */
extern HWND g_hDlgSNTP;       /**< SNTP options dialog handle */
extern HWND g_hwndSheet;      /**< property sheet window */
/** frequently used icon handles */
extern HICON g_hIconTClock, g_hIconPlay, g_hIconStop, g_hIconDel;
#ifdef WIN2K_COMPAT
/** make background of desktop icon text labels transparent:
 * (for Windows 2000 only)
 * UnAdvertized EasterEgg Function */
void SetDesktopIconTextBk(int enable);
#endif // WIN2K_COMPAT

/**
 * \brief returns full path to currently started Clock[64].exe
 * \return path to Clock.exe incl. filename
 * \remark currently just a wrapper for \c _pgmptr */
//inline const char* GetClockExe(){
//	return _pgmptr;
//}
#define GetClockExe() _pgmptr

void RegisterSession(HWND hwnd);
void UnregisterSession(HWND hwnd);
void ToggleCalendar(int type);
int GetStartupFile(HWND hDlg,char filename[MAX_PATH]);
void AddStartup(HWND hDlg);
void RemoveStartup(HWND hDlg);
int CreateLink(LPCSTR fname, LPCSTR dstpath, LPCSTR name);

// used by PageMisc.c and main.c
extern const char kSectionImmersiveShell[56+1]; ///< SOFTWARE/Microsoft/Windows/CurrentVersion/ImmersiveShell
extern const char kKeyWin32Tray[27+1]; ///< UseWin32TrayClockExperience

// settings.c
int CheckSettings();

// propsheet.c
extern char g_bApplyClock;
extern char g_bApplyTaskbar;
void MyPropertySheet(int page);
BOOL SelectMyFile(HWND hDlg, const char* filter, DWORD nFilterIndex, const char* deffile, char* retfile);

// alarm.c
typedef struct Schedule Schedule;
struct Schedule {
	Schedule* prev;
	Schedule* next;
	time_t time;
	int id;
	unsigned data;
};

/**
 * \brief add a schedule to our timetable
 * \param id schedule ID, should have the \c SCHEDID_START_FLAG_ flag as everything else is used for alarms
 * \param ts target time_t or time offset from 0 to 86400 (1 day)
 * \param data schedule specific data (such as ALRM_* flags for hourly chime)
 * \return pointer to queued schedule or \c NULL on failure
 * \sa TimetableRemove(), AlarmEnable() */
Schedule* TimetableAdd(int id, time_t ts, unsigned data);
void TimetableRemove(int id);
void TimetableQueue(Schedule* alert, int add);
Schedule* TimetableSearchID(int id);

enum{
	SCHEDID_START_FLAG_ = 0x80000000,
	SCHEDID_CHIME,
	SCHEDID_UPDATE,
	SCHEDID_WIN2K,
};

#define MAX_ALARM 999999
enum{
	DAYF_MONDAY   = 0x01, DAYF_TUESDAY = 0x02, DAYF_WEDNESDAY = 0x04,
	DAYF_THURSDAY = 0x08, DAYF_FRIDAY  = 0x10, DAYF_SATURDAY  = 0x20, DAYF_SUNDAY = 0x40,
	DAYF_DAILY     = /*0x7f*/ (DAYF_MONDAY | DAYF_TUESDAY | DAYF_WEDNESDAY | DAYF_THURSDAY | DAYF_FRIDAY | DAYF_SATURDAY | DAYF_SUNDAY),
	DAYF_OVERFLOW  = 0x80,
};
#define DAYF(x) (1 << (x))
#define DAYF_FromWDay(x) (((x) > 0 ? DAYF((x)-1) : DAYF_SUNDAY))
enum{
	ALRM_ENABLED =0x00000001,
	ALRM_ONESHOT =0x00000002,
	ALRM_12H     =0x00000004,
	ALRM_CHIMEHR =0x00000010,
	ALRM_REPEAT  =0x00000020,
	ALRM_BLINK   =0x00000040,
	ALRM_DIALOG  =0x00000080,
};
typedef struct{
	char name[TNY_BUFF];
	char message[512];
	char settings[TNY_BUFF];
} dlgmsg_t;
typedef struct alarm_t {
	int days;
	int hour;
	int minute;
	int iTimes;
	dlgmsg_t dlgmsg;
	unsigned char uFlags;
	char fname[MAX_PATH];
} alarm_t;

/**
 * \brief enable / disable alarm
 * \param enable 1: enable, 0: disable, -1: toggle, -2: read from reg */
void AlarmEnable(int idx, int enable);
/**
 * \brief enable / disable hourly chime
 * \param enable 1: enable, 0: disable, -1: toggle, -2: read from reg */
void AlarmChimeEnable(int enable);

int GetAlarmNum();
void SetAlarmNum(int num);
void ReadAlarmFromReg(alarm_t* pAS, int idx);
void SaveAlarmToReg(alarm_t* pAS, int idx);
int DeleteAlarmFromReg(int idx);

void StopFile();
int IsPlaying();
void EndAlarm();
void InitAlarm();
int OnMCINotify(HWND hwnd);
void OnTimerAlarm(HWND hwnd, time_t time);
/** \brief play sound or execute program \p fname
 * \param[in] hwnd
 * \param[in] fname sound to be played, or program to be executed. Can be relative to \e TCLOCK/waves/
 * \param[in] dwLoops number of times to repeat
 * \return boolean (started to play a sound) */
BOOL PlayFile(HWND hwnd, const char* fname, DWORD dwLoops);

// alarmday.c
#define ALARMDAY_OKFLAG 0x80000000
int ChooseAlarmDay(HWND hDlg, unsigned days);

// soundselect.c
/**
 * \brief adds a list of sound files from \e /waves/ to given combobox controls
 * \param boxes[] array of combobox controls
 * \param num number of controls in \a boxes
 * \remark first entry is always "&lt;  no sound  &gt;" and will be selected if combobox is empty */
void ComboBoxArray_AddSoundFiles(HWND boxes[], int num);
BOOL IsMMFile(const char* fname);
BOOL BrowseSoundFile(HWND hDlg, const char* deffile, char* fname);

// pageformat.c
void InitFormat();

// menu.c
LRESULT OnTClockCommand(HWND hwnd, WPARAM wParam);
void OnContextMenu(HWND hwnd, int xPos, int yPos);

// mouse.c
void OnTimerMouse(HWND hwnd);
void OnMouseMsg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
// PageMouse.c
void CheckMouseMenu();

// timer.c
void UpdateTimerMenu(HMENU hMenu);

void StopTimer(int id);
void StartTimer(int id);
void ToggleTimer(int id);

void WatchTimer(int reset);
void CancelAllTimersOnStartUp();

void EndAllTimers();
void DialogTimer();
void OnTimerTimer(HWND hwnd);

// StopWatch.c
BOOL IsDialogStopWatchMessage(HWND hwnd, MSG* msg);
void DialogStopWatch();
void StopWatch_Start(HWND hDlg);
void StopWatch_Stop(HWND hDlg);
void StopWatch_Reset(HWND hDlg);
void StopWatch_Pause(HWND hDlg);
void StopWatch_Resume(HWND hDlg);
void StopWatch_Lap(HWND hDlg,int bFromStop);
void StopWatch_TogglePause(HWND hDlg);

// ExitWindows.c
BOOL ShutDown();
BOOL ReBoot();
BOOL LogOff();

// PageHotKey.c
typedef union {
	struct {
		uint8_t vk;
		uint8_t fsMod;
	} key;
	uint16_t word;
} hotkey_t;
hotkey_t GetHotkey(int idx);
void SetHotkey(int idx, hotkey_t hotkey);
void HotkeyBox_Init(HWND hDlg, int idx);
hotkey_t HotkeyBox_GetValue(HWND box);
void HotkeyBox_SetValue(HWND box, hotkey_t hotkey);
/**
 * \brief (un-)registers hotkeys
 * \param hwnd target hotkey window
 * \param want_register \c 0 to remove them, \c 1 to add them, \c 2 for re-registration
 * \sa RegisterHotKey(), UnregisterHotKey() */     
void RegisterHotkeys(HWND hwnd, int want_register);

// SNTP.c
void SyncTimeNow();
void NetTimeConfigDialog(int justElevated);

// BounceWind.c
int BounceWindOptions(HWND hDlg, dlgmsg_t* dlg);
void ReleaseTheHound(HWND hwnd, const char* title, const char* text, char* settings);

// Macros
BOOL EnableDlgItemSafeFocus(HWND hDlg,int control,BOOL bEnable,int nextFocus);
#define EnableDlgItem(hDlg,id,b) EnableWindow(GetDlgItem((hDlg),(id)),(b))
#define ShowDlgItem(hDlg,id,b) ShowWindow(GetDlgItem((hDlg),(id)),(b)?SW_SHOW:SW_HIDE)

