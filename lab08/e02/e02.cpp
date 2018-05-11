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

typedef struct student_t {
	INT id;
	LONG rn;
	TCHAR name[STR_LENGTH];
	TCHAR surname[STR_LENGTH];
	INT mark;
} student_t;

INT _tmain(INT argc, LPTSTR argv[]) {
	FILE* fpIn;
	HANDLE hOut;
	student_t s;

	if(argc != 3) {
		_ftprintf(stderr, _T("Parameter errors %s file_input file_output\n"), argv[0]);
		return 1;
	}

	fpIn = _tfopen(argv[1], _T("r"));
	if(fpIn == NULL) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"),
			argv[1], GetLastError());
		return 2;
	}

	hOut = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if(hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), 
			argv[2], GetLastError());
		fclose(fpIn);
		return 3;
	}

	while(_ftscanf(fpIn, _T("%d %ld %s %s %d"), 
		&s.id, &s.rn, &s.name, &s.surname, &s.mark) == 5) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
		DWORD bytesWritten;
		WriteFile(hOut, &s, sizeof(student_t), &bytesWritten, NULL);
		if(bytesWritten != sizeof(student_t)) {
			_ftprintf(stderr, _T("Error writing student: %d %ld %s %s %d\n"),
				s.id, s.rn, s.name, s.surname, s.mark);
			fclose(fpIn);
			CloseHandle(hOut);
			return 4;
		}
	}

	_tprintf(_T("File %s correctly written\n"), argv[2]);
	fclose(fpIn);
	CloseHandle(hOut);

	HANDLE hIn = CreateFile(argv[2], GENERIC_READ, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if(hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"),
			argv[2], GetLastError());
		fclose(fpIn);
		return 5;
	}

	_tprintf(_T("Reading file %s to check errors\n"), argv[2]);
	DWORD bytesRead;
	while(ReadFile(hIn, &s, sizeof(student_t), &bytesRead, NULL) && bytesRead > 0) {
		_tprintf(_T("Student read: %d %ld %s %s %d\n"),
			s.id, s.rn, s.name, s.surname, s.mark);
	}

	CloseHandle(hIn);
	
	Sleep(5000);
	return 0;
}