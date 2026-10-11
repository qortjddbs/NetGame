#include <windows.h>
#include <stdio.h>

DWORD WINAPI MyThread(LPVOID arg)
{
	while (1);
	return 0;
}

int main()
{
	// 우선순위 값의 범위를 출력한다.
	printf("우선순위: %d ~ %d\n", THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_TIME_CRITICAL);

	// CPU 개수를 알아낸다.
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// CPU 개수만큼 스레드를 생성한다.
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++) {
		// 스레드를 생성한다.
		HANDLE hThread = CreateThread(NULL, 0, MyThread, NULL, 0, NULL);
		// 우선순위를 높게 설정한다.
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		CloseHandle(hThread);
	}

	// 우선순위를 낮게 설정한다.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	Sleep(1000);
	printf("주 스레드 실행!\n");
	return 0;
}

// 이론대로라면 주 스레드가 우선순위 7이고, 무한 루프 스레드들이 우선순위 9이므로 32번 줄이 실행될 수 없음
// 근데 Windows에 기아 방지 장치가 있기 때문에 몇 초 뒤에 32번째 줄이 실행되고 프로그램이 종료됨
// => 따라서 스레드는 할 일이 없을 때 Sleep이나 대기 함수로 CPU를 양보해야 한다!