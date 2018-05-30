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

typedef struct _readerParameter {
	BOOL done;						// Termination
	LPHANDLE readerSemaphore;		// Other readers
	LPHANDLE comparatorSemaphore;	// Comparator thread
	LPTSTR startingDirectory;		// Directory to scan
	TCHAR currentEntryName[L];		// Current entry name
} READERPARAMETER;

typedef READERPARAMETER* LPREADERPARAMETER;

typedef struct _comparatorParameter {
	BOOL done;								// Termination
	INT nThreads;							// Number of threads
	LPREADERPARAMETER readersParameters;	// Array of reader parameters
	LPHANDLE readerSemaphores;				// Other readers
	LPHANDLE comparatorSemaphore;			// Comparator thread
} COMPARATORPARAMETER;

typedef COMPARATORPARAMETER* LPCOMPARATORPARAMETER;

DWORD WINAPI readerThread(LPVOID parameter);
DWORD WINAPI comparatorThread(LPVOID parameter);
VOID Traverse(LPREADERPARAMETER p, LPTSTR SourcePathName, int level);
LPTSTR CleanPath(LPREADERPARAMETER p);
DWORD FileType(LPWIN32_FIND_DATA pFileData);

INT _tmain(int argc, LPTSTR argv[]) {
	if (argc < 3) {
		_ftprintf(stderr, _T("Parameter errors %s directories ...\n"), argv[0]);
		Sleep(5000);
		return 1;
	}

	INT nThreads = argc - 1;
	LPHANDLE hThreads = (LPHANDLE)malloc(nThreads * sizeof(HANDLE)); // Threads
	LPHANDLE hReadersSemaphores = (LPHANDLE)malloc(nThreads * sizeof(HANDLE)); // Reader semaphores
	LPREADERPARAMETER readersParameters = (LPREADERPARAMETER)malloc(nThreads * sizeof(READERPARAMETER)); // Reader threads parameters
	HANDLE comparatorSemaphore = CreateSemaphore(NULL, 0, nThreads, NULL); // Comparator semaphore

	setbuf(stdout, 0); // No buffering on stdout

	// Thread creation
	for (INT i = 0; i < nThreads; i++) {
		// Semaphore - Initially 1 because thread needs to enter the first time
		hReadersSemaphores[i] = CreateSemaphore(NULL, 1, 1, NULL);
		// Preparing thread parameters
		readersParameters[i].startingDirectory = argv[i + 1]; // Initial path
		readersParameters[i].done = FALSE;
		readersParameters[i].readerSemaphore = &hReadersSemaphores[i]; // Reference to the created semaphore
		readersParameters[i].comparatorSemaphore = &comparatorSemaphore;
		hThreads[i] = CreateThread(NULL, 0, readerThread, (PVOID)&readersParameters[i], 0, NULL);
	}

	COMPARATORPARAMETER comparatorParameters = {FALSE, nThreads, readersParameters, hReadersSemaphores, &comparatorSemaphore};
	// hReaderSemaphores is the pointer to the array of reader semaphores. Comparator needs to know which thread signaled and unlocks all

	HANDLE comparator = CreateThread(NULL, 0, comparatorThread, (PVOID)&comparatorParameters, 0, NULL);

	// Wait for readers termination
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

	// Wait for comparator termination
	WaitForSingleObject(comparator, INFINITE);
	_tprintf(_T("[%u] collected\n"), GetThreadId(comparator));
	CloseHandle(comparator);

	free(hThreads);
	free(hReadersSemaphores);
	free(readersParameters);
	Sleep(5000);
	return 0;
}

VOID Traverse(LPREADERPARAMETER p, LPTSTR SourcePathName, int level) {
	HANDLE SearchHandle;
	WIN32_FIND_DATA FindData;
	DWORD FType, l;
	TCHAR NewPath[L], SearchPath[L];

	_tprintf(_T("[%u - %d] Visiting directory: %s\n"), GetCurrentThreadId(), level, SourcePathName);
	_stprintf(SearchPath, _T("%s\\*"), SourcePathName);
	SearchHandle = FindFirstFile(SearchPath, &FindData);

	do {
		if (p->done) // Early termination
			return;
		WaitForSingleObject(*p->readerSemaphore, INFINITE);
		FType = FileType(&FindData);
		l = _tcslen(SourcePathName);
		if (SearchPath[l - 1] == '\\') {
			_stprintf(NewPath, _T("%s%s"), SourcePathName, FindData.cFileName);
		}
		else {
			_stprintf(NewPath, _T("%s\\%s"), SourcePathName, FindData.cFileName);
		}

		_tprintf(_T("[%u - %d] New filename found: %s\n"), GetCurrentThreadId(), level, NewPath);
		
		// NewPath contains the filename/pathname just found
		// Remove the starting path. Comparator must compare only "relative" paths
		INT l = _tcslen(p->startingDirectory);
		_tcscpy(p->currentEntryName, NewPath + l);
		ReleaseSemaphore(*p->comparatorSemaphore, 1, NULL); // Signal comparator

		if (FType == TYPE_FILE) { // File
//			_ftprintf(fpOutput, _T("[%u - %d] File found: %s\n"), GetCurrentThreadId(), level, NewPath);
		}
		if (FType == TYPE_DIR) { // Directory
			Traverse(p, NewPath, level + 1);
		}
	} while (FindNextFile(SearchHandle, &FindData));

	_tprintf(_T("[%u - %d] Exiting directory: %s\n"), GetCurrentThreadId(), level, SourcePathName);

	FindClose(SearchHandle);
	return;
}

DWORD WINAPI readerThread(LPVOID parameter) {
	LPREADERPARAMETER p = (LPREADERPARAMETER)parameter;
	INT l = _tcslen(p->startingDirectory);
	for (INT i = 0; i < l; i++) {
		if (p->startingDirectory[i] == '/') {
			p->startingDirectory[i] = '\\';
		}
	}

	_tprintf(_T("[%u] started on %s\n"), GetCurrentThreadId(), p->startingDirectory);
	Traverse(p, p->startingDirectory, 0);
	_tprintf(_T("[%u] terminated\n"), GetCurrentThreadId());

	p->done = TRUE;
	ReleaseSemaphore(*p->comparatorSemaphore, 1, NULL); // Signal comparator

	return 0;
}

DWORD WINAPI comparatorThread(LPVOID parameter) {
	LPCOMPARATORPARAMETER p = (LPCOMPARATORPARAMETER) parameter;
	BOOL different = FALSE, someoneActive = TRUE;

	_tprintf(_T("[%u] Comparator started\n"), GetCurrentThreadId());
	while (!different && someoneActive) {
		for(INT i = 0; i < p->nThreads; i++) // All threads must have signaled
			WaitForSingleObject(*p->comparatorSemaphore, INFINITE);
 		_tprintf(_T("[%u] Comparator ready!\n"), GetCurrentThreadId());
		// All threads have a ready entry
		BOOL someoneFinished = FALSE;
		someoneActive = FALSE;
		for (INT i = 0; i < p->nThreads; i++) {
			if (p->readersParameters[i].done) {
				someoneFinished = TRUE;
				break;
			}
		}
		for (INT i = 0; i < p->nThreads; i++) {
			if (! p->readersParameters[i].done) {
				someoneActive = TRUE;
				break;
			}
		}
		if (someoneFinished && someoneActive) {
			different = TRUE;
		}
		else {
			_tprintf(_T("[%u] %s\n"), GetCurrentThreadId(), p->readersParameters[0].currentEntryName);
			for (INT i = 1; i < p->nThreads; i++) {
				_tprintf(_T("[%u] %s\n"), GetCurrentThreadId(), p->readersParameters[i].currentEntryName);
				if (_tcscmp(p->readersParameters[i].currentEntryName, p->readersParameters[0].currentEntryName) != 0) { // Different strings
					different = TRUE;
					break;
				}
			}
		}

		if (different) {
			for (INT i = 0; i < p->nThreads; i++)
				p->readersParameters[i].done = TRUE;
		}

		for(INT i = 0; i < p->nThreads; i++) // Wake up all threads
			ReleaseSemaphore(p->readerSemaphores[i], 1, NULL);
	}
	_tprintf(_T("[%u] Comparator terminated, %s\n"), GetCurrentThreadId(), different ? _T("different") : _T("equal"));

	return 0;
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