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
#include <time.h>

typedef struct _threadParameter {
	LPHANDLE MyDirection;		// My direction semaphore
	LPHANDLE Busy;				// Busy semaphore
	LPINT Running;				// Number of active cars
	INT TimeArrival;			// Cars arrive randomly every 0 ... TimeArrival seconds
	INT Time;					// Time needed to run
	TCHAR Class[4];				// L2R or R2L
} THREADPARAMETER;

typedef THREADPARAMETER* LPTHREADPARAMETER;

DWORD WINAPI ThreadFunction(LPVOID parameter);
VOID WaitThreadPool(LPHANDLE threads, INT nThreads);
VOID CloseHandles(LPHANDLE handles, INT nHandles);

INT _tmain(int argc, LPTSTR argv[]) {
	if (argc < 6) {
		_ftprintf(stderr, _T("Parameter errors %s timeL2R, timeR2L, timeARRIVAL, nL2R, nR2L\n"), argv[0]);
		Sleep(5000);
		return 1;
	}
	INT TimeL2R = _ttoi(argv[1]),
		TimeR2L = _ttoi(argv[2]),
		TimeArrival = _ttoi(argv[3]),
		nL2R = _ttoi(argv[4]),
		nR2L = _ttoi(argv[5]);

	LPHANDLE L2RThreads = (LPHANDLE)malloc(nL2R * sizeof(HANDLE));
	if (L2RThreads == NULL) {
		_ftprintf(stderr, _T("Error creating first handles array\n"));
		Sleep(5000);
		return 2;
	}

	LPHANDLE R2LThreads = (LPHANDLE)malloc(nR2L * sizeof(HANDLE));
	if (R2LThreads == NULL) {
		_ftprintf(stderr, _T("Error creating second handles array\n"));
		free(L2RThreads);
		Sleep(5000);
		return 3;
	}

	HANDLE L2RSemaphore = CreateSemaphore(NULL, 1, 1, _T("L2R")),
		R2LSemaphore = CreateSemaphore(NULL, 1, 1, _T("R2L")),
		BusySemaphore = CreateSemaphore(NULL, 1, 1, _T("Busy"));
	if (L2RSemaphore == INVALID_HANDLE_VALUE || R2LSemaphore == INVALID_HANDLE_VALUE ||
		BusySemaphore == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error creating semaphores\n"));
		free(L2RThreads);
		free(R2LThreads);
		Sleep(5000);
		return 5;
	}

	INT L2RRunning = 0, R2LRunning = 0;
	setbuf(stdout, 0);
	srand(time(NULL));

	THREADPARAMETER L2RParameters = {&L2RSemaphore, &BusySemaphore, &L2RRunning, TimeArrival, TimeL2R, _T("L2R")},
		R2LParameters = {&R2LSemaphore, &BusySemaphore, &R2LRunning, TimeArrival, TimeR2L, _T("R2L") };

	INT i;
	// Release thread "simultaneously"
	for (i = 0; i < min(nL2R, nR2L); i++) {
		Sleep(TimeArrival * 1000);
		L2RThreads[i] = CreateThread(NULL, 0, ThreadFunction, (LPVOID)&L2RParameters, 0, NULL);
		R2LThreads[i] = CreateThread(NULL, 0, ThreadFunction, (LPVOID)&R2LParameters, 0, NULL);
	}

	// Release remaining threads, if present
	for (INT j = i; j < nL2R; j++) {
		Sleep(TimeArrival * 1000);
		L2RThreads[i] = CreateThread(NULL, 0, ThreadFunction, (LPVOID)&L2RParameters, 0, NULL);
	}

	// Release remaining threads, if present
	for (INT j = i; j < nR2L; j++) {
		Sleep(TimeArrival * 1000);
		R2LThreads[i] = CreateThread(NULL, 0, ThreadFunction, (LPVOID)&R2LParameters, 0, NULL);
	}
	
	WaitThreadPool(L2RThreads, nL2R); // Wait Left 2 Right threads
	WaitThreadPool(R2LThreads, nR2L); // Wait Right 2 Left threads
	CloseHandles(L2RThreads, nL2R); // Close Left 2 Right handles
	CloseHandles(R2LThreads, nR2L); // Close Right 2 Left handles

	free(L2RThreads);
	free(R2LThreads);

	Sleep(5000);
	return 0;
}

DWORD WINAPI ThreadFunction(LPVOID parameter) {
	LPTHREADPARAMETER p = (LPTHREADPARAMETER)parameter;

	_tprintf(_T("[%s - %4u] Car ready\n"), p->Class, GetCurrentThreadId());
	WaitForSingleObject(*p->MyDirection, INFINITE);
	(*p->Running)++;
	if (*p->Running == 1)
		WaitForSingleObject(*p->Busy, INFINITE);
	ReleaseSemaphore(*p->MyDirection, 1, NULL);

	_tprintf(_T("[%s - %4u] Car started ...\n"), p->Class, GetCurrentThreadId());
	Sleep(rand() % p->Time * 1000); // Run
	_tprintf(_T("[%s - %4u] ... Car arrived\n"), p->Class, GetCurrentThreadId());

	WaitForSingleObject(*p->MyDirection, INFINITE);
	(*p->Running)--;
	if (*p->Running == 0)
		ReleaseSemaphore(*p->Busy, 1, NULL);
	ReleaseSemaphore(*p->MyDirection, 1, NULL);
	_tprintf(_T("[%s - %4u] Car terminated\n"), p->Class, GetCurrentThreadId());

	ExitThread(0);
}

VOID WaitThreadPool(LPHANDLE threads, INT nThreads) {
	for (INT i = 0; i < nThreads; i += MAXIMUM_WAIT_OBJECTS)
		WaitForMultipleObjects(min(nThreads - i, MAXIMUM_WAIT_OBJECTS), &threads[i], TRUE, INFINITE);
}

VOID CloseHandles(LPHANDLE handles, INT nHandles) {
	for (INT i = 0; i < nHandles; i += MAXIMUM_WAIT_OBJECTS)
		CloseHandle(handles[i]);
}