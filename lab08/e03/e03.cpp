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

#define STR_LENGTH (30+1)
#define COMMAND_LENGTH 128
#define BUFFER_LENGHT 128

#define VERSION 'C'
#if VERSION == 'A'
	#define Read(hIn, position) ReadFilePointer(hIn, position)
	#define Write(hIn, position) WriteFilePointer(hIn, position)
#elif VERSION == 'B'
	#define Read(hIn, position) ReadOverlapped(hIn, position)
	#define Write(hIn, position) WriteOverlapped(hIn, position)
#else
	#define Read(hIn, position) ReadLock(hIn, position)
	#define Write(hIn, position) WriteLock(hIn, position)
#endif

typedef struct student_t {
	INT id;
	LONG rn;
	TCHAR name[STR_LENGTH];
	TCHAR surname[STR_LENGTH];
	INT mark;
} student_t;

VOID DisplayMenu();
VOID ReadFilePointer(HANDLE hIn, INT position);
VOID WriteFilePointer(HANDLE hOut, INT position);
VOID ReadOverlapped(HANDLE hIn, INT position);
VOID WriteOverlapped(HANDLE hOut, INT position);
VOID ReadLock(HANDLE hIn, INT position);
VOID WriteLock(HANDLE hOut, INT position);

INT _tmain(INT argc, LPTSTR argv[]) {
	student_t s;
	TCHAR command[COMMAND_LENGTH];
	TCHAR operation;
	INT position;
	BOOL exit = FALSE;

	if (argc != 2) {
		_ftprintf(stderr, _T("Parameter errors %s file_input\n"), argv[0]);
		return 1;
	}

	HANDLE hIn = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"),
			argv[1], GetLastError());
		return 2;
	}

	_tprintf(_T("Reading file %s\n"), argv[1]);
	DWORD bytesRead;
	while (ReadFile(hIn, &s, sizeof(student_t), &bytesRead, NULL) && bytesRead > 0) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
	}

	while (!exit) {
		DisplayMenu();
		_fgetts(command, COMMAND_LENGTH, stdin);
		/* Analyze command */
		if (_stscanf(command, _T("%c"), &operation) != 1) {
			_tprintf(_T("Command contains some errors\n"));
		}
		else {
			switch (operation) {
			case 'r':
			case 'R': 
				/* Read */
				if (_stscanf(command, _T("%*c %d"), &position) != 1) {
					_tprintf(_T("Read command contains some errors\n"));
					break;
				}
				/* Read in the indicated position */
//				ReadFilePointer(hIn, position);
//				ReadOverlapped(hIn, position);
//				ReadLock(hIn, position);
				Read(hIn, position);
				break;
			case 'w':
			case 'W':
				/* Write */
				if (_stscanf(command, _T("%*c %d"), &position) != 1) {
					_tprintf(_T("Write command contains some errors\n"));
					break;
				}
				/* Write in the indicated position */
//				WriteFilePointer(hIn, position);
//				WriteOverlapped(hIn, position);
//				WriteLock(hIn, position);
				Write(hIn, position);
				break;
			case 'e':
			case 'E':
				/* Exit */
				exit = TRUE;
				break;
			default:
				/* Command unrecognized */
				_ftprintf(stderr, _T("Unknown operation %c\n"), operation);
				break;
			}
		}
	}

	CloseHandle(hIn);
	return 0;
}

VOID DisplayMenu() {
	_tprintf(_T(
		"R n -> Read student in position n\n"
		"W n -> Write student in position n\n"
		"E   -> Exit\n"
		"Choice: "
	));
}

VOID ReadFilePointer(HANDLE hIn, INT position) {
	student_t s;
	DWORD bytesRead;
	LARGE_INTEGER pointer;

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	if (SetFilePointerEx(hIn, pointer, NULL, FILE_BEGIN) == FALSE) {
		_ftprintf(stderr, _T("Cannot move pointer in position %d\n"), position);
		return;
	}

	if (ReadFile(hIn, &s, sizeof(student_t), &bytesRead, NULL) && bytesRead == sizeof(student_t)) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
	}
	else {
		_ftprintf(stderr, _T("Error reading in position %d\n"), position);
	}
}

VOID WriteFilePointer(HANDLE hOut, INT position) {
	student_t s;
	DWORD bytesWritten;
	LARGE_INTEGER pointer;
	TCHAR buffer[BUFFER_LENGHT];

	_tprintf(_T("Insert student data <registration number> <name> <surname> <mark>:\n"));
	_fgetts(buffer, BUFFER_LENGHT, stdin);

	/* Analyze buffer */
	if (_stscanf(buffer, _T("%ld %s %s %d"),
		&s.rn, &s.name, &s.surname, &s.mark) != 4) {
		_tprintf(_T("Student data contain some error\n"));
		return;
	}
	s.id = position;

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	if (SetFilePointerEx(hOut, pointer, NULL, FILE_BEGIN) == FALSE) {
		_ftprintf(stderr, _T("Cannot move pointer in position %d\n"), position);
		return;
	}

	if (WriteFile(hOut, &s, sizeof(student_t), &bytesWritten, NULL) && bytesWritten == sizeof(student_t)) {
		_tprintf(_T("Student written correctly\n"));
	}
	else {
		_ftprintf(stderr, _T("Error writing in position %d\n"), position);
	}
}

VOID ReadOverlapped(HANDLE hIn, INT position) {
	student_t s;
	DWORD bytesRead;
	LARGE_INTEGER pointer;
	OVERLAPPED o = {0, 0, 0, 0, NULL};

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	o.Offset = pointer.LowPart;
	o.OffsetHigh = pointer.HighPart;

	if (ReadFile(hIn, &s, sizeof(student_t), &bytesRead, &o) && bytesRead == sizeof(student_t)) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
	}
	else {
		_ftprintf(stderr, _T("Error reading in position %d\n"), position);
	}
}

VOID WriteOverlapped(HANDLE hOut, INT position) {
	student_t s;
	DWORD bytesWritten;
	LARGE_INTEGER pointer;
	TCHAR buffer[BUFFER_LENGHT];
	OVERLAPPED o = {0, 0, 0, 0, NULL};

	_tprintf(_T("Insert student data <registration number> <name> <surname> <mark>:\n"));
	_fgetts(buffer, BUFFER_LENGHT, stdin);

	/* Analyze buffer */
	if (_stscanf(buffer, _T("%ld %s %s %d"),
		&s.rn, &s.name, &s.surname, &s.mark) != 4) {
		_tprintf(_T("Student data contain some error\n"));
		return;
	}
	s.id = position;

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	o.Offset = pointer.LowPart;
	o.OffsetHigh = pointer.HighPart;

	if (WriteFile(hOut, &s, sizeof(student_t), &bytesWritten, &o) && bytesWritten == sizeof(student_t)) {
		_tprintf(_T("Student written correctly\n"));
	}
	else {
		_ftprintf(stderr, _T("Error writing in position %d\n"), position);
	}
}

VOID ReadLock(HANDLE hIn, INT position) {
	student_t s;
	DWORD bytesRead;
	LARGE_INTEGER pointer, size;
	OVERLAPPED o = {0, 0, 0, 0, NULL};

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	o.Offset = pointer.LowPart;
	o.OffsetHigh = pointer.HighPart;
	size.QuadPart = sizeof(student_t);

	if (LockFileEx(hIn, 0, 0, size.LowPart, size.HighPart, &o) == FALSE) {
		_ftprintf(stderr, _T("Error locking file region. Error: %x\n"), GetLastError());
		return;
	}

	if (ReadFile(hIn, &s, sizeof(student_t), &bytesRead, &o) && bytesRead == sizeof(student_t)) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
	}
	else {
		_ftprintf(stderr, _T("Error reading in position %d\n"), position);
	}

	if (UnlockFileEx(hIn, 0, size.LowPart, size.HighPart, &o) == FALSE) {
		_ftprintf(stderr, _T("Error unlocking file region. Error: %x\n"), GetLastError());
	}
}

VOID WriteLock(HANDLE hOut, INT position) {
	student_t s;
	DWORD bytesWritten;
	LARGE_INTEGER pointer, size;
	TCHAR buffer[BUFFER_LENGHT];
	OVERLAPPED o = {0, 0, 0, 0, NULL};

	_tprintf(_T("Insert student data <registration number> <name> <surname> <mark>:\n"));
	_fgetts(buffer, BUFFER_LENGHT, stdin);

	/* Analyze buffer */
	if (_stscanf(buffer, _T("%ld %s %s %d"),
		&s.rn, &s.name, &s.surname, &s.mark) != 4) {
		_tprintf(_T("Student data contain some error\n"));
		return;
	}
	s.id = position;

	pointer.QuadPart = (position - 1) * sizeof(student_t);
	o.Offset = pointer.LowPart;
	o.OffsetHigh = pointer.HighPart;
	size.QuadPart = sizeof(student_t);

	if (LockFileEx(hOut, LOCKFILE_EXCLUSIVE_LOCK, 0, size.LowPart, size.HighPart, &o) == FALSE) {
		_ftprintf(stderr, _T("Error locking file region. Error: %x\n"), GetLastError());
		return;
	}

	if (WriteFile(hOut, &s, sizeof(student_t), &bytesWritten, &o) && bytesWritten == sizeof(student_t)) {
		_tprintf(_T("Student written correctly\n"));
	}
	else {
		_ftprintf(stderr, _T("Error writing in position %d\n"), position);
	}

	if (UnlockFileEx(hOut, 0, size.LowPart, size.HighPart, &o) == FALSE) {
		_ftprintf(stderr, _T("Error unlocking file region. Error: %x\n"), GetLastError());
	}
}