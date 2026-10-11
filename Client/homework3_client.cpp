#include "..\Server\Common.h"

#define SERVERPORT 9000
#define BUFSIZE    4096

int main(int argc, char* argv[])
{
	int retval;

	if (argc < 3) {
		printf("사용법: %s <서버 IP> <파일명>\n", argv[0]);
		return 1;
	}

	const char* server_ip = argv[1];
	const char* file_name = argv[2];

	FILE* fp = fopen(file_name, "rb");
	if (fp == NULL) {
		printf("파일 열기 실패\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	int file_size = (int)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	if (inet_pton(AF_INET, server_ip, &serveraddr.sin_addr) != 1) {
		printf("잘못된 IP 주소 : %s\n", server_ip);
		fclose(fp);
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		// 형식은 맞지만 그 주소에 서버가 없거나 닿지 않는 경우
		int err = WSAGetLastError();
		if (err == WSAETIMEDOUT)
			printf("서버 %s에 연결할 수 없습니다 (시간 초과: IP 주소, 네트워크, 방화벽 확인)\n", server_ip);
		else if (err == WSAECONNREFUSED)
			printf("서버 %s가 연결을 거부했습니다 (서버 실행 여부, 포트 %d 확인)\n", server_ip, SERVERPORT);
		else
			err_display("connect()");
		fclose(fp);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE];
	int len;

	len = (int)strlen(file_name);

	// 파일 이름 길이 보내기(고정 길이)
	retval = send(sock, (char*)&len, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(fp);
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	// 파일 이름 보내기(가변 길이)
	retval = send(sock, file_name, len, 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(fp);
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	// 파일 크기 보내기 (고정 길이)
	retval = send(sock, (char*)&file_size, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(fp);
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	// 파일 데이터 보내기 (가변 길이)
	int n;
	while ((n = (int)fread(buf, 1, BUFSIZE, fp)) > 0) {
		retval = send(sock, buf, n, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}

	printf("[TCP 클라이언트] %s (%d바이트)를 "
		"보냈습니다.\n", file_name, file_size);
	

	// 파일 닫기
	fclose(fp);

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
