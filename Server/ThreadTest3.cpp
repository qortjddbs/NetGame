#include <windows.h>
#include <stdio.h>

int sum = 0;

DWORD WINAPI MyThread(LPVOID arg)
{
	int num = (int)(long long)arg;
	for (int i = 1; i <= num; i++)
		sum += i;
	return 0;
}

int main(int argc, char *argv[])
{
	int num = 100;
	HANDLE hThread = CreateThread(NULL, 0, MyThread,
		(LPVOID)(long long)num, CREATE_SUSPENDED, NULL);
	// num을 (long long)으로 타입 캐스팅 하는 이유 : LPVOID (포인터 = 8바이트), int는 4바이트여서 그대로 넣으면 경고가 뜨기 때문
	// 5번째 인자 Suspend -> 일단 만들기만하고 대기

	printf("스레드 실행 전. 계산 결과 = %d\n", sum);
	ResumeThread(hThread);
	// 대기중인 쓰레드를 실행 상태로 변경
	WaitForSingleObject(hThread, INFINITE);
	printf("스레드 실행 후. 계산 결과 = %d\n", sum);
	CloseHandle(hThread);
	return 0;
}
