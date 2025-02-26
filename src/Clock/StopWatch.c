// Written by Stoic Joker: Tuesday, 03/16/2010 @ 10:18:59pm
// Modified by Stoic Joker: Monday, 03/22/2010 @ 7:32:29pm
#include "tclock.h"
//#define TIMETEXT_DEFAULT "00 h 00 m 00 s 000 ms"
//#define TIMETEXT_FORMAT "%02d h %02d m %02lu s %03lu ms"
#define TIMETEXT_DEFAULT "00:00:00.000"
#define TIMETEXT_FORMAT "%02d:%02d:%02lu.%03lu"
static LARGE_INTEGER m_frequency={{0}};
static LARGE_INTEGER m_start;// start time
static LARGE_INTEGER m_lap;// latest lap time
static LARGE_INTEGER m_stop;// latest lap time
static char m_paused; // Global Pause/Resume Displayed Counter.
// resize vars
static int m_rezCX; // original dialog width (fixed)
static int m_rezCY; // original dialog height (minimal)
static int m_rezYcontrols;
static int m_rezCXlist;
static int m_rezCYlist;

static INT_PTR CALLBACK DlgProcStopwatch(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DlgProcStopwatchExport(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void OnTimer(HWND hDlg);

//================================================================================================
// -------------------------------------------------------------------+++--> Open Stopwatch Dialog:
void DialogStopWatch()   //--------------------------------------------------------+++-->
{
	CreateDialogParamOnce(&g_hDlgStopWatch, 0, MAKEINTRESOURCE(IDD_STOPWATCH), NULL, DlgProcStopwatch, 0);
}

BOOL IsDialogStopWatchMessage(HWND hwnd, MSG* msg){ // handles hotkeys
	int msgid=LOWORD(msg->message);
	if(msgid==WM_KEYDOWN || msgid==WM_KEYUP){
		if(msg->hwnd==hwnd || GetParent(msg->hwnd)==hwnd){
			int control;
			switch(msg->wParam){
			case 'S':
				control=IDC_SW_START;
				break;
			case 'W':
				control=IDC_SW_RESET;
				break;
			case 'A':
				control=IDC_SW_LAP;
				break;
			case 'E':
				control=IDC_SW_EXPORT;
				break;
			default:
				return IsDialogMessage(hwnd,msg);
			}
			if(msgid==WM_KEYDOWN && !(msg->lParam&0x40000000)){ // was up before, not repeated
				SendMessage(hwnd,WM_COMMAND,control,0);
			}
			return 1;
		}
	}
	return IsDialogMessage(hwnd,msg);
}
void StopWatch_Start(HWND hDlg){
	if(m_start.QuadPart){
		KillTimer(hDlg,1);
	}
	ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_SW_LAPS));
	QueryPerformanceCounter(&m_start);
	m_lap=m_start;
	m_paused = 0;
	SetTimer(hDlg,1,7,NULL);
	SetDlgItemText(hDlg,IDC_SW_START,"Stop (s)");
	EnableDlgItem(hDlg,IDC_SW_RESET,1);
}
void StopWatch_Stop(HWND hDlg){
	if(!m_start.QuadPart) return;
	KillTimer(hDlg,1);
	m_paused = 1;
	OnTimer(hDlg); // update time text
	m_start.QuadPart=0;
	SetDlgItemText(hDlg,IDC_SW_START,"Start (s)");
	EnableDlgItemSafeFocus(hDlg,IDC_SW_RESET,0,IDC_SW_START);
}
void StopWatch_Reset(HWND hDlg){
	if(!m_start.QuadPart) return;
	SetDlgItemText(hDlg,IDC_SW_ELAPSED,TIMETEXT_DEFAULT);
	ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_SW_LAPS));
	if(m_paused){ // paused
		m_start.QuadPart=0;
		EnableDlgItemSafeFocus(hDlg,IDC_SW_RESET,0,IDC_SW_START);
	}else{ // running
		QueryPerformanceCounter(&m_start);
		m_lap=m_start;
	}
}
void StopWatch_Pause(HWND hDlg){
	if(m_paused) return;
	KillTimer(hDlg,1);
	QueryPerformanceCounter(&m_stop);
	m_paused = 1;
	OnTimer(hDlg); // update time text
	StopWatch_Lap(hDlg,1);
	SetDlgItemText(hDlg,IDC_SW_START,"Start (s)");
}
void StopWatch_Resume(HWND hDlg){
	LARGE_INTEGER end,diff;
	if(!m_paused) return;
	if(!m_start.QuadPart){
		StopWatch_Start(hDlg);
		return;
	}
	QueryPerformanceCounter(&end);
	diff.QuadPart=end.QuadPart-m_stop.QuadPart;
	m_start.QuadPart+=diff.QuadPart;
	if(m_lap.QuadPart)
		m_lap.QuadPart+=diff.QuadPart;
	else
		m_lap=end;
	m_paused = 0;
	SetTimer(hDlg,1,7,NULL);
	SetDlgItemText(hDlg,IDC_SW_START,"Stop (s)");
}
void StopWatch_TogglePause(HWND hDlg){
	if(m_paused){
		StopWatch_Resume(hDlg);
	}else{
		StopWatch_Pause(hDlg);
	}
}
void StopWatch_Lap(HWND hDlg,int bFromStop){ // Get Current Time as Lap Time and Add it to the ListView Control
	char buf[TNY_BUFF];
	int hrs, min;
	LVITEM lvItem; // ListView Control Row Identifier
	LARGE_INTEGER end;
	unsigned long elapsed;
	HWND hList=GetDlgItem(hDlg,IDC_SW_LAPS);
	
	if(!m_start.QuadPart || !m_lap.QuadPart)
		return;
	QueryPerformanceCounter(&end);
	if(m_paused)
		end=m_stop;
	elapsed=(unsigned long)((end.QuadPart-m_lap.QuadPart)*1000/m_frequency.QuadPart);
	if(m_paused)
		m_lap.QuadPart=0;
	else
		m_lap=end;
	
	wsprintf(buf,"Lap %d",ListView_GetItemCount(hList)+1);
	if(bFromStop)
		strcat(buf," [S]");
	lvItem.mask=LVIF_TEXT;
	lvItem.iSubItem=0;
	lvItem.iItem=0;
	lvItem.pszText=buf;
	ListView_InsertItem(hList,&lvItem);
	
	hrs=elapsed/3600000; elapsed%=3600000;
	min=elapsed/60000; elapsed%=60000;
	wsprintf(buf,"%02d:%02d:%02lu.%03lu",hrs,min,elapsed/1000,elapsed%1000);
	lvItem.iSubItem=1;
	ListView_SetItem(hList,&lvItem);
	
	elapsed=(unsigned long)((end.QuadPart-m_start.QuadPart)*1000/m_frequency.QuadPart);
	hrs=elapsed/3600000; elapsed%=3600000;
	min=elapsed/60000; elapsed%=60000;
	wsprintf(buf,"%02d:%02d:%02lu.%03lu",hrs,min,elapsed/1000,elapsed%1000);
	lvItem.iSubItem=2;
	ListView_SetItem(hList,&lvItem);
}
//================================================================================================
// ----------------------------------------------------+++--> Initialize Stopwatch Dialog Controls:
static void OnInit(HWND hDlg)   //-----------------------------------------------------------------+++-->
{
	RECT rc;
	LVCOLUMN lvCol;
	HWND hList=GetDlgItem(hDlg,IDC_SW_LAPS);
	/// basic init
	m_paused=1;
	m_start.QuadPart=0;
	QueryPerformanceFrequency(&m_frequency);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL,(LPARAM)g_hIconTClock);
	SendMessage(hDlg, WM_SETICON, ICON_BIG,(LPARAM)g_hIconTClock);
	/// init resize info
	GetWindowRect(hDlg,&rc);
	m_rezCX=rc.right-rc.left;
	m_rezCY=rc.bottom-rc.top;
	GetWindowRect(GetDlgItem(hDlg,IDC_SW_START),&rc);
	ScreenToClient(hDlg,(POINT*)&rc);
	m_rezYcontrols=rc.top;
	GetWindowRect(hList,&rc);
	m_rezCYlist=rc.bottom-rc.top;
	m_rezCXlist=rc.right-rc.left;
	rc.bottom=api.GetInt("Timers","SwSize",0);
	if(rc.bottom){
		SetWindowPos(hDlg,HWND_TOP,0,0,m_rezCX,rc.bottom,SWP_NOMOVE|SWP_NOZORDER);
	}
	/// init list view
	ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
	SetXPWindowTheme(hList,L"Explorer",NULL);
	
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.cx = 65; // Column Width
	lvCol.iSubItem = 0;
	lvCol.fmt = LVCFMT_CENTER;
	lvCol.pszText = TEXT("Lap");
	ListView_InsertColumn(hList,0,&lvCol);
	
	lvCol.cx = 85;
	lvCol.iSubItem = 1;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = TEXT("Time");
	ListView_InsertColumn(hList,1,&lvCol);
	
	lvCol.cx = 85;
	lvCol.iSubItem = 2;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = TEXT("Total");
	ListView_InsertColumn(hList,2,&lvCol);
	/// init font
	{
		LOGFONT logfont;
		HFONT hfont;
		hfont=(HFONT)SendMessage(hDlg,WM_GETFONT,0,0);
		GetObject(hfont,sizeof(LOGFONT),&logfont);
		logfont.lfHeight=logfont.lfHeight*145/100;
		logfont.lfWeight=FW_BOLD;
		hfont=CreateFontIndirect(&logfont);
		SendDlgItemMessage(hDlg,IDC_SW_ELAPSED,WM_SETFONT,(WPARAM)hfont,0);
		hfont=(HFONT)SendMessage(hDlg,WM_GETFONT,0,0);
		GetObject(hfont,sizeof(LOGFONT),&logfont);
		logfont.lfHeight=logfont.lfHeight*160/100;
		hfont=CreateFontIndirect(&logfont);
		SendDlgItemMessage(hDlg,IDC_SW_START,WM_SETFONT,(WPARAM)hfont,0);
		SendDlgItemMessage(hDlg,IDC_SW_RESET,WM_SETFONT,(WPARAM)hfont,0);
	}
	SetDlgItemText(hDlg, IDC_SW_ELAPSED, TIMETEXT_DEFAULT);
}
//================================================================================================
//-------------------------//------------------+++--> Updates the Stopwatch's Elapsed Time Display:
static void OnTimer(HWND hDlg)   //----------------------------------------------------------------+++-->
{
	char szElapsed[TNY_BUFF];
	int hrs, min;
	union{
		unsigned long elapsed;
		LARGE_INTEGER end;// latest lap time
	} un;
	
	QueryPerformanceCounter(&un.end);
	un.end.QuadPart-=m_start.QuadPart;
	un.elapsed=(unsigned long)(un.end.QuadPart*1000/m_frequency.QuadPart);
	
	hrs=un.elapsed/3600000; un.elapsed%=3600000;
	min=un.elapsed/60000; un.elapsed%=60000;
	wsprintf(szElapsed,TIMETEXT_FORMAT,hrs,min,un.elapsed/1000,un.elapsed%1000);
	SetDlgItemText(hDlg, IDC_SW_ELAPSED, szElapsed);
}
//================================================================================================
// --------------------------------------------------+++--> Message Processor for Stopwatch Dialog:
static INT_PTR CALLBACK DlgProcStopwatch(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)   //------+++-->
{
	switch(msg) {
	case WM_INITDIALOG:
		OnInit(hDlg);
		api.PositionWindow(hDlg,21);
		return TRUE;
	case WM_DESTROY:{
		// save pos & size
		RECT rc; GetWindowRect(hDlg,&rc);
		rc.bottom=rc.bottom-rc.top;
		if(rc.bottom!=m_rezCY){
			api.SetInt("Timers","SwSize",rc.bottom);
		}else{
			api.DelValue("Timers","SwSize");
		}
		// cleaup elapsed font
		{HFONT hfont=(HFONT)SendDlgItemMessage(hDlg,IDC_SW_ELAPSED,WM_GETFONT,0,0);
		SendDlgItemMessage(hDlg,IDC_SW_ELAPSED,WM_SETFONT,0,0);
		DeleteObject(hfont);
		// cleanup button font
		hfont=(HFONT)SendDlgItemMessage(hDlg,IDC_SW_START,WM_GETFONT,0,0);
		SendDlgItemMessage(hDlg,IDC_SW_START,WM_SETFONT,0,0);
		SendDlgItemMessage(hDlg,IDC_SW_RESET,WM_SETFONT,0,0);
		DeleteObject(hfont);}
		break;}
	/// handling
	case WM_ACTIVATE:
		if(LOWORD(wParam)==WA_ACTIVE || LOWORD(wParam)==WA_CLICKACTIVE){
			SetWindowPos(hDlg,HWND_TOPMOST_nowarn,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		}else{
			SetWindowPos(hDlg,HWND_NOTOPMOST_nowarn,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// actually it should be lParam, but that's "always" NULL for other process' windows
			SetWindowPos(GetForegroundWindow(),HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		}
		break;
	case WM_CTLCOLORSTATIC:
		if((HWND)lParam!=GetDlgItem(hDlg,IDC_SW_ELAPSED))
			break;
		SetTextColor((HDC)wParam,0x00000000);
		SetBkColor((HDC)wParam,0x00FFFFFF);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	/// resizing
	case WM_WINDOWPOSCHANGING:{
		WINDOWPOS* info=(WINDOWPOS*)lParam;
		if(!(info->flags&SWP_NOSIZE)){
			if(info->cx!=m_rezCX || info->cy<m_rezCY){
				RECT rc; GetWindowRect(hDlg,&rc);
				if(info->cx!=m_rezCX){
					info->cx=m_rezCX;
					info->x=rc.left;
				}
				if(info->cy<m_rezCY){
					info->cy=m_rezCY;
					info->y=rc.top;
				}
			}
		}
		return TRUE;}
	case WM_WINDOWPOSCHANGED:{
		WINDOWPOS* info=(WINDOWPOS*)lParam;
		if(!(info->flags&SWP_NOSIZE)){
			int diff=info->cy-m_rezCY;
			int control;
			for(control=IDC_SW_START; control<=IDC_SW_EXPORT; ++control){
				HWND hwnd=GetDlgItem(hDlg,control);
				RECT rc; GetWindowRect(hwnd,&rc);
				ScreenToClient(hDlg,(POINT*)&rc);
				SetWindowPos(hwnd,HWND_TOP,rc.left,m_rezYcontrols+diff,0,0,SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);
			}
			SetWindowPos(GetDlgItem(hDlg,IDC_SW_LAPS),HWND_TOP,0,0,m_rezCXlist,m_rezCYlist+diff,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
		}
		return TRUE;}
	/// user interaction
	case WM_TIMER:
		if(!m_paused)
			OnTimer(hDlg);
		return TRUE;
	case WM_COMMAND: {
			WORD id = LOWORD(wParam);
			switch(id) {
			case IDC_SW_START: // Start/Stop
				StopWatch_TogglePause(hDlg);
				break;
			case IDC_SW_RESET:
				StopWatch_Reset(hDlg);
				break;
			case IDC_SW_EXPORT:
				DialogBox(0,MAKEINTRESOURCE(IDD_STOPWATCH_EXPORT),hDlg,DlgProcStopwatchExport);
				break;
			case IDC_SW_LAP:
				StopWatch_Lap(hDlg,0);
				break;
			case IDCANCEL:
				KillTimer(hDlg, 1);
				g_hDlgStopWatch = NULL;
				DestroyWindow(hDlg);
			}
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL SaveFileDialog(HWND hDlg, char* file /*in/out*/, int filebuflen)
{
	OPENFILENAME ofn={sizeof(OPENFILENAME)};
	ofn.hwndOwner=hDlg;
	ofn.lpstrFile=file;
	ofn.nMaxFile=filebuflen;
	ofn.Flags=OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
	return GetSaveFileName(&ofn);
}
static void export_print(char** out, const char* fmt, const char* time, int num, const char* lap, const char* lapflags){
	const char* pos;
	for(pos=fmt; *pos; ++pos){
		if(*pos=='\\'){
			++pos;
			switch(*pos){
			case 'n': // new line
				*out+=wsprintf(*out,"\r\n");
				break;
			case 't': // total time
				*out+=wsprintf(*out,time);
				break;
			case '#': // (lap) num
				*out+=wsprintf(*out,"%2i",num);
				break;
			case 'l': // lap time
				*out+=wsprintf(*out,lap);
				break;
			case 'f': // lap flags (currently [S] only)
				*out+=wsprintf(*out,lapflags);
				break;
			default:
				**out=*--pos; ++(*out);
			}
			continue;
		}
		**out=*pos; ++(*out);
	}
}
static void export_text(HWND hDlg){
	HWND hList=GetDlgItem(GetParent(hDlg),IDC_SW_LAPS);
	int laps=ListView_GetItemCount(hList);
	int iter;
	char* buf,* bufpos;
	char total[128], lap[128], totaltime[32], laptime[32], lapflags[16];
	GetDlgItemText(hDlg,IDC_SWE_TOTAL,total,sizeof(total));
	GetDlgItemText(hDlg,IDC_SWE_LAP,lap,sizeof(lap));
	buf=bufpos=malloc(32 + strlen(total) + ((strlen(lap)+32)*laps));
	if(total[0]!='\\' || total[1]!='n'){
		lapflags[0]='\0';
		GetDlgItemText(GetParent(hDlg),IDC_SW_ELAPSED,totaltime,sizeof(totaltime));
		export_print(&bufpos,total,totaltime,laps,totaltime,lapflags);
	}
	for(iter=0; iter<laps; ++iter){
		ListView_GetItemText(hList,laps-1-iter,0,lapflags,sizeof(lapflags));
		if(strchr(lapflags,'[')){
			strcpy(lapflags,strchr(lapflags,'[')-1);
		}else
			lapflags[0]='\0';
		ListView_GetItemText(hList,laps-1-iter,1,laptime,sizeof(laptime));
		ListView_GetItemText(hList,laps-1-iter,2,totaltime,sizeof(totaltime));
		export_print(&bufpos,lap,totaltime,iter+1,laptime,lapflags);
	}
	if(total[0]=='\\' && total[1]=='n'){
		lapflags[0]='\0';
		GetDlgItemText(GetParent(hDlg),IDC_SW_ELAPSED,totaltime,sizeof(totaltime));
		export_print(&bufpos,total+2,totaltime,laps,totaltime,lapflags);
		bufpos+=wsprintf(bufpos,"\r\n");
	}
	*bufpos='\0';
	SetDlgItemText(hDlg,IDC_SWE_OUT,buf);
	free(buf);
}
static INT_PTR CALLBACK DlgProcStopwatchExport(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam; // unused
	switch(msg) {
	case WM_INITDIALOG:{
		char buf[128];
		api.GetStr("Timers","SwExT",buf,sizeof(buf),"");
		SetDlgItemText(hDlg,IDC_SWE_TOTAL,buf);
		api.GetStr("Timers","SwExL",buf,sizeof(buf),"");
		SetDlgItemText(hDlg,IDC_SWE_LAP,buf);
		SendMessage(hDlg,WM_COMMAND,IDOK,0);
		Edit_SetSel(GetDlgItem(hDlg,IDC_SWE_OUT),0,-1);
		SetFocus(GetDlgItem(hDlg,IDC_SWE_OUT));
		return FALSE;}
	case WM_DESTROY:{
		break;}
	case WM_COMMAND: {
			switch(LOWORD(wParam)) {
			case IDC_SWE_EXPORT:{
				char filename[MAX_PATH];
				unsigned buflen=(unsigned)SendDlgItemMessage(hDlg,IDC_SWE_OUT,WM_GETTEXTLENGTH,0,0);
				char* buf=malloc(buflen+1);
				if(buf && buflen){
					GetDlgItemText(hDlg,IDC_SWE_OUT,buf,buflen+1);
					*filename='\0';
					if(SaveFileDialog(hDlg,filename,sizeof(filename))){
						FILE* fp=fopen(filename,"wb");
						if(fp){
							fwrite(buf,1,buflen,fp);
							fclose(fp);
						}
					}
				}
				free(buf);
				break;}
			case IDOK:{
				char buf[128];
				GetDlgItemText(hDlg,IDC_SWE_TOTAL,buf,sizeof(buf));
				if(!*buf){
					api.DelValue("Timers","SwExT");
					SetDlgItemText(hDlg,IDC_SWE_TOTAL,"\\n--------------------\\n\\t");
				}else
					api.SetStr("Timers","SwExT",buf);
				GetDlgItemText(hDlg,IDC_SWE_LAP,buf,sizeof(buf));
				if(!*buf){
					api.DelValue("Timers","SwExL");
					SetDlgItemText(hDlg,IDC_SWE_LAP,"Lap \\#\\f: \\l (\\t)\\n");
				}else
					api.SetStr("Timers","SwExL",buf);
				export_text(hDlg);
				break;}
			case IDCANCEL:
				EndDialog(hDlg, TRUE);
			}
			return TRUE;
		}
	}
	return FALSE;
}
