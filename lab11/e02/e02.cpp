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

#define ELEMENTS 12
#define MAXVALUE 150

typedef struct _threadParameter {
	LPHANDLE Mutex;				// Protects multiple producers/consumers
	LPHANDLE EmptySemaphore;
	LPHANDLE FullSemaphore;
	CONST INT NumberElements;	// Number of elements to produce/consume
	LPINT InIndex;				// Reading index
	LPINT OutIndex;				// Writing index
	LPINT CommonBuffer;			// Queue
	CONST INT BufferSize;
	CONST INT MaxWaitingTime;
} THREADPARAMETER;

typedef THREADPARAMETER* LPTHREADPARAMETER;

DWORD WINAPI ConsumerThread(LPVOID parameter);
DWORD WINAPI ProducerThread(LPVOID parameter);
VOID WaitThreadPool(LPHANDLE threads, INT nThreads);
VOID CloseHandles(LPHANDLE handles, INT nHandles);

INT _tmain(int argc, LPTSTR argv[]) {
	if (argc < 5) {
		_ftprintf(stderr, _T("Parameter errors %s P C N T\n"), argv[0]);
		Sleep(5000);
		return 1;
	}
	INT NProducers = _ttoi(argv[1]), // Number of producers
		NConsumers = _ttoi(argv[2]), // Number of consumers
		BufferSize = _ttoi(argv[3]), // Buffer size
		Time = _ttoi(argv[4]); // Producing/Consuming time interval

	LPHANDLE Producers = (LPHANDLE)malloc(NProducers * sizeof(HANDLE));
	if (Producers == NULL) {
		_ftprintf(stderr, _T("Error creating first handles array\n"));
		Sleep(5000);
		return 2;
	}

	LPHANDLE Consumers = (LPHANDLE)malloc(NConsumers * sizeof(HANDLE));
	if (Consumers == NULL) {
		_ftprintf(stderr, _T("Error creating second handles array\n"));
		free(Producers);
		Sleep(5000);
		return 3;
	}

	HANDLE MutexConsumers = CreateMutex(NULL, FALSE, _T("MutexConsumers")),
		MutexProducers = CreateMutex(NULL, FALSE, _T("MutexProducers")),
		EmptySemaphore = CreateSemaphore(NULL, BufferSize, BufferSize, _T("Empty")),
		FullSemaphore = CreateSemaphore(NULL, 0, BufferSize, _T("Full"));

	if (MutexConsumers == INVALID_HANDLE_VALUE || MutexProducers == INVALID_HANDLE_VALUE
		|| EmptySemaphore == INVALID_HANDLE_VALUE || FullSemaphore == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error creating semaphores\n"));
		free(Producers);
		free(Consumers);
		Sleep(5000);
		return 4;
	}
	
	LPINT Buffer = (LPINT)malloc(BufferSize * sizeof(INT));
	if (Buffer == NULL) {
		_ftprintf(stderr, _T("Error creating buffer\n"));
		free(Producers);
		free(Consumers);
		Sleep(5000);
		return 5;
	}

	INT InIndex = 0, OutIndex = 0;
	INT AvgProduction = ELEMENTS / NProducers, AvgConsumption = ELEMENTS / NConsumers;
	THREADPARAMETER ConsumerParameters = {&MutexConsumers, &EmptySemaphore, &FullSemaphore, AvgConsumption, &InIndex, &OutIndex, Buffer, BufferSize, Time},
		ProducerParameters = {&MutexProducers, &EmptySemaphore, &FullSemaphore, AvgProduction, &InIndex, &OutIndex, Buffer, BufferSize, Time};

	srand(time(NULL));
	setbuf(stdout, 0);
	INT i;
	
	// Release thread "simultaneously"
	for (i = 0; i < min(NProducers, NConsumers) - 1; i++) {
		Producers[i] = CreateThread(NULL, 0, ConsumerThread, (LPVOID)&ProducerParameters, 0, NULL);
		Consumers[i] = CreateThread(NULL, 0, ProducerThread, (LPVOID)&ConsumerParameters, 0, NULL);
	}

	INT RemainderConsumption = ELEMENTS % NConsumers,
		RemainderProduction = ELEMENTS % NProducers;
	THREADPARAMETER LastConsumerParameters = { &MutexConsumers, &EmptySemaphore, &FullSemaphore, AvgConsumption + RemainderConsumption,
			&InIndex, &OutIndex, Buffer, BufferSize, Time },
		LastProducerParameters = { &MutexConsumers, &EmptySemaphore, &FullSemaphore, AvgProduction + RemainderProduction,
			&InIndex, &OutIndex, Buffer, BufferSize, Time };

	// Release remaining threads, if present
	for (INT j = i; j < NProducers - 1; j++) {
		Producers[i] = CreateThread(NULL, 0, ProducerThread, (LPVOID)&ProducerParameters, 0, NULL);
	}
	Producers[NProducers - 1] = CreateThread(NULL, 0, ProducerThread, (LPVOID)&LastProducerParameters, 0, NULL);

	// Release remaining threads, if present
	for (INT j = i; j < NConsumers - 1; j++) {
		Consumers[i] = CreateThread(NULL, 0, ConsumerThread, (LPVOID)&ConsumerParameters, 0, NULL);
	}
	Consumers[NConsumers - 1] = CreateThread(NULL, 0, ConsumerThread, (LPVOID)&LastConsumerParameters, 0, NULL);

	WaitThreadPool(Producers, NProducers);
	WaitThreadPool(Consumers, NConsumers);
	CloseHandles(Producers, NProducers);
	CloseHandles(Consumers, NConsumers);

	free(Producers);
	free(Consumers);

	Sleep(5000);
	return 0;
}

DWORD WINAPI ConsumerThread(LPVOID parameter) {
	LPTHREADPARAMETER p = (LPTHREADPARAMETER)parameter;

	_tprintf(_T("[C %4d] needs to consume %d elements ...\n"), GetCurrentThreadId(), p->NumberElements);
	for (INT i = 0; i < p->NumberElements; i++) {
		WaitForSingleObject(*p->FullSemaphore, INFINITE);
		WaitForSingleObject(*p->Mutex, INFINITE);

		// Dequeueing
		_tprintf(_T("[C %4d] #%2d dequeueing on index %d ...\n"), GetCurrentThreadId(), *p->OutIndex, i);
//		Sleep(rand() % (p->MaxWaitingTime * 1000));
		INT Value = p->CommonBuffer[*p->OutIndex];
		*p->OutIndex = (*p->OutIndex + 1) % p->BufferSize;
		_tprintf(_T("[C %4d] #%2d ... dequeued\n"), GetCurrentThreadId(), i);

		ReleaseSemaphore(*p->Mutex, 1, NULL);
		ReleaseSemaphore(*p->EmptySemaphore, 1, NULL);
		
		// Consuming
		Sleep(rand() % (p->MaxWaitingTime * 1000));
		_tprintf(_T("[C %4d] #%2d consumed %d\n"), GetCurrentThreadId(), i, Value);
	}

	ExitThread(0);
}

DWORD WINAPI ProducerThread(LPVOID parameter) {
	LPTHREADPARAMETER p = (LPTHREADPARAMETER)parameter;

	_tprintf(_T("[P %4d] needs to produce %d elements ...\n"), GetCurrentThreadId(), p->NumberElements);
	for (INT i = 0; i < p->NumberElements; i++) {
		// Producing ?!
		INT Value = rand() % MAXVALUE + 1;
		_tprintf(_T("[P %4d] #%2d produced %d\n"), GetCurrentThreadId(), i, Value);
		Sleep(rand() % (p->MaxWaitingTime * 1000));

		WaitForSingleObject(*p->EmptySemaphore, INFINITE);
		WaitForSingleObject(*p->Mutex, INFINITE);

		// Enqueueing
		_tprintf(_T("[P %4d] #%2d enqueueing on index %d ...\n"), GetCurrentThreadId(), i, *p->InIndex);
//		Sleep(rand() % (p->MaxWaitingTime * 1000));
		p->CommonBuffer[*p->InIndex] = Value;
		*p->InIndex = (*p->InIndex + 1) % p->BufferSize;
		_tprintf(_T("[P %4d] #%2d ... enqueued\n"), GetCurrentThreadId(), i);

		ReleaseSemaphore(*p->Mutex, 1, NULL);
		ReleaseSemaphore(*p->FullSemaphore, 1, NULL);
	}

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