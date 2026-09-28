#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    4096

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	int len; // 고정 길이 데이터
	char buf[BUFSIZE + 1]; // 가변 길이 데이터

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
			// 파일 이름 크기 받기(고정 길이)
			retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			// 파일 이름 받기(가변 길이)
			retval = recv(client_sock, buf, len, MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			buf[retval] = '\0';

			// 파일 크기 받기(고정 길이)
			int file_size;
			retval = recv(client_sock, (char*)&file_size, sizeof(int), MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			FILE* fp = fopen(buf, "wb");
			if (fp == NULL) {
				printf("파일 생성 실패: %s\n", buf);
				break;
			}

			// 파일 데이터 받기(가변 길이)
			char data[BUFSIZE];
			int received = 0;
			while (received < file_size) {
				int want = file_size - received;	// 남은 양
				if (want > BUFSIZE) want = BUFSIZE;	// 버퍼보다 크면 버퍼만큼만

				retval = recv(client_sock, data, want, 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0)
					break;

				fwrite(data, 1, retval, fp);		// 받은 만큼만 파일에 씀
				received += retval;

				// 수신률 표시: \r로 같은 줄을 덮어써서 스크롤 안됨
				printf("\r[%s] 수신률: %3d%% (%d/%d바이트)", buf, (int)((long long)received * 100 / file_size), received, file_size);
			}
			printf("\n");
			fclose(fp);

			if (received == file_size) printf("[TCP 서버] %s 수신 완료\n", buf);
			else printf("[TCP 서버] %s 수신 실패 (%d/%d 바이트)\n", buf, received, file_size);
		}

		// 소켓 닫기
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
