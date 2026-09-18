#include "Common.h"

int main(int argc, char *argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	/*----------------*/
	/* IPv4 변환 연습 */
	/*----------------*/
	// 원래의 IPv4 주소 출력
	const char *ipv4test = "147.46.114.70";
	printf("IPv4 주소(변환 전) = %s\n", ipv4test);

	// inet_pton() 함수 연습
	struct in_addr ipv4num;							// in_addr : IPv4 주소 하나(32비트)만 담는 구조체 - sockaddr_in 안에있는 sin_addr 필드가 이걸로 선언되어있음
	inet_pton(AF_INET, ipv4test, &ipv4num);			// ipv4test(문자열)를 2진수로 변환하여 ipv4num에 넣어라
	printf("IPv4 주소(변환 후) = %#x\n", ipv4num.s_addr);	// 즉, ipv4num.s_addr은 ULONG(unsigned long)을 뜻함

	// inet_ntop() 함수 연습
	char ipv4str[INET_ADDRSTRLEN];		// INET_ADDRSTRLEN이 16이 아니라 22인 이유 : 원래는 IP주소(15) + 널(1)이면 충분하지만, 
										// Windows쪽 매크로는 같은 이름을 쓰는 다른 API까지 안전하게 커버하려고 마이크로소프트가 좀 더 보수적으로 잡아놓음
										// ex) "255.255.255.255:65535" -> 15자(IP) + 1자(콜론) + 5자(포트 최대 5자리) + 1자(널 종료) = 22

	inet_ntop(AF_INET, &ipv4num, ipv4str, sizeof(ipv4str));
	printf("IPv4 주소(다시 변환 후) = %s\n", ipv4str);
	printf("\n");

	/*----------------*/
	/* IPv6 변환 연습 */
	/*----------------*/
	// 원래의 IPv6 주소 출력
	const char *ipv6test = "2001:0230:abcd:ffab:0023:eb00:ffff:1111";
	printf("IPv6 주소(변환 전) = %s\n", ipv6test);

	// inet_pton() 함수 연습
	struct in6_addr ipv6num;
	inet_pton(AF_INET6, ipv6test, &ipv6num);
	printf("IPv6 주소(변환 후) = 0x");
	for (int i = 0; i < 16; i++)
		printf("%02x", ipv6num.s6_addr[i]);				// %02x : 16진수 숫자 출력할 때 자릿수 맞추기 위해 사용
	printf("\n");

	// inet_ntop() 함수 연습
	char ipv6str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &ipv6num, ipv6str, sizeof(ipv6str));
	printf("IPv6 주소(다시 변환 후) = %s\n", ipv6str);

	// 윈속 종료
	WSACleanup();
	return 0;
}
