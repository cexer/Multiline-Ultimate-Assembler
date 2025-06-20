#include "stdafx.h"
#include "main_common.h"
#include "plugin.h"
#include "assembler_dlg.h"
#include "options_dlg.h"

extern HINSTANCE hDllInst;

static int g_pluginHandle;
static int g_hMenu;
static int g_hMenuDisasm;;
static int g_dwProcessId;

#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif // DLL_EXPORT

#define MENU_MAIN             0
#define MENU_DISASM           1
#define MENU_OPTIONS          2
#define MENU_HELP             3
#define MENU_ABOUT            4

#define MENU_CPU_DISASM       5

static int GetPluginVersion();
static void DisassembleSelection();
static bool CmdShow(int argc, char** argv);
static bool CmdDisasmSelection(int argc, char** argv);
static bool CmdClose(int argc, char** argv);

DLL_EXPORT void plugsetup(PLUG_SETUPSTRUCT *setupStruct)
{
	hwollymain = setupStruct->hwndDlg;
	g_hMenu = setupStruct->hMenu;
	g_hMenuDisasm = setupStruct->hMenuDisasm;

	HRSRC hResource = FindResource(hDllInst, MAKEINTRESOURCE(IDB_X64DBG_ICON), "PNG");
	if(hResource)
	{
		HGLOBAL hMemory = LoadResource(hDllInst, hResource);
		if(hMemory)
		{
			DWORD dwSize = SizeofResource(hDllInst, hResource);
			LPVOID lpAddress = LockResource(hMemory);
			if(lpAddress)
			{
				ICONDATA IconData;
				IconData.data = lpAddress;
				IconData.size = dwSize;

				_plugin_menuseticon(g_hMenu, &IconData);
				_plugin_menuseticon(g_hMenuDisasm, &IconData);
			}
		}
	}

	_plugin_menuaddentry(g_hMenu, MENU_MAIN, "&Multiline Ultimate Assembler\tCtrl+M");
	_plugin_menuaddseparator(g_hMenu);
	_plugin_menuaddentry(g_hMenu, MENU_OPTIONS, "&Options");
	_plugin_menuaddseparator(g_hMenu);
	_plugin_menuaddentry(g_hMenu, MENU_HELP, "&Help");
	_plugin_menuaddentry(g_hMenu, MENU_ABOUT, "&About");

	_plugin_menuaddentry(g_hMenuDisasm, MENU_CPU_DISASM, "&Disassemble selection\tCtrl+Shift+M");
}

void _CBPLUGIN(CBTYPE cbType, void* callbackInfo)
{
    if (cbType == CB_ATTACH)
    {
        PLUG_CB_ATTACH* cb = (PLUG_CB_ATTACH*)callbackInfo;
        g_dwProcessId = cb->dwProcessId;
    }
    if (cbType == CB_DETACH)
    {
        g_dwProcessId = 0;
    }
    if (cbType == CB_CREATEPROCESS)
    {
        PLUG_CB_CREATEPROCESS* cb = (PLUG_CB_CREATEPROCESS*)callbackInfo;
        g_dwProcessId = cb->fdProcessInfo->dwProcessId;
    }
    if (cbType == CB_EXITPROCESS)
    {
        PLUG_CB_EXITPROCESS* cb = (PLUG_CB_EXITPROCESS*)callbackInfo;
        g_dwProcessId = 0;
    }
}

int GetDebugeeProcessId()
{
    return g_dwProcessId;
}

DLL_EXPORT bool pluginit(PLUG_INITSTRUCT* initStruct)
{
	initStruct->pluginVersion = GetPluginVersion();
	initStruct->sdkVersion = PLUG_SDKVERSION;
	lstrcpy(initStruct->pluginName, DEF_PLUGINNAME);
	g_pluginHandle = initStruct->pluginHandle;

	char *pError = PluginInit(hDllInst);
	if(pError)
	{
		MessageBox(hwollymain, pError, "Multiline Ultimate Assembler error", MB_ICONHAND);
		return false;
	}

	_plugin_logputs("Multiline Ultimate Assembler v" DEF_VERSION);
	_plugin_logputs("  " DEF_COPYRIGHT);

	_plugin_registercommand(g_pluginHandle, "multiasm_show", CmdShow, false);
	_plugin_registercommand(g_pluginHandle, "multiasm_disasm_selection", CmdDisasmSelection, true);
	_plugin_registercommand(g_pluginHandle, "multiasm_close", CmdClose, false);
    _plugin_registercallback(g_pluginHandle, CB_ATTACH, _CBPLUGIN);
    _plugin_registercallback(g_pluginHandle, CB_DETACH, _CBPLUGIN);
    _plugin_registercallback(g_pluginHandle, CB_CREATEPROCESS, _CBPLUGIN);
    _plugin_registercallback(g_pluginHandle, CB_EXITPROCESS, _CBPLUGIN);

	return true;
}

static int GetPluginVersion()
{
	char *p = DEF_VERSION;
	int nVersion = 0;

	while(*p != '\0')
	{
		char c = *p;
		if(c >= '0' && c <= '9')
		{
			nVersion *= 10;
			nVersion += c - '0';
		}

		p++;
	}

	return nVersion;
}

DLL_EXPORT bool plugstop()
{
	_plugin_menuclear(g_hMenu);
	_plugin_menuclear(g_hMenuDisasm);

	_plugin_unregistercommand(g_pluginHandle, "multiasm_show");
	_plugin_unregistercommand(g_pluginHandle, "multiasm_disasm_selection");
	_plugin_unregistercommand(g_pluginHandle, "multiasm_close");

	AssemblerCloseDlg();
	PluginExit();
	return true;
}

DLL_EXPORT CDECL void CBWINEVENT(CBTYPE cbType, PLUG_CB_WINEVENT *info)
{
	MSG *pMsg = info->message;

	if(!info->result && AssemblerPreTranslateMessage(pMsg))
	{
		info->retval = true;
		return;
	}

	if(info->result &&
		pMsg->message == WM_KEYUP &&
		pMsg->wParam == 'M')
	{
		bool ctrlDown = GetKeyState(VK_CONTROL) < 0;
		bool altDown = GetKeyState(VK_MENU) < 0;
		bool shiftDown = GetKeyState(VK_SHIFT) < 0;

		if(!altDown && ctrlDown)
		{
			if(shiftDown)
			{
				if(DbgIsDebugging())
					DisassembleSelection();
			}
			else
			{
				AssemblerShowDlg();
			}

			*info->result = 0;
			info->retval = true;
			return;
		}
	}
}

DLL_EXPORT CDECL void CBMENUENTRY(CBTYPE cbType, void *callbackInfo)
{
	PLUG_CB_MENUENTRY *info = (PLUG_CB_MENUENTRY *)callbackInfo;

	switch(info->hEntry)
	{
	case MENU_MAIN:
		// Menu item, main plugin functionality
		AssemblerShowDlg();
		break;

	case MENU_DISASM:
	case MENU_CPU_DISASM:
		if(DbgIsDebugging())
			DisassembleSelection();
		else
			MessageBox(hwollymain, "No process is loaded", NULL, MB_ICONASTERISK);
		break;

	case MENU_OPTIONS:
		// Menu item "Options"
		if(ShowOptionsDlg())
			AssemblerOptionsChanged();
		break;

	case MENU_HELP:
		// Menu item "Help"
		if(!OpenHelp(hwollymain, hDllInst))
			MessageBox(hwollymain, "Failed to open the \"multiasm.chm\" help file", NULL, MB_ICONHAND);
		break;

	case MENU_ABOUT:
		// Menu item "About", displays plugin info.
		AboutMessageBox(hwollymain, hDllInst);
		break;
	}
}

static void DisassembleSelection()
{
	SELECTIONDATA selection;

	if(GuiSelectionGet(GUI_DISASSEMBLY, &selection))
		AssemblerLoadCode(selection.start, selection.end - selection.start + 1);
}

static bool CmdShow(int argc, char** argv)
{
	if(argc > 1)
	{
		_plugin_logputs("Command does not accept arguments");
		return false;
	}

	GuiExecuteOnGuiThread(AssemblerShowDlg);
	return true;
}

static bool CmdDisasmSelection(int argc, char** argv)
{
	if(argc > 1)
	{
		_plugin_logputs("Command does not accept arguments");
		return false;
	}

	GuiExecuteOnGuiThread(DisassembleSelection);
	return true;
}

static bool CmdClose(int argc, char** argv)
{
	if(argc > 1)
	{
		_plugin_logputs("Command does not accept arguments");
		return false;
	}

	GuiExecuteOnGuiThread(AssemblerCloseDlg);
	return true;
}
