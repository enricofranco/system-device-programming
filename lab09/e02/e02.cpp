#define UNICODE
#define _UNICODE

#ifdef UNICODE
#define TCHAR WCHAR
#else
#define TCHAR CHAR
#endif

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#define TYPE_FILE	1
#define TYPE_DIR	2
#define TYPE_DOT	3

#define L 500

VOID TraverseAndCreate(LPTSTR SourcePathName, LPTSTR DestPathName);
DWORD FileType(LPWIN32_FIND_DATA pFileData);

INT _tmain (int argc, LPTSTR argv[]) {
	if (argc < 3) {
		_ftprintf(stderr, _T("Parameter errors %s source_directory destination_directory\n"), argv[0]);
		return 1;
	}

	TCHAR tmpPath[L], DestPathName[L];

	GetCurrentDirectory(L, tmpPath);
	_stprintf(DestPathName, _T("%s\\%s"), tmpPath, argv[2]);
	TraverseAndCreate(argv[1], DestPathName);
	
	Sleep(5000);
	return 0;
}

VOID TraverseAndCreate(LPTSTR SourcePathName, LPTSTR DestPathName) {
	HANDLE SearchHandle;
	WIN32_FIND_DATA FindData;
	DWORD FType, l;
	TCHAR NewPath[L];

	_tprintf(_T("--> Create Dir : %s\n"), DestPathName);
	CreateDirectory(DestPathName, NULL);
	SetCurrentDirectory(SourcePathName);
	SearchHandle = FindFirstFile(_T("*"), &FindData);
	
	do {
		FType = FileType(&FindData);
		l = _tcslen(DestPathName);
		if (DestPathName[l - 1] == '\\') {
			_stprintf(NewPath, _T("%s%s"), DestPathName, FindData.cFileName);
		} else {
			_stprintf(NewPath, _T("%s\\%s"), DestPathName, FindData.cFileName);
		}
		if (FType == TYPE_FILE) {
			CopyFile(FindData.cFileName, NewPath, FALSE);
		}
		if (FType == TYPE_DIR) {
			TraverseAndCreate(FindData.cFileName, NewPath);
			SetCurrentDirectory(_T(".."));
		}
	} while (FindNextFile(SearchHandle, &FindData));

	FindClose(SearchHandle);
	return;
}

DWORD FileType (LPWIN32_FIND_DATA pFileData) {
	BOOL IsDir;
	DWORD FType;
	FType = TYPE_FILE;
	IsDir = (pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (IsDir)
		if (lstrcmp(pFileData->cFileName, _T(".")) == 0 || lstrcmp(pFileData->cFileName, _T("..")) == 0)
			FType = TYPE_DOT;
		else
			FType = TYPE_DIR;
	return FType;
}