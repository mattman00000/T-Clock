//-------------------------------------------------------------------
//--+++--> PageMisc.c - Stoic Joker: Saturday, 06/05/2010 @ 10:46:02pm
//------------------------------------------------------------------*/
// Last Modified by Stoic Joker: Sunday, 03/13/2011 @ 11:54:05am
#include "tclock.h"

static void OnInit(HWND hDlg);
static void OnApply(HWND hDlg);

#define SendPSChanged(hDlg) SendMessage(GetParent(hDlg),PSM_CHANGED,(WPARAM)(hDlg),0)

#include "../common/calendar.inc"
typedef struct{
	const char* name;
	COLORREF color[CALENDAR_COLOR_NUM];
} CalendarPreset;

#define CALENDAR_PRESETS 3
static CalendarPreset m_calendar_preset[CALENDAR_PRESETS] = {
	{"Preset: default", {0}},
	{"Preset: high contrast", {0x000000, 0xFFFFFF, 0x000000, 0x000000, 0xF28E28, 0xEB00DD}},
	{"Preset: theme colored", {TCOLOR(TCOLOR_THEME), 0x000000, TCOLOR(TCOLOR_THEME), 0x000000, 0x808080, 0x808080}},
};
static char m_calendar_dirty;

static INT_PTR CALLBACK DlgProcCalendarColors(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	case WM_INITDIALOG:{
		ColorBox colors[6];
		HWND calendar = GetDlgItem(hDlg, IDC_CAL_PREVIEW);
		HWND preset_cb = GetDlgItem(hDlg, IDC_CAL_PRESET);
		int idx;
		m_calendar_dirty = 0;
		for(idx=0; idx<CALENDAR_COLOR_NUM; ++idx){
			unsigned color_id = CALENDAR_COLOR_BEGIN+(idx*2);
			colors[idx].hwnd = GetDlgItem(hDlg, color_id);
			colors[idx].color = api.GetInt("Calendar", g_calendar_color[idx].reg, TCOLOR(TCOLOR_DEFAULT));
			m_calendar_preset[0].color[idx] = (COLORREF)MonthCal_GetColor(calendar, g_calendar_color[idx].mcsc);
			if(colors[idx].color != TCOLOR(TCOLOR_DEFAULT)){
				m_calendar_dirty |= (1<<idx);
				MonthCal_SetColor(calendar, g_calendar_color[idx].mcsc, api.GetColor(colors[idx].color,0));
			}
		}
		ColorBox_Setup(colors, 6);
		for(idx=0; idx<CALENDAR_PRESETS; ++idx)
			ComboBox_AddString(preset_cb, m_calendar_preset[idx].name);
		ComboBox_AddString(preset_cb, "custom colors");
		m_calendar_dirty ^= 1; // toggle bit to force refresh
		SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_CAL_OUTER,CBN_SELCHANGE), (LPARAM)GetDlgItem(hDlg,IDC_CAL_OUTER));
		return TRUE;}
	case WM_MEASUREITEM:
		return ColorBox_OnMeasureItem(wParam, lParam);
	case WM_DRAWITEM:
		return ColorBox_OnDrawItem(wParam, lParam);
	case WM_COMMAND:{
		unsigned control_id = LOWORD(wParam);
		unsigned control_notify = HIWORD(wParam);
		switch(control_id){
		case IDC_CAL_PREVIEW:
			break;
		case IDC_CAL_PRESET:{
			HWND calendar = GetDlgItem(hDlg, IDC_CAL_PREVIEW);
			HWND preset_cb = GetDlgItem(hDlg, IDC_CAL_PRESET);
			int preset = ComboBox_GetCurSel(preset_cb);
			int idx;
			if(preset == CALENDAR_PRESETS)
				break;
			for(idx=0; idx<CALENDAR_COLOR_NUM; ++idx){
				unsigned color_id = CALENDAR_COLOR_BEGIN+(idx*2);
				HWND color_cb = GetDlgItem(hDlg, color_id);
				if(preset == 0)
					ColorBox_SetColor(color_cb, TCOLOR(TCOLOR_DEFAULT));
				else
					ColorBox_SetColor(color_cb, m_calendar_preset[preset].color[idx]);
				MonthCal_SetColor(calendar, g_calendar_color[idx].mcsc, api.GetColor(m_calendar_preset[preset].color[idx],0));
			}
			if(preset == 0)
				m_calendar_dirty = 0;
			else
				m_calendar_dirty = 0x01|0x02|0x04|0x08|0x10|0x20;
			SetXPWindowTheme(GetDlgItem(hDlg,IDC_CAL_PREVIEW), NULL, (m_calendar_dirty ? L"" : NULL));
			break;}
		case IDC_CAL_OUTER: case IDC_CAL_FORE: case IDC_CAL_BACK: case IDC_CAL_TITLE: case IDC_CAL_TITLE_BG: case IDC_CAL_TRAIL:
			if(control_notify == CBN_SELCHANGE){
				unsigned id = (control_id-CALENDAR_COLOR_BEGIN)/2;
				unsigned color = ColorBox_GetColorRaw((HWND)lParam);
				char dirty = m_calendar_dirty;
				if(color == TCOLOR(TCOLOR_DEFAULT))
					dirty &= ~(1<<id);
				else
					dirty |= 1<<id;
				if(dirty != m_calendar_dirty){
					m_calendar_dirty = dirty;
					SetXPWindowTheme(GetDlgItem(hDlg,IDC_CAL_PREVIEW), NULL, (dirty&~1 ? L"" : NULL));
				}
				ComboBox_SetCurSel(GetDlgItem(hDlg,IDC_CAL_PRESET), (dirty ? CALENDAR_PRESETS : 0));
				if(color == TCOLOR(TCOLOR_DEFAULT))
					color = m_calendar_preset[0].color[id];
				else
					color = api.GetColor(color, 0);
				MonthCal_SetColor(GetDlgItem(hDlg,IDC_CAL_PREVIEW), g_calendar_color[id].mcsc, color);
			}
			break;
		case IDOK:
			{int idx;
			for(idx=0; idx<CALENDAR_COLOR_NUM; ++idx){
				unsigned color_id = CALENDAR_COLOR_BEGIN+(idx*2);
				HWND color_cb = GetDlgItem(hDlg, color_id);
				api.SetInt("Calendar", g_calendar_color[idx].reg, ColorBox_GetColorRaw(color_cb));
			}}
			/* fall through */
		case IDCANCEL:
			EndDialog(hDlg, control_id);
			break;
		default:
			ColorBox_ChooseColor((HWND)lParam);
		}
		return TRUE;}
	}
	return FALSE;
}

//================================================================================================
//---------------------------------------------+++--> Dialog Procedure of Miscellaneous Tab Dialog:
INT_PTR CALLBACK PageMiscProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)   //------+++-->
{
	switch(message) {
	case WM_INITDIALOG:
		OnInit(hDlg);
		return TRUE;
		
	case WM_COMMAND: {
			WORD id, code;
			id = LOWORD(wParam);
			code = HIWORD(wParam);
			
			if(id==IDCB_CLOSECAL) {
				if(IsDlgButtonChecked(hDlg, IDCB_CALTOPMOST))
					CheckDlgButton(hDlg, IDCB_CALTOPMOST, FALSE);
			}else if(id==IDCB_CALTOPMOST) {
				if(IsDlgButtonChecked(hDlg, IDCB_CLOSECAL))
					CheckDlgButton(hDlg, IDCB_CLOSECAL, FALSE);
			}else if(id==IDCB_USECALENDAR) {
				UINT iter,enable=IsDlgButtonChecked(hDlg,IDCB_USECALENDAR);
				for(iter=GROUP_CALENDAR; iter<=GROUP_CALENDAR_END; ++iter) EnableDlgItem(hDlg,iter,enable);
				SendPSChanged(hDlg);
				return TRUE;
			}else if(id==IDC_CALCOLORS) {
				INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_DATE_CLASSES};
				InitCommonControlsEx(&icex);
				DialogBox(NULL,MAKEINTRESOURCE(IDD_CALENDAR_COLOR),hDlg,DlgProcCalendarColors);
			}
			
			if((id==IDC_FIRSTWEEK&&code==CBN_SELCHANGE) || (id==IDC_FIRSTDAY&&code==CBN_SELCHANGE) || (id==IDC_CALMONTHS&&code==EN_CHANGE) || (id==IDC_CALMONTHSPAST&&code==EN_CHANGE) || id==IDC_OLDCALENDAR)
				SendPSChanged(hDlg);
			else if(((id==IDCB_CLOSECAL) ||  // IF Anything Happens to Anything,
				(id==IDCB_SHOWWEEKNUMS) || //--+++--> Send Changed Message.
				(id==IDCB_SHOW_DOY) ||
				(id==IDCB_TRANS2KICONS) ||
				(id==IDCB_MONOFF_ONLOCK) ||
				(id==IDCB_MULTIMON) ||
				(id==IDCB_CALTOPMOST)) && (code==BST_CHECKED||code==BST_UNCHECKED)) {
				SendPSChanged(hDlg);
			}
			
			return TRUE;
		}
		
	case WM_NOTIFY:
		switch(((NMHDR*)lParam)->code) {
		case PSN_APPLY:
			OnApply(hDlg);
		}
		return TRUE;
	}
	return FALSE;
}
/**
 * \brief accesses <code>HKCR/Control Panel/International</code>; international user settings
 * \param entry entry to read, such as \e iFirstWeekOfYear, \e iFirstDayOfWeek
 * \return current value for given \p entry
 * \remark \e iFirstWeekOfYear should be either 0-2
 * \remark \e iFirstDayOfWeek ranges from 0-6, starting on Monday
 * \sa SetInternational() */
int GetInternationalInt(const char* entry)
{
	char val[8];
	api.GetSystemStr(HKEY_CURRENT_USER, "Control Panel\\International", entry, val, sizeof(val), "");
	return atoi(val);
}
/**
 * \brief writes to <code>HKCR/Control Panel/International</code>; international user settings
 * \param entry entry to write, such as \e iFirstWeekOfYear, \e iFirstDayOfWeek
 * \param val new value
 * \sa GetInternationalInt() */
void SetInternational(const char* entry, const char* val)
{
	api.SetSystemStr(HKEY_CURRENT_USER, "Control Panel\\International", entry, val);
}
//================================================================================================
//--------------------+++--> Initialize Properties Dialog & Customize T-Clock Controls as Required:
static void OnInit(HWND hDlg)   //----------------------------------------------------------+++-->
{
	HWND week_cb = GetDlgItem(hDlg, IDC_FIRSTWEEK);
	HWND day_cb = GetDlgItem(hDlg, IDC_FIRSTDAY);
	UINT iter;
	if(api.OS >= TOS_VISTA && !api.GetIntEx("Calendar","bCustom",0)){
		for(iter=GROUP_CALENDAR; iter<=GROUP_CALENDAR_END; ++iter) EnableDlgItem(hDlg,iter,0);
		CheckDlgButton(hDlg,IDCB_USECALENDAR, 0);
	}else CheckDlgButton(hDlg,IDCB_USECALENDAR, 1);
	if(api.OS >= TOS_WIN10){
		int old_calendar = api.GetSystemInt(HKEY_LOCAL_MACHINE, kSectionImmersiveShell, kKeyWin32Tray, 0);
		CheckDlgButton(hDlg, IDC_OLDCALENDAR, old_calendar);
	}else
		EnableDlgItem(hDlg, IDC_OLDCALENDAR, 0);
	/// on Calendar defaults change, also update the Calendar itself to stay sync!
	CheckDlgButton(hDlg, IDCB_SHOW_DOY, api.GetIntEx("Calendar","ShowDayOfYear",1));
	CheckDlgButton(hDlg, IDCB_SHOWWEEKNUMS, api.GetIntEx("Calendar","ShowWeekNums",0));
	CheckDlgButton(hDlg, IDCB_CLOSECAL, api.GetIntEx("Calendar","CloseCalendar",1));
	CheckDlgButton(hDlg, IDCB_CALTOPMOST, api.GetIntEx("Calendar","CalendarTopMost",0));
#	ifdef WIN2K_COMPAT
	CheckDlgButton(hDlg, IDCB_TRANS2KICONS, api.GetInt("Desktop","Transparent2kIconText",0));
#	endif // WIN2K_COMPAT
	CheckDlgButton(hDlg, IDCB_MONOFF_ONLOCK, api.GetInt("Desktop","MonOffOnLock",0));
	CheckDlgButton(hDlg, IDCB_MULTIMON, api.GetInt("Desktop","Multimon",1));
	
	SendDlgItemMessage(hDlg,IDC_CALMONTHSPIN,UDM_SETRANGE32,1,12);
	SendDlgItemMessage(hDlg,IDC_CALMONTHSPIN,UDM_SETPOS32,0,api.GetInt("Calendar","ViewMonths",3));
	SendDlgItemMessage(hDlg,IDC_CALMONTHPASTSPIN,UDM_SETRANGE32,0,2);
	SendDlgItemMessage(hDlg,IDC_CALMONTHPASTSPIN,UDM_SETPOS32,0,api.GetInt("Calendar","ViewMonthsPast",1));
	
	ComboBox_AddString(week_cb, "week containing January 1 (USA)");
	ComboBox_AddString(week_cb, "first full week");
	ComboBox_AddString(week_cb, "first week with four days (EU)");
	ComboBox_SetCurSel(week_cb, GetInternationalInt("iFirstWeekOfYear"));
	ComboBox_AddString(day_cb, "Monday");
	ComboBox_AddString(day_cb, "Tuesday");
	ComboBox_AddString(day_cb, "Wednesday");
	ComboBox_AddString(day_cb, "Thursday");
	ComboBox_AddString(day_cb, "Friday");
	ComboBox_AddString(day_cb, "Saturday");
	ComboBox_AddString(day_cb, "Sunday");
	ComboBox_SetCurSel(day_cb, GetInternationalInt("iFirstDayOfWeek"));
	
	if(api.OS > TOS_2000) {
		for(iter=IDCB_TRANS2KICONS_GRP; iter<=IDCB_TRANS2KICONS; ++iter)
			EnableDlgItem(hDlg,iter,FALSE);
	}else{
		for(iter=IDCB_MONOFF_ONLOCK_GRP; iter<=IDCB_MONOFF_ONLOCK; ++iter)
			EnableDlgItem(hDlg,iter,FALSE);
	}
	if(api.OS < TOS_WIN8){
		for(iter=IDCB_MULTIMON_GRP; iter<=IDCB_MULTIMON; ++iter)
			EnableDlgItem(hDlg,iter,FALSE);
		if(api.OS < TOS_VISTA){
			EnableDlgItem(hDlg, IDCB_USECALENDAR, FALSE);
		}
	}
}
//================================================================================================
//-------------------------//-----------------------------+++--> Save Current Settings to Registry:
void OnApply(HWND hDlg)   //----------------------------------------------------------------+++-->
{
	union{
		short i;
		char str[2];
	} intstr;
	char bRefresh = ((unsigned)api.GetInt("Desktop","Multimon",1) != IsDlgButtonChecked(hDlg,IDCB_MULTIMON));
	
	api.SetInt("Calendar","bCustom", IsDlgButtonChecked(hDlg,IDCB_USECALENDAR));
	api.SetInt("Calendar","CloseCalendar", IsDlgButtonChecked(hDlg,IDCB_CLOSECAL));
	api.SetInt("Calendar","ShowWeekNums", IsDlgButtonChecked(hDlg,IDCB_SHOWWEEKNUMS));
	api.SetInt("Calendar","ShowDayOfYear", IsDlgButtonChecked(hDlg,IDCB_SHOW_DOY));
	api.SetInt("Calendar","CalendarTopMost", IsDlgButtonChecked(hDlg,IDCB_CALTOPMOST));
	api.SetInt("Calendar","ViewMonths", (int)SendDlgItemMessage(hDlg,IDC_CALMONTHSPIN,UDM_GETPOS32,0,0));
	api.SetInt("Calendar","ViewMonthsPast", (int)SendDlgItemMessage(hDlg,IDC_CALMONTHPASTSPIN,UDM_GETPOS32,0,0));
#	ifdef WIN2K_COMPAT
	if(api.OS == TOS_2000) {
		int value = IsDlgButtonChecked(hDlg,IDCB_TRANS2KICONS);
		SetDesktopIconTextBk(value);
		api.SetInt("Desktop", "Transparent2kIconText", value);
		if(value)
			TimetableAdd(SCHEDID_WIN2K, 30, 30);
		else
			TimetableRemove(SCHEDID_WIN2K);
	}
#	endif // WIN2K_COMPAT
	api.SetInt("Desktop","MonOffOnLock", IsDlgButtonChecked(hDlg, IDCB_MONOFF_ONLOCK));
	api.SetInt("Desktop","Multimon", IsDlgButtonChecked(hDlg,IDCB_MULTIMON));
	
	intstr.i = '0' + (char)ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_FIRSTWEEK));
	SetInternational("iFirstWeekOfYear", intstr.str);
	intstr.i = '0' + (char)ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_FIRSTDAY));
	SetInternational("iFirstDayOfWeek", intstr.str);
	
	if(api.OS >= TOS_XP) { // This feature requires XP+
		BOOL enabled=IsDlgButtonChecked(hDlg, IDCB_MONOFF_ONLOCK);
		if(enabled){
			RegisterSession(g_hwndTClockMain);
		} else {
			UnregisterSession(g_hwndTClockMain);
		}
	}
	if(api.OS >= TOS_WIN10){
		int old_calendar = api.GetSystemInt(HKEY_LOCAL_MACHINE, kSectionImmersiveShell, kKeyWin32Tray, 0);
		if((int)IsDlgButtonChecked(hDlg, IDC_OLDCALENDAR) != old_calendar){
			char param[5] = "/Wc0";
			if(!old_calendar)
				param[3] = '1';
			if(api.ExecElevated(GetClockExe(), param, NULL) == 1)
				CheckDlgButton(hDlg, IDC_OLDCALENDAR, old_calendar);
		}
	}
	if(bRefresh){
		SendMessage(g_hwndClock,CLOCKM_REFRESHCLOCK,0,0);
	}
}
