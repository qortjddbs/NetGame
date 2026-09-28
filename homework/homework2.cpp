// 과제 2 - 연습문제 3-6
// 도메인 이름을 명령행 인수로 입력받아, 해당 호스트의 모든 별명과 모든 IPv4 주소를 출력하는 프로그램을 작성하시오. 
// 명령행 인수 방식은 명령프롬프트에서 C > homework2 www.tukorea.ac.kr 의 형태로 실행

#include "..\Server\Common.h"

bool GetAllAliases(const char *name)
{
	struct hostent* ptr = gethostbyname(name);
	if (ptr == NULL) {
		err_display("gethostbyname()");
		return false;
	}
	if (ptr->h_aliases[0] == NULL) {
		printf("해당 호스트는 별명이 존재하지 않습니다.\n");
		return true;
	}
	for (int i = 0; ptr->h_aliases[i] != NULL; ++i) {
		printf("%s의 별명은 %d - %s\n", name, i + 1, ptr->h_aliases[i]);
	}
	return true;
}

// 도메인 이름 -> IPv4 주소
bool GetALLIPAddr(const char* name)
{
	struct hostent* ptr = gethostbyname(name);
	if (ptr == NULL) {
		err_display("gethostbyname()");
		return false;
	}
	if (ptr->h_addrtype != AF_INET)
		return false;
	char str[INET_ADDRSTRLEN];
	for (int i = 0; ptr->h_addr_list[i] != NULL; ++i) {
		inet_ntop(AF_INET, ptr->h_addr_list[i], str, sizeof(str));
		printf("%s의 IPv4 주소는 %d - %s\n", name, i + 1, str);
	}
	return true;
}

int main(int argc, char *argv[])		// C>homework2 www.tukorea.ac.kr 로 실행하면 argc == 2, argv[0]은 프로그램 이름, argv[1]은 www.tukorea.ac.kr
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	GetAllAliases(argv[1]);
	GetALLIPAddr(argv[1]);

	return 0;
}