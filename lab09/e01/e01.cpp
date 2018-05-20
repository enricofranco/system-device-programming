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

#define FILE_LENGHT 20
#define MAX_NUMBER 100

#define SKIP_READING

typedef struct parameter_t {
	PTCHAR filename;
	PINT values;
	INT n;
} parameter_t;

VOID generateFile(TCHAR filename[]);
DWORD WINAPI threadFunction(LPVOID parameter);

INT _tmain(INT argc, LPTSTR argv[]) {
	if (argc < 3) {
		_ftprintf(stderr, _T("Parameter errors %s input_files ... output_file\n"), argv[0]);
		return 1;
	}

	PHANDLE h = (PHANDLE) malloc((argc - 2) * sizeof(HANDLE));
	parameter_t* p = (parameter_t*)malloc((argc - 2) * sizeof(parameter_t));
	srand((unsigned int) time(NULL));

	/* Launch threads */
	for (INT i = 0; i < argc - 2; i++) {
		generateFile(argv[i + 1]);
		/* Preparing thread parameter */
		p[i].filename = argv[i + 1];
		p[i].values = NULL;
		p[i].n = 0;

		h[i] = CreateThread(NULL, 0, threadFunction, (PVOID) &p[i], 0, NULL);
#ifndef SKIP_READING
		Sleep(500);
#endif
	}

	/* Wait threads */
	INT nThreads = argc - 2;
	while (nThreads > 0) {
		DWORD status = WaitForMultipleObjects(nThreads, h, FALSE, INFINITE);
		if (status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + nThreads) {
			int i = (int) status - (int) WAIT_OBJECT_0;
			CloseHandle(h[i]);
			h[i] = h[nThreads - 1];
			nThreads--;
		}
	}

	/* Merge */
	/* Final dimension of the array */
	INT n = 0;
	for (INT i = 0; i < argc - 2; i++)
		n += p[i].n;

	PINT values = (PINT) malloc(n * sizeof(INT));
	PINT index = (PINT) malloc((argc - 2) * sizeof(INT));
	
	/* Index contains the index of the first valid element in the array */
	for (INT i = 0; i < argc - 2; i++)
		index[i] = 0;

	for (INT i = 0; i < n; i++) {
		INT k = 0;
		/* Select the first array with valid elements */
		for (INT j = 0; j < argc -2; j++)
			if (index[j] < p[j].n) {
				k = j;
				break;
			}

		/* Starting from next array, search the minimum */
		for (INT j = k + 1; j < argc - 2; j++) {
			if (index[j] < p[j].n) { /* Valid size */
				if (p[j].values[index[j]] < p[k].values[index[k]]) /* Update current minimum */
					k = j;
			}
		}
		values[i] = p[k].values[index[k]++];
	}

	/* Write output file */
	PTCHAR filename = argv[argc - 1];
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);	
	DWORD bytesWritten, bytesRead;
	INT value;

	if (hFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open file %s in writing mode. Error: %x\n"),
			filename, GetLastError());
		return 1;
	}

	WriteFile(hFile, &n, sizeof(INT), &bytesWritten, NULL);
	if (bytesWritten != sizeof(INT)) {
		_ftprintf(stderr, _T("Error writing file: %s\n"), filename);
		CloseHandle(hFile);
		return 2;
	}

	for (INT i = 0; i < n; i++) {
		WriteFile(hFile, &values[i], sizeof(INT), &bytesWritten, NULL);
		if (bytesWritten != sizeof(INT)) {
			_ftprintf(stderr, _T("Error writing file: %s\n"), filename);
			CloseHandle(hFile);
			return 3;
		}
	}

	CloseHandle(hFile);

	/* Print file */
	hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open file %s in reading mode. Error: %x\n"),
			filename, GetLastError());
		return 1;
	}

	ReadFile(hFile, &value, sizeof(INT), &bytesRead, NULL);
	if (bytesRead <= 0) {
		_ftprintf(stderr, _T("Error reading file: %s\n"), filename);
		CloseHandle(hFile);
		return 2;
	}

	_tprintf(_T("[%s] %d: "), filename, value);
	for (INT i = 0; i < n; i++) {
		ReadFile(hFile, &value, sizeof(INT), &bytesRead, NULL);
		if (bytesRead <= 0) {
			_ftprintf(stderr, _T("Error reading file: %s\n"), p->filename);
			CloseHandle(hFile);
			return 3;
		}
		_tprintf(_T("%d "), value);
	}
	_tprintf(_T("\n"));

	CloseHandle(hFile);

	free(values);
	free(p);
	free(h);
	Sleep(5000);
	return 0;
}

VOID generateFile(TCHAR filename[]) {
	HANDLE hOut = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open file %s in writing mode. Error: %x\n"),
			filename, GetLastError());
		return;
	}
	
	INT n = rand() % FILE_LENGHT + 1; /* Number of values */
	DWORD bytesWritten;
	
	WriteFile(hOut, &n, sizeof(INT), &bytesWritten, NULL);
	if (bytesWritten != sizeof(INT)) {
		_ftprintf(stderr, _T("Error writing file: %s\n"), filename);
		CloseHandle(hOut);
		return;
	}

	for (INT i = 0; i < n; i++) {
		INT value = rand() % MAX_NUMBER;
		WriteFile(hOut, &value, sizeof(INT), &bytesWritten, NULL);
		if (bytesWritten != sizeof(INT)) {
			_ftprintf(stderr, _T("Error writing file: %s\n"), filename);
			CloseHandle(hOut);
			return;
		}
	}

	CloseHandle(hOut);
}

DWORD WINAPI threadFunction(LPVOID parameter) {
	parameter_t* p = (parameter_t*) parameter;

	HANDLE hIn = CreateFile(p->filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open file %s in reading mode. Error: %x\n"),
			p->filename, GetLastError());
		return 1;
	}

	DWORD bytesRead;
	INT n, value;

	ReadFile(hIn, &n, sizeof(INT), &bytesRead, NULL);
	if (bytesRead <= 0) {
		_ftprintf(stderr, _T("Error reading file: %s\n"), p->filename);
		CloseHandle(hIn);
		return 2;
	}

	PINT v = (PINT) malloc(n * sizeof(INT));
	for (INT i = 0; i < n; i++) {
		ReadFile(hIn, &v[i], sizeof(INT), &bytesRead, NULL);
		if (bytesRead <= 0) {
			_ftprintf(stderr, _T("Error reading file: %s\n"), p->filename);
			CloseHandle(hIn);
			return 3;
		}
	}

	CloseHandle(hIn);

	/* Print unsorted array */
#ifndef SKIP_READING
	_tprintf(_T("[%s] %d: "), p->filename, n);
	for (INT i = 0; i < n; i++) {
		_tprintf(_T("%d "), v[i]);
	}
	_tprintf(_T("\n"));
#endif

	/* Sort the array */
	for (INT i = 0; i < n - 1; i++) {
		for (INT j = i + 1; j < n; j++) {
			if (v[i] > v[j]) {
				INT tmp = v[i];
				v[i] = v[j];
				v[j] = tmp;
			}
		}
	}

	/* Print sorted array */
#ifndef SKIP_READING
	_tprintf(_T("[%s] %d: "), p->filename, n);
	for (INT i = 0; i < n; i++) {
		_tprintf(_T("%d "), v[i]);
	}
	_tprintf(_T("\n"));
#endif

	/* Update parameters */
	p->n = n;
	p->values = (PINT)malloc(n * sizeof(INT));
	for (INT i = 0; i < n; i++) {
		p->values[i] = v[i];
	}

	return 0;
}