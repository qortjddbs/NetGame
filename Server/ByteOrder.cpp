#include "Common.h"

int main(int argc, char *argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 0x => 16진수 표현볍
	u_short x1 = 0x1234;		// 10진수로 4660
	u_long  y1 = 0x12345678;	// 10진수로 305419896
	u_short x2;
	u_long  y2;

	// 호스트 바이트 -> 네트워크 바이트
	// #은 접두어 0x를 앞에 붙여주는 플래그
	// %x는 16진수로 출력하라는 뜻 (%x는 a~f를 소문자로, %X는 A~F를 대문자로 나타냄)
	printf("[호스트 바이트 -> 네트워크 바이트]\n");
	printf("%#x -> %#x\n", x1, x2 = htons(x1));
	printf("%#x -> %#x\n", y1, y2 = htonl(y1));

	// 네트워크 바이트 -> 호스트 바이트
	printf("\n[네트워크 바이트 -> 호스트 바이트]\n");
	printf("%#x -> %#x\n", x2, ntohs(x2));
	printf("%#x -> %#x\n", y2, ntohl(y2));

	// 잘못된 사용 예
	printf("\n[잘못된 사용 예]\n");
	printf("%#x -> %#x\n", x1, htonl(x1));

	// 윈속 종료
	WSACleanup();
	return 0;
}
