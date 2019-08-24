/*
HISTORY:
========
20091109 (AndersK)
	*Initial version


Docs say we must wait for RegisterWindowMessage(L"TaskbarButtonCreated") before we can call ITaskbarList3 methods, but there is no way for us to catch this message in a plugin that is only used on one page, so we just assume everything is ready...

*/

#if (defined(_MSC_VER) && !defined(_DEBUG))
	#pragma comment(linker,"/opt:nowin98")
	#pragma comment(linker,"/ignore:4078")
	#pragma comment(linker,"/merge:.rdata=.text")
#endif

#define _WIN32_WINNT 0x0500

#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <Objbase.h>
#include <ShObjIdl.h>


const GUID CLSID_ITaskbarList={0x56FDF344,0xFD6D,0x11d0,{0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90}};
const GUID IID_ITaskbarList1 ={0x56FDF342,0xFD6D,0x11d0,{0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90}};
const GUID IID_ITaskbarList3 ={0xea1afb91,0x9e28,0x4b86,{0x90,0xe9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf}};

#ifndef __ITaskbarList3_INTERFACE_DEFINED__

enum	TBPFLAG {
	TBPF_NOPROGRESS = 0,
};

typedef void* LPTHUMBBUTTON;//dummy typedef!
enum TBATFLAG {TBATF_DUMMY};

class ITaskbarList3 : public IUnknown 
{
public:
	virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();
	//ITaskbarList(1)
	virtual STDMETHODIMP HrInit();
	virtual STDMETHODIMP AddTab(HWND hwnd);
	virtual STDMETHODIMP DeleteTab(HWND hwnd);
	virtual STDMETHODIMP ActivateTab(HWND hwnd);
	virtual STDMETHODIMP SetActiveAlt(HWND hwnd);
	//ITaskbarList2
	virtual STDMETHODIMP MarkFullscreenWindow(HWND hwnd,BOOL fFullscreen);
	//ITaskbarList3
	virtual STDMETHODIMP SetProgressValue(HWND hwnd,ULONGLONG ullCompleted, ULONGLONG ullTotal);
	virtual STDMETHODIMP SetProgressState(HWND hwnd,TBPFLAG tbpFlags);
	virtual STDMETHODIMP RegisterTab(HWND hwndTab, HWND hwndMDI);
	virtual STDMETHODIMP UnregisterTab(HWND hwndTab);
	virtual STDMETHODIMP SetTabOrder(HWND hwndTab,HWND hwndInsertBefore);
	virtual STDMETHODIMP SetTabActive(HWND hwndTab,HWND hwndMDI, TBATFLAG tbatFlags);
	virtual STDMETHODIMP ThumbBarAddButtons(HWND hwnd,UINT cButtons, LPTHUMBBUTTON pButton);
	virtual STDMETHODIMP ThumbBarUpdateButtons(HWND hwnd,UINT cButtons, LPTHUMBBUTTON pButton);
	virtual STDMETHODIMP ThumbBarSetImageList(HWND hwnd,HIMAGELIST himl);
	virtual STDMETHODIMP SetOverlayIcon(HWND hwnd,HICON hIcon, LPCWSTR pszDescription);
	virtual STDMETHODIMP SetThumbnailTooltip(HWND hwnd,LPCWSTR pszTip);
	virtual STDMETHODIMP SetThumbnailClip(HWND hwnd,RECT *prcClip);
	//ITaskbarList4
	//virtual STDMETHODIMP SetTabProperties(HWND hwndTab,STPFLAG stpFlags);
};

#endif	///__ITaskbarList3_INTERFACE_DEFINED__

#define NSISCALL __stdcall
typedef struct {
  LPVOID xxx1;//exec_flags_type *exec_flags;
  int (NSISCALL *ExecuteCodeSegment)(int, HWND);
  void (NSISCALL *validate_filename)(LPTSTR);
  int (NSISCALL *RegisterPluginCallback)(HMODULE,LPVOID);
} extra_parameters;


ITaskbarList3*g_pTL=NULL;
WNDPROC g_PBOrgProc;
HWND g_hwndNsis;
HMODULE g_ThisDll;
UINT g_rangeTot;


LRESULT CALLBACK PBSubProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) 
{
	switch(msg) 
	{
	case WM_DESTROY:
		SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)g_PBOrgProc);
		if (g_pTL) 
		{
			g_pTL->SetProgressState(g_hwndNsis,TBPF_NOPROGRESS);
			g_pTL->Release();
			g_pTL=NULL;
		}
		break;
	
	case PBM_SETRANGE:
		wp=LOWORD(lp);
		lp=HIWORD(lp);
	case PBM_SETRANGE32:
		g_rangeTot=wp+lp;
		break;
	
	case PBM_SETPOS:
		if (g_pTL) 
		{
			g_pTL->SetProgressValue(g_hwndNsis,wp,g_rangeTot);
		}
		break;
	}
	return CallWindowProc(g_PBOrgProc,hwnd,msg,wp,lp);
}

UINT_PTR NSISCALL NSISPluginCallback(UINT Event) 
{
/*	switch(Event) 
	{
	case NSPIM_UNLOAD:
		break;
	}
*/	return 0;
}

extern "C" void __declspec(dllexport) __cdecl Start(HWND hwndNSIS,int N_CCH,TCHAR*N_Vars,LPVOID ppST,extra_parameters*pXP) 
{
	HRESULT hr;
	g_hwndNsis=hwndNSIS;
	
	pXP->RegisterPluginCallback(g_ThisDll,(LPVOID)NSISPluginCallback);
	
	CoCreateInstance(CLSID_ITaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_ITaskbarList3,(void**)&g_pTL);
	
	
	if (g_pTL) 
	{
		bool ok=false;
		hr=g_pTL->HrInit();
		
		if (SUCCEEDED(hr)) 
		{
			HWND hProgress=FindWindowEx(FindWindowEx(hwndNSIS,NULL,_T("#32770"),NULL),NULL,_T("msctls_progress32"),NULL);
			
			
			
			if (hProgress)
			{
				PBRANGE pbr;
				SendMessage(hProgress,PBM_GETRANGE,0,(LPARAM)&pbr);
				g_rangeTot=pbr.iLow+pbr.iHigh;
				
				
				g_PBOrgProc=(WNDPROC)SetWindowLongPtr(hProgress,GWLP_WNDPROC,(LONG_PTR)PBSubProc);
				ok=g_PBOrgProc != NULL;
				
				
			}

		}
		
		if (!ok)
		{
			g_pTL->Release();
			g_pTL=NULL;
		}
	}
}

extern "C" BOOL WINAPI _DllMainCRTStartup(HMODULE hInst,UINT Reason,LPVOID lpReserved)
{
	g_ThisDll=hInst;
	return TRUE;
}
