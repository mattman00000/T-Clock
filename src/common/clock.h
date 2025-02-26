#ifndef CLOCK_API_H_
#define CLOCK_API_H_
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_API 1

#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#	define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#endif // WM_DWMCOLORIZATIONCOLORCHANGED

#define TCOLOR_MASK		0xFFFFFFC0
#define TCOLOR_MAGIC	0xFAFCFEC0
#define TCOLOR(tcolor) (TCOLOR_MAGIC|(unsigned)(tcolor))
enum TCOLORS {
	TCOLOR_BEGIN_		=50,
	TCOLOR_DEFAULT		=50, /**< meta, default color (if passed to \c GetColor() without \c use_raw, returns default clock foreground color) */
	TCOLOR_TRANSPARENT, /**< meta, fully transparent color */
	TCOLOR_THEME, /**< meta, current theme window color */
	TCOLOR_THEME_DARK, /**< meta, current theme window color (dark) */
	TCOLOR_THEME_ALPHA, /**< meta, current theme window color (alpha) */
	TCOLOR_THEME_BG, /**< meta, current theme's clock background color <b>failed attempt and useless</b> */
	TCOLOR_END_,
};

enum TOS {
	TOS_OLDER	=0x0000, /**< unknown (older) OS or error \sa TClockAPI::OS */
	TOS_2000	=0x0001, /**< does anyone still use it? \sa TClockAPI::OS */
	TOS_XP		=0x0002, /**< the king is dead, long live the king! \sa TClockAPI::OS */
	TOS_XP_64	=0x0004 | TOS_XP, /**< 64bit XP \sa TClockAPI::OS */
	TOS_VISTA	=0x0008, /**< first one with lots of cool stuff... poorly implemented though \sa TClockAPI::OS */
	TOS_WIN7	=0x0010, /**< best OS as of 2009-2015 as some might say \sa TClockAPI::OS */
	TOS_WIN8	=0x0020, /**< first to support multiple taskbars, yet buggy \sa TClockAPI::OS */
	TOS_WIN8_1	=0x0040, /**< first to require weird/stupid manifest... \sa TClockAPI::OS */
	TOS_WIN10	=0x0080, /**< latest, didn't add anything new to T-Clock yet \sa TClockAPI::OS */
	TOS_NEWER	=0x8000, /**< in case we're "outdated" and the curent OS is newer than our known ones \sa TClockAPI::OS */
};

typedef struct TClockAPI TClockAPI;
struct TClockAPI {
	HINSTANCE hInstance; /**< handle to T-Clock.dll */
/**
 * \brief holds current OS version flags
 * \sa TOS, TOS_2000, TOS_XP, TOS_XP_64, TOS_VISTA, TOS_WIN7, TOS_WIN8, TOS_WIN8_1, TOS_WIN10, TOS_NEWER, TOS_OLDER */
	unsigned short OS;
	unsigned short desktop_button_size; /**< size of the "show desktop" button (W7- 10px ?, W8 8px, W10 4+1px) */
	const char* root; /**< our root folder path w/o ending slash */
	size_t root_len; /**< length of our root folder path */
	
/**
 * \brief starts injection into explorer to replace clock
 * \param hwndMain handle to main/control window */
	void (*Inject)(HWND hwndMain);
/**
 * \brief finalize injection after it's done by removing our temporarily hook on explorer */
	void (*InjectFinalize)();
/**
 * \brief restore original clock and unload dll from explorer */
	void (*Exit)();
	
	// misc
	
/**
 * \brief get currently opened calendar \c HWND, if any
 * \return calendar \c HWND or \c NULL */
	HWND (*GetCalendar)();
/**
 * \brief displays a message box with clock icon
 * \param parent can be NULL
 * \param msg
 * \param title
 * \param uType any of \c MB_OK, \c MB_OKCANCEL, \c MB_RETRYCANCEL, \c MB_YESNO, \c MB_YESNOCANCEL, ...
 * \param uBeep \c MB_ICON* constant, \c MB_OK (default beep) or \c -1U ( \c 0xFFFFFFFF ) for silence
 * \return zero on error. Otherwise the button pressed such as \c IDOK, \c IDCANCEL
 * \sa MessageBox(), MessageBoxEx(), MessageBoxIndirect(), MessageBeep() */
	int (*Message)(HWND parent, const char* msg, const char* title, UINT uType, UINT uBeep);
/**
 * \brief positions a window near the clock
 * \param padding padding to use. \c 21 is our \e default, \c 11 is used for Win8's calendar */
	void (*PositionWindow)(HWND hwnd, int padding);
/**
 * \brief our GetTickCount64 helper that uses GetTickCount() as a fallback
 * \return elapsed milliseconds since system start
 * \sa GetTickCount(), GetTickCount64() */
	ULONGLONG (WINAPI *GetTickCount64)();
/**
 * \brief checks if given path exists
 * \return 1 for a file, 2 for directory */
	char (*PathExists)(const char* path);
/**
 * \brief "smartly" extracts filename and parameters from command (used by \c ExecFile())
 * \param[in] command command line to parse
 * \param[out] app buffer of size \c MAX_PATH that receives the application path
 * \param[out] params optional buffer of size \c MAX_PATH that receives parameters
 * \return 0 if a valid path was found
 * \remark this function tries to be smart, so spaces are generally ignored for as long as it finds a valid file
 * \sa ExecFile(), MAX_PATH */
	int (*GetFileAndOption)(const char* command, char* app, char* params);
/**
 * \brief parses given color ( \c COLORREF ) for use by T-Clock or Windows
 * \param color color to parse (can be either one of \c TCOLORS, a Windows system color or a user defined color)
 * \param use_raw \c 1 if raw color should be returned, eg. "unparsed" for some \c TCOLOR_* values that wouldn't be good to preview
 * \return parsed color value
 * \remark \a use_raw currently only applies to \c TCOLOR_DEFAULT and \c TCOLOR_TRANSPARENT
 * \remark \b we use the highest byte as a transparency value, ranging from 0-255, while 255 means fully transparent
 * \sa TCOLORS, On_DWMCOLORIZATIONCOLORCHANGED(), COLORREF, RGB(), GetSysColor() */
	unsigned (*GetColor)(unsigned color,int use_raw);
/**
 * \brief callback for use by \c WM_DWMCOLORIZATIONCOLORCHANGED messages to read current theme's color
 * \param argb wParam of \c WM_DWMCOLORIZATIONCOLORCHANGED
 * \param blend lParam of \c WM_DWMCOLORIZATIONCOLORCHANGED
 * \sa GetColor(), WM_DWMCOLORIZATIONCOLORCHANGED */
	void (*On_DWMCOLORIZATIONCOLORCHANGED)(unsigned argb, BOOL blend);
	
	// registry
	
/**
 * \brief read a int value from our registry
 * \param section,entry
 * \param defval default value to return
 * \return read int or defval */
	int (*GetInt)(const char* section, const char* entry, int defval);
/**
 * \brief read a int64_t value from our registry
 * \param section,entry
 * \param defval default value to return
 * \return read int64_t or \p defval */
	int64_t (*GetInt64)(const char* section, const char* entry, int64_t defval);
/**
 * \brief try to read a int value from our registry or add it if missing
 * \param section,entry
 * \param defval default value to write and return
 * \return read int or defval on failure */
	int (*GetIntEx)(const char* section, const char* entry, int defval);
/**
 * \brief read a int value from Windows' registry
 * \param rootkey,section,entry
 * \param defval default value to return if entry wasn't found
 * \return read int or defval on failure */
	int (*GetSystemInt)(HKEY rootkey, const char* section, const char* entry, int defval);
/**
 * \brief read a string value from our registry
 * \param[in] section,entry
 * \param[out] val output buffer of \a len size
 * \param[in] len size of \a val
 * \param[in] defval default value to return if \a entry wasn't found
 * \return size of returned string excl. zero terminator */
	int (*GetStr)(const char* section, const char* entry, char* val, int len, const char* defval);
/**
 * \brief try to read a string value from our registry or add it if missing
 * \param[in] section,entry
 * \param[out] val output buffer of \a len size
 * \param[in] len size of \a val
 * \param[in] defval default value to write and return if \a entry wasn't found
 * \return size of returned string excl. zero terminator */
	int (*GetStrEx)(const char* section, const char* entry, char* val, int len, const char* defval);
/**
 * \brief read a string value from Windows' registry
 * \param[in] rootkey,section,entry
 * \param[out] val output buffer of \a len size
 * \param[in] len size of \a val
 * \param[in] defval default value to return if \a entry wasn't found
 * \return size of returned string excl. zero terminator */
	int (*GetSystemStr)(HKEY rootkey, const char* section, const char* entry, char* val, int len, const char* defval);
/**
 * \brief update or add a int value in our registry
 * \param section,entry
 * \param val new value
 * \return boolean */
	int (*SetInt)(const char* section, const char* entry, int val);
/**
 * \brief update or add a int64_t value in our registry
 * \param section,entry
 * \param val new value
 * \return boolean */
	int (*SetInt64)(const char* section, const char* entry, int64_t val);
/**
 * \brief update or add a int value in Windows' registry
 * \param rootkey,section,entry
 * \param val new value
 * \return boolean */
	int (*SetSystemInt)(HKEY rootkey, const char* section, const char* entry, int val);
/**
 * \brief update or add a string value in our registry
 * \param section,entry
 * \param val new value
 * \return boolean */
	int (*SetStr)(const char* section, const char* entry, const char* val);
/**
 * \brief update or add a string value in Windows' registry
 * \param rootkey,section,entry
 * \param val new value
 * \return boolean */
	int (*SetSystemStr)(HKEY rootkey, const char* section, const char* entry, const char* val);
/**
 * \brief deletes a value from our registry
 * \param section
 * \param entry value to delete
 * \return boolean */
	int (*DelValue)(const char* section, const char* entry);
/**
 * \brief deletes a value from Windows' registry
 * \param section
 * \param entry value to delete
 * \return boolean */
	int (*DelSystemValue)(HKEY rootkey, const char* section, const char* entry);
/**
 * \brief deletes an entire key from our registry
 * \param section key to delete
 * \return boolean */
	int (*DelKey)(const char* section);
	
	// exec
	
/**
 * \brief a wrapper for ShellExecuteEx()
 * \param method "open", "runas", etc.
 * \param app path to run
 * \param params = \c NULL (optional program arguments)
 * \param parent = \c NULL (parent window)
 * \param show = \c SW_SHOWNORMAL
 * \return -1 on failure, 0 on success,1 if user canceled
 * \sa ShellExecute(), ShellExecuteEx(), Exec() */
	int (*ShellExecute)(const char* method, const char* app, const char* params, HWND parent, int show);
/**
 * \brief starts an application
 * \param app path to run
 * \param params = \c NULL (optional program arguments)
 * \param parent = \c NULL (parent window)
 * \return -1 on failure, 0 on success, 1 if user canceled
 * \sa ExecElevated(), ExecFile(), ShellExecute() */
	int (*Exec)(const char* app, const char* params, HWND parent);
/**
 * \brief starts an application elevated (displays UAC dialog when required)
 * \return -1 on failure, 0 on success, 1 if user canceled
 * \remarks this function is mainly for Vista+, though even Win2000 shows an user logon screen
 * \sa Exec(), ExecFile(), ShellExecute() */
	int (*ExecElevated)(const char* app, const char* params, HWND parent);
/**
 * \brief opens a file or starts an application
 * \param command full command-line with file-name and optional arguments
 * \param parent = \c NULL (parent window)
 * \return -1 on failure, 0 on success, 1 if user canceled
 * \remark makes use of \c GetFileAndOption() internally and thus same rules apply for \p command
 * \sa Exec(), ExecElevated(), ShellExecute(), GetFileAndOption() */
	int (*ExecFile)(const char* command, HWND parent);
	// format stuff
/**
 * \brief retrieves a format specifier, width and padding; starting from \p offset[0]
 * \param[in,out] offset current offset of format input
 * \param[out] minimum minimum width (padding zeros)
 * \param[out] padding white-space padding
 * \return parsed format specifier eg. 'h' or \0 if end was reached
 * \remark \p offset will point to next format position or \0 if end was reached */
	char (*GetFormat)(const char** offset, int* minimum, int* padding);
/**
 * \brief writes \p number to string \p buffer
 * \param[out] buffer
 * \param[in] number
 * \param[in] minimum pads number with zeros to reach given minimum
 * \param[in] padding additional white-space padding
 * \return chars written (excl. \0) */
	int (*WriteFormatNum)(char* buffer, int number, int minimum, int padding);
	// translation API
	const char* (*T)(int hash);
	const char* (*Translate)(const char* str);
	const char* (*TranslateWindow)(HWND hwnd);
};

/**
 * \brief load and initialize T-Clock.dll
 * \param dll_path path to our T-Clock[64].dll
 * \param api reference to our API struct that receives functions
 * \return non-zero on error. greater than 0 if internal API failure, lower than 0 if external */
int LoadClockAPI(const char* dll_path, TClockAPI* api);

extern TClockAPI api;

#ifdef __cplusplus
}
#endif
#endif // CLOCK_API_H_
