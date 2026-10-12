#include "..\Server\Common.h"

#define SERVERPORT 9000
#define BUFSIZE    4096

// [공유 자원] 모든 클라이언트 스레드가 함께 사용하는 전역 변수들
// - cs        : 공유 자원을 한 번에 한 스레드만 사용하도록 막는 임계 영역 객체.
//               main()에서 스레드를 만들기 전에 초기화하고, 프로그램 끝에서 삭제한다.
// - g_nextLine: 다음에 접속하는 클라이언트에게 나눠 줄 콘솔 줄 번호.
//               0번 줄은 비워 두고(필요하면 서버 상태 표시용), 클라이언트는 2번 줄부터 한 줄씩 차지한다.
//               여러 스레드가 동시에 읽고 증가시키므로 반드시 임계 영역 안에서만 다룬다.
CRITICAL_SECTION cs;
int g_nextLine = 2;

// [스레드 함수] 클라이언트 한 명과의 파일 수신을 전담한다.
// - 주 스레드가 accept()로 연결을 받을 때마다 CreateThread()로 이 함수를 실행하는 스레드를 하나씩 만든다.
// - 스레드 함수의 형태는 DWORD WINAPI 함수이름(LPVOID arg)로 정해져 있다.
// - 이 함수가 return 하면 스레드가 종료된다.
DWORD WINAPI ProcessClient(LPVOID arg)
{
	// 아래 지역 변수들은 스레드마다 각자의 스택에 따로 만들어진다.
	// 그래서 클라이언트 A와 B의 스레드가 같은 함수를 실행해도 buf, len 등이 서로 섞이지 않는다.
	// (과제 3에서는 main()의 변수였지만, 여러 스레드가 공유하면 파일명이 덮어써지므로 이곳으로 옮겼다.)
	int retval;

	// CreateThread()의 네 번째 인자로 넘어온 값을 다시 SOCKET으로 되돌린다.
	// 주 스레드는 소켓의 "주소"가 아니라 "값"을 넘겼으므로, 주 스레드가 다음 accept()에서
	// client_sock 변수를 덮어써도 이 스레드가 가진 소켓 값은 바뀌지 않는다.
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in client_addr;
	char addr[INET_ADDRSTRLEN];
	int addr_len;
	int len; // 고정 길이 데이터
	char buf[BUFSIZE + 1]; // 가변 길이 데이터

	// 클라이언트 정보 얻기
	// 스레드에는 소켓 하나만 넘어왔기 때문에 main()의 clientaddr를 볼 수 없다.
	// getpeername()은 "연결된 소켓"에서 상대방(클라이언트)의 주소를 직접 꺼내 준다.
	addr_len = sizeof(client_addr);
	getpeername(client_sock, (struct sockaddr*)&client_addr, &addr_len);
	inet_ntop(AF_INET, &client_addr.sin_addr, addr, sizeof(addr));	// 이진 주소 → "192.168.0.10" 같은 문자열
	int myLine;	// 이 클라이언트가 수신률을 표시할 콘솔 줄 번호

	// [임계 영역 ①] 줄 번호 배정
	// g_nextLine++ 은 한 줄이지만 실제로는 "읽기 → 1 더하기 → 쓰기" 세 단계로 실행된다.
	// 두 클라이언트가 거의 동시에 접속해 두 스레드가 이 단계를 섞어서 실행하면
	// 둘 다 같은 줄 번호를 받아 한 줄에 서로 덮어쓰게 되므로, 임계 영역으로 감싸 한 번에 한 스레드만 실행하게 한다.
	EnterCriticalSection(&cs);
	myLine = g_nextLine++;
	LeaveCriticalSection(&cs);

	// 클라이언트와 데이터 통신 (과제 3과 같은 고정 길이 + 가변 길이 방식)
	// recv()는 블로킹 함수라 데이터가 올 때까지 이 스레드를 멈추게 한다.
	// 하지만 멈추는 것은 이 클라이언트의 스레드뿐이므로, 주 스레드는 계속 accept()를 할 수 있고
	// 다른 클라이언트의 스레드도 각자 수신을 계속한다. (멀티스레드로 바꾼 이유)
	// 또한 기다리는 동안 스레드는 대기 상태가 되어 CPU를 쓰지 않으므로, 같은 우선순위의 다른 스레드가 굶지 않는다.
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

		// 받은 파일 이름 그대로 서버의 작업 폴더에 파일을 만든다(같은 이름이 있으면 덮어씀).
		// 클라이언트마다 파일 이름이 다르므로 스레드끼리 같은 파일에 쓰지 않는다.
		FILE* fp = fopen(buf, "wb");
		if (fp == NULL) {
			// [임계 영역 ②] 콘솔 출력 (아래 수신률 출력과 같은 방식)
			EnterCriticalSection(&cs);
			COORD pos = { 0, (SHORT)myLine };
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
			printf("파일 생성 실패: %s", buf);
			LeaveCriticalSection(&cs);
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

			// [임계 영역 ②] 수신률 출력
			// 과제 3의 \r은 커서를 "현재 줄"의 맨 앞으로만 옮긴다. 여러 스레드가 동시에 \r로 출력하면
			// 같은 줄을 서로 덮어쓰므로, 스레드마다 받은 줄 번호(myLine)로 커서를 직접 옮겨 자기 줄에만 출력한다.
			// - COORD: 콘솔 화면의 위치(X = 칸, Y = 줄)를 나타내는 구조체. { 0, myLine } = 내 줄의 맨 앞
			// - GetStdHandle(STD_OUTPUT_HANDLE): 이 프로그램의 콘솔 출력 창 핸들
			// - SetConsoleCursorPosition(): 콘솔 커서를 지정한 위치로 옮기는 Windows API (강의자료에는 없는 함수)
			// "커서 이동"과 "printf"는 두 단계라서, 그 사이에 다른 스레드가 자기 줄로 커서를 옮겨 버리면
			// 이 스레드의 출력이 다른 클라이언트의 줄에 찍힌다. 그래서 두 단계를 임계 영역으로 묶는다.
			// 또 콘솔 커서는 프로세스 안의 모든 스레드가 함께 쓰는 공유 자원이므로 줄 번호와 같은 cs로 보호한다.
			// 출력 끝에 \n을 넣지 않아 커서가 아래로 내려가지 않으므로 화면이 scroll up 되지 않는다.
			EnterCriticalSection(&cs);
			COORD pos = { 0, (SHORT)myLine };
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
			printf("%s - [%s] 수신률: %3d%% (%d/%d바이트)", addr, buf, (int)((long long)received * 100 / file_size), received, file_size);
			LeaveCriticalSection(&cs);
		}
		fclose(fp);

		// [임계 영역 ②] 수신 결과 출력
		// 같은 줄을 덮어쓰는 것이라, 새 문장이 이전 문장보다 짧으면 이전 글자의 꼬리가 남는다.
		// - 완료 문장은 수신률 문장보다 길어서 이전 글자를 모두 덮는다.
		// - 실패 문장은 더 짧을 수 있어 끝에 공백을 붙여 남은 글자를 지운다.
		EnterCriticalSection(&cs);
		COORD pos = { 0, (SHORT)myLine };
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
		if (received == file_size) printf("[%s] %s 수신 완료: %3d%% (%d/%d바이트)", addr, buf, (int)((long long)received * 100 / file_size), received, file_size);
		else printf("[%s] %s 수신 실패 (%d/%d 바이트)                   ", addr, buf, received, file_size);
		LeaveCriticalSection(&cs);
	}

	// 소켓 닫기
	// 클라이언트 소켓은 그 소켓을 맡은 스레드가 직접 닫는다.
	// 클라이언트가 연결을 끊으면 recv()가 0을 리턴해 위 while 문을 빠져나와 이곳에 도착한다.
	closesocket(client_sock);
	return 0;	// 스레드 함수가 리턴하면 스레드가 종료된다.
}

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
	// INADDR_ANY: 서버 PC의 모든 IP 주소로 들어오는 접속을 받는다.
	// (다른 컴퓨터의 클라이언트와 같은 컴퓨터의 클라이언트 모두 서버의 실제 IP로 접속할 수 있다.)
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
	HANDLE hThread;

	// 임계 영역 초기화: 스레드를 만들기 "전에" 해야 한다.
	// (스레드가 초기화되지 않은 임계 영역에 들어가면 안 되기 때문)
	InitializeCriticalSection(&cs);

	// 콘솔 화면을 지운다. 이렇게 해야 화면 맨 위가 0번 줄이 되어
	// SetConsoleCursorPosition()으로 지정하는 줄 번호와 실제 화면 위치가 일치한다.
	system("cls");

	// [주 스레드의 역할] 접속을 받고 스레드를 만드는 일만 반복한다.
	// 실제 파일 수신은 스레드에게 맡기고 곧바로 다시 accept()로 돌아오므로,
	// 한 클라이언트가 파일을 보내는 동안에도 다른 클라이언트의 접속을 받을 수 있다.
	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 스레드 생성 - SOCKET은 64비트 기준 8바이트. LPVOID(포인터)랑 크기가 같아 바로 변환 가능 -> 중간 캐스팅 불필요
		// CreateThread(보안 속성, 스택 크기, 스레드 함수, 스레드 함수에 넘길 인자, 생성 옵션, 스레드 ID)
		// - 보안 속성 NULL, 스택 크기 0: 기본값 사용
		// - 생성 옵션 0: 만들자마자 바로 실행 (CREATE_SUSPENDED를 주면 ResumeThread() 전까지 멈춤)
		// - &client_sock(주소)을 넘기면 다음 accept()에서 값이 덮어써질 수 있으므로 소켓 "값"을 넘긴다.
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);

		// 스레드 생성에 실패하면 이 클라이언트를 처리할 스레드가 없으므로 소켓을 바로 닫는다.
		// 성공하면 핸들을 바로 닫는다. 주 스레드는 이 스레드의 종료를 기다릴(WaitForSingleObject) 필요가 없기 때문이다.
		// CloseHandle()은 핸들만 반납하는 것이고 스레드를 종료시키지 않는다. 스레드는 계속 실행된다.
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	// 임계 영역 삭제: 모든 스레드가 더 이상 사용하지 않을 때 삭제한다.
	// (이 서버는 accept()가 실패할 때만 while 문을 빠져나오므로, 보통은 여기까지 오지 않는다.)
	DeleteCriticalSection(&cs);
	WSACleanup();
	return 0;
}