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

#define L MAX_PATH

#define VERSION 'B'

DWORD WINAPI threadFunction(LPVOID parameter);
VOID Traverse(LPTSTR SourcePathName, FILE*fpOutput);
DWORD FileType(LPWIN32_FIND_DATA pFileData);

typedef struct parameter_t {
	TCHAR startingDirectory[L];
};

INT _tmain(int argc, LPTSTR argv[]) {
	if (argc < 2) {
		_ftprintf(stderr, _T("Parameter errors %s directory ...\n"), argv[0]);
		return 1;
	}

	INT nThreads = argc - 1;
	LPHANDLE hThreads = (LPHANDLE) malloc(nThreads * sizeof(HANDLE));
	parameter_t* p = (parameter_t*)malloc(nThreads * sizeof(parameter_t));

	/* Launch threads */
	for (INT i = 0; i < nThreads; i++) {
		/* Preparing thread parameters */
		_tcscpy_s(p[i].startingDirectory, argv[i + 1]);
		hThreads[i] = CreateThread(NULL, 0, threadFunction, (PVOID)&p[i], 0, NULL);
	}

	/* Wait threads */
	while (nThreads > 0) {
		DWORD status = WaitForMultipleObjects(nThreads, hThreads, FALSE, INFINITE);
		if (status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + nThreads) {
			int i = (int)status - (int)WAIT_OBJECT_0;
			_tprintf(_T("[%u] collected\n"), GetThreadId(hThreads[i]));
			CloseHandle(hThreads[i]);
			hThreads[i] = hThreads[nThreads - 1];
			nThreads--;
		}
	}

	Sleep(5000);
	return 0;
}

DWORD WINAPI threadFunction(LPVOID parameter) {
	parameter_t* p = (parameter_t*)parameter;
	INT l =  _tcslen(p->startingDirectory);
	for (INT i = 0; i < l; i++) {
		if (p->startingDirectory[i] == '/') {
			p->startingDirectory[i] = '\\';
		}
	}

	_tprintf(_T("[%u] started\n"), GetCurrentThreadId());

#if VERSION == 'A' /* Interleaved print */
	Traverse(p->startingDirectory, stdout);
#elif VERSION == 'B' /* Print on file */
	/* Generate filename */
	TCHAR FileName[L];
	_stprintf(FileName, _T("%u.txt"), GetCurrentThreadId());
	/* Create file */
	FILE* fpOut = _tfopen(FileName, _T("w+"));
	if (fpOut == NULL) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"),
			FileName, GetLastError());
		return 1;
	}
	Traverse(p->startingDirectory, fpOut);
	fclose(fpOut);
#endif
	
	_tprintf(_T("[%u] terminated\n"), GetCurrentThreadId());
	return 0;
}

VOID Traverse(LPTSTR SourcePathName, FILE*fpOutput) {
	HANDLE SearchHandle;
	WIN32_FIND_DATA FindData;
	DWORD FType, l;
	TCHAR NewPath[L], PathName[L];

	_ftprintf(fpOutput, _T("[%u] Visiting directory: %s\n"), GetCurrentThreadId(), SourcePathName);
	_stprintf(PathName, _T("%s\\*"), SourcePathName);
	SearchHandle = FindFirstFile(PathName, &FindData);

	do {
		FType = FileType(&FindData);
		l = _tcslen(SourcePathName);
		if (PathName[l - 1] == '\\') {
			_stprintf(NewPath, _T("%s%s"), SourcePathName, FindData.cFileName);
		}
		else {
			_stprintf(NewPath, _T("%s\\%s"), SourcePathName, FindData.cFileName);
		}

		/* Directory */
		if (FType == TYPE_DIR) {
			Traverse(NewPath, fpOutput);
		}
	} while (FindNextFile(SearchHandle, &FindData));

	FindClose(SearchHandle);
	return;
}

DWORD FileType(LPWIN32_FIND_DATA pFileData) {
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