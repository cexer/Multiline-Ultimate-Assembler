#include "stdafx.h"
#include "plugin.h"

HWND hwollymain;

// Config functions

BOOL MyGetintfromini(HINSTANCE dllinst, TCHAR *key, int *p_val, int min, int max, int def)
{
	int val;

	if(!BridgeSettingGetUint(DEF_PLUGINNAME, key, &val))
	{
		*p_val = def;

		return FALSE;
	}

	if(min && max && (val < min || val > max))
		*p_val = def;
	else
		*p_val = val;

	return TRUE;
}

BOOL MyWriteinttoini(HINSTANCE dllinst, TCHAR *key, int val)
{
	return BridgeSettingSetUint(DEF_PLUGINNAME, key, val) != false;
}

int MyGetstringfromini(HINSTANCE dllinst, TCHAR *key, TCHAR *s, int length)
{
	char *buf;
	int len;

	if(length >= MAX_SETTING_SIZE)
	{
		if(!BridgeSettingGet(DEF_PLUGINNAME, key, s))
		{
			*s = '\0';
			return 0;
		}

		return lstrlen(s);
	}

	buf = (char *)HeapAlloc(GetProcessHeap(), 0, MAX_SETTING_SIZE*sizeof(char));
	if(!buf)
		return 0;

	if(!BridgeSettingGet(DEF_PLUGINNAME, key, buf))
	{
		HeapFree(GetProcessHeap(), 0, buf);
		*s = '\0';
		return 0;
	}

	len = lstrlen(buf);
	if(len > length - 1)
		len = length - 1;

	lstrcpyn(s, buf, len + 1);

	HeapFree(GetProcessHeap(), 0, buf);

	return len;
}

BOOL MyWritestringtoini(HINSTANCE dllinst, TCHAR *key, TCHAR *s)
{
	return BridgeSettingSet(DEF_PLUGINNAME, key, s) != false;
}

// Assembler functions

DWORD SimpleDisasm(BYTE *cmd, DWORD cmdsize, DWORD ip, BYTE *dec, BOOL bSizeOnly,
	TCHAR *pszResult, DWORD *jmpconst, DWORD *adrconst, DWORD *immconst)
{
	BYTE cmd_safe[MAXCMDSIZE];
	if(cmdsize < MAXCMDSIZE)
	{
		CopyMemory(cmd_safe, cmd, cmdsize);
		ZeroMemory(cmd_safe + cmdsize, MAXCMDSIZE - cmdsize);
		cmd = cmd_safe;
	}

	BASIC_INSTRUCTION_INFO basicinfo;
	if(!DbgFunctions()->DisasmFast(cmd, ip, &basicinfo))
		return 0;

	if(!bSizeOnly)
	{
		lstrcpy(pszResult, basicinfo.instruction); // pszResult should have at least COMMAND_MAX_LEN chars

		*jmpconst = basicinfo.addr;
		*adrconst = basicinfo.memory.value;
		*immconst = basicinfo.value.value;
	}

	return basicinfo.size;
}

int AssembleShortest(TCHAR *lpCommand, DWORD dwAddress, BYTE *bBuffer, TCHAR *lpError)
{
	int size;
	if(!DbgFunctions()->Assemble(dwAddress, bBuffer, &size, lpCommand, lpError))
		return 0;

	return size;
}

int AssembleWithGivenSize(TCHAR *lpCommand, DWORD dwAddress, int req_size, BYTE *bBuffer, TCHAR *lpError)
{
	int size;
	if(!DbgFunctions()->Assemble(dwAddress, bBuffer, &size, lpCommand, lpError))
		return 0;

	// TODO: fix when implemented
	if(size > req_size)
	{
		lstrcpy(lpError, "AssembleWithGivenSize: internal assembler error");
		return 0;
	}

	while(size < req_size)
		bBuffer[size++] = 0x90; // Fill with NOPs

	return size;
}

// Memory functions

BOOL SimpleReadMemory(void *buf, DWORD addr, DWORD size)
{
	return DbgMemRead(addr, buf, size);
}

BOOL SimpleWriteMemory(void *buf, DWORD addr, DWORD size)
{
	return DbgFunctions()->MemPatch(addr, buf, size);
}

// Symbolic functions

int GetLabel(DWORD addr, TCHAR *name)
{
	if(!DbgGetLabelAt(addr, SEG_DEFAULT, name))
		return 0;

	return lstrlen(name);
}

int GetComment(DWORD addr, TCHAR *name)
{
	if(!DbgGetCommentAt(addr, name))
		return 0;

	return lstrlen(name);
}

BOOL QuickInsertLabel(DWORD addr, TCHAR *s)
{
	return DbgSetAutoLabelAt(addr, s);
}

BOOL QuickInsertComment(DWORD addr, TCHAR *s)
{
	return DbgSetAutoCommentAt(addr, s);
}

void MergeQuickData(void)
{
}

void DeleteRangeLabels(DWORD addr0, DWORD addr1)
{
	DbgClearAutoLabelRange(addr0, addr1);
}

void DeleteRangeComments(DWORD addr0, DWORD addr1)
{
	DbgClearAutoCommentRange(addr0, addr1);
}

// Module functions

PLUGIN_MODULE FindModuleByName(TCHAR *lpModule)
{
	return (PLUGIN_MODULE)DbgFunctions()->ModBaseFromName(lpModule);
}

PLUGIN_MODULE FindModuleByAddr(DWORD dwAddress)
{
	return (PLUGIN_MODULE)DbgFunctions()->ModBaseFromAddr(dwAddress);
}

DWORD GetModuleBase(PLUGIN_MODULE module)
{
	return (DWORD)module;
}

DWORD GetModuleSize(PLUGIN_MODULE module)
{
	return DbgFunctions()->ModSizeFromAddr((duint)module);
}

BOOL GetModuleName(PLUGIN_MODULE module, TCHAR *pszModuleName)
{
	return DbgFunctions()->ModNameFromAddr((duint)module, pszModuleName, FALSE);
}

BOOL IsModuleWithRelocations(PLUGIN_MODULE module)
{
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)module;

	LONG e_lfanew;
	if(!DbgMemRead(
		(ULONG_PTR)&pDosHeader->e_lfanew,
		(BYTE *)&e_lfanew,
		sizeof(LONG)))
	{
		return FALSE;
	}

	IMAGE_NT_HEADERS *pNtHeader = (IMAGE_NT_HEADERS *)((char *)pDosHeader + e_lfanew);

	DWORD dwRelocVirtualAddress;
	if(!DbgMemRead(
		(ULONG_PTR)&pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
		(BYTE *)&dwRelocVirtualAddress,
		sizeof(DWORD)))
	{
		return FALSE;
	}

	return dwRelocVirtualAddress != 0;
}

// Memory functions

PLUGIN_MEMORY FindMemory(DWORD dwAddress)
{
	return (PLUGIN_MEMORY)DbgMemFindBaseAddr(dwAddress, NULL);
}

DWORD GetMemoryBase(PLUGIN_MEMORY mem)
{
	return (DWORD)mem;
}

DWORD GetMemorySize(PLUGIN_MEMORY mem)
{
	ULONG_PTR size;
	if(!DbgMemFindBaseAddr((duint)mem, &size))
		return 0;

	return size;
}

void EnsureMemoryBackup(PLUGIN_MEMORY mem)
{
}

// Analysis functions

BYTE *FindDecode(DWORD addr, DWORD *psize)
{
	// TODO: not implemented
	return NULL;
}

int DecodeGetType(BYTE decode)
{
	// TODO: not implemented
	return DECODE_UNKNOWN;
}

// Misc.

BOOL IsProcessLoaded()
{
	return DbgIsDebugging();
}

void SuspendAllThreads()
{
	// Note: I'm not sure it's required to be implemented here
	// It's recommended to call for OllyDbg v2, though

	//ThreaderPauseAllThreads(false);
}

void ResumeAllThreads()
{
	// Note: I'm not sure it's required to be implemented here
	// It's recommended to call for OllyDbg v2, though

	//ThreaderResumeAllThreads(false);
}

DWORD GetCpuBaseAddr()
{
	SELECTIONDATA selection;

	if(!GuiSelectionGet(GUI_DISASSEMBLY, &selection))
		return 0;
		
	return DbgMemFindBaseAddr(selection.start, NULL);
}

void InvalidateGui()
{
	GuiUpdateAllViews();
}
