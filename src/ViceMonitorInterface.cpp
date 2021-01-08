// Testing if the text mode monitor can work alongside the binary connection
#ifdef _WIN32
#include "winsock2.h"
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <netdb.h>
#endif
#include <inttypes.h>
#include "platform.h"
#include "struse/struse.h"
#include "ViceInterface.h"

#ifdef _WIN32
#define VI_SOCKET SOCKET
#else
#define VI_SOCKET int
#define strcpy_s strcpy
#define OutputDebugStringA printf
#define SOCKET_ERROR -1
void Sleep(int ms)
{
	timespec t = { 0, ms * 1000 };
	nanosleep(&t, &t);
}
#endif

class ViceMonitorConnection {
	enum { RECEIVE_SIZE = 1024 };

	char ipAddress[32];
	uint32_t ipPort;
	bool connected;
	bool stopped;

public:
	ViceMonitorConnection(const char* ip, uint32_t port);
	~ViceMonitorConnection();

	static IBThreadRet ViceMonitorThread(void* data);
	void monitorThread();

	bool open();
	void SendMonitorLine(const char* message, int size);

	bool isConnected() { return connected; }
	bool isStopped() { return stopped; }

	IBMutex msgSendMutex;

};

static VI_SOCKET s;
static IBThread threadHandle;
static ViceMonitorConnection* viceMon = nullptr;
static strown<256> sInitialCommand;


ViceMonitorConnection::ViceMonitorConnection(const char* ip, uint32_t port) : ipPort(port), connected(false), stopped(false)
{
	IBMutexInit(&msgSendMutex, "VICE Send Message Mutex");
	strcpy_s(ipAddress, ip);
}

ViceMonitorConnection::~ViceMonitorConnection()
{
	IBMutexDestroy(&msgSendMutex);
}

bool ViceMonitorConnection::open()
{
#ifdef _WIN32	
	WSADATA ws;
	long status = WSAStartup(0x0101, &ws);
	if (status != 0) { return false; }
#endif

	s = socket(AF_INET, SOCK_STREAM, 0);

	// Make sure the user has specified a port
	if (ipPort < 0 || ipPort > 65535) { return false; }

	int32_t iResult = 0;
	int32_t dwRetval;
	sockaddr_in saGNI;
	char hostname[NI_MAXHOST];
	char servInfo[NI_MAXSERV];
	saGNI.sin_family = AF_INET;
	inet_pton(AF_INET, ipAddress, &(saGNI.sin_addr.s_addr));
	saGNI.sin_port = htons(ipPort);
	dwRetval = getnameinfo((struct sockaddr*)&saGNI,
						   sizeof(struct sockaddr),
						   hostname,
						   NI_MAXHOST, servInfo, NI_MAXSERV, NI_NUMERICSERV);

	if (dwRetval != 0) {
		return false;
	}

	iResult = ::connect(s, (struct sockaddr*)&saGNI, sizeof(saGNI));
	return iResult == 0;
}

IBThreadRet ViceMonitorConnection::ViceMonitorThread(void* data)
{
	((ViceMonitorConnection*)data)->monitorThread();
	if ((void*)viceMon == data) {
		viceMon = nullptr;
		delete (ViceMonitorConnection*)data;
	}
	return 0;
}

void ViceMonitorConnection::monitorThread()
{
	char* recvBuf = (char*)malloc(RECEIVE_SIZE);
	if (!recvBuf) { return; }

	// Open the connection
	if (!open()) { return; }

	int32_t timeout = 100000;// SOCKET_READ_TIMEOUT_SEC*1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO | SO_DEBUG, (char*)&timeout, sizeof(timeout));


	bool activeConnection = true;
	{
//		VICEBinRegisters regAvailMsg(++lastRequestID, true);
//		VICEBinRegisters regMsg(++lastRequestID, false);
//		AddMessage((uint8_t*)&regMsg, sizeof(regMsg));
//		AddMessage((uint8_t*)&regAvailMsg, sizeof(regAvailMsg));
//		AddMessage((uint8_t*)&regMsg, sizeof(regMsg));
	}

	size_t bufferRead = 0;
	connected = true;

	while (activeConnection) {
	// close after all commands have been sent?
//		if (closeRequest && !commands.size()) {
//			threadHandle = INVALID_HANDLE_VALUE;
//			close();
//			break;
//		}
		if (sInitialCommand) {
			send(s, sInitialCommand.get(), sInitialCommand.get_len(), NULL);
			sInitialCommand.clear();
		}
		int bytesReceived = recv(s, recvBuf + bufferRead, RECEIVE_SIZE, 0);
		if (bytesReceived == SOCKET_ERROR) {
#ifdef _WIN32
			if (WSAGetLastError() == WSAETIMEDOUT) {
//				if ((state == Vice_None || state == Vice_Running) && stopRequest) {
//					offs = 0;
//					send(s, "r\n", 2, NULL);
//					stopRequest = false;
//				} else if (syncRequest) {
//					syncRequest = false;
//					ClearAllPCBreakpoints();
//					ResetViceBP();
//					send(s, sBundle, (int)strlen(sBundle), NULL);
//					state = Vice_Sync;
//					syncing = true;
//					monitorOn = true;
//					offs = 0;
//					viceRunning = false;
//					viceReloadSymbols = true;
//				}
				Sleep(50);
			} else {
				activeConnection = false;
				break;
			}
#else
			Sleep(50);
#endif
		} else {
			bufferRead += bytesReceived;
			size_t bk = 0;
			size_t consumed = 0;
			while (bufferRead) {
				size_t st = bk;
				for (; bk < bufferRead; ++bk) {
					if (recvBuf[bk] == 0x0a) break;
				}
				if (bk < bufferRead) {
					++bk;
					ViceLog(strref((const char*)(recvBuf + st), strl_t(bk - st)));
				} else {
					if (st) {
						memmove(recvBuf, recvBuf + st, bk - st);
						bufferRead -= st;
						break;
					}
				}
			}
		}
	}
	// connection with VICE was terminated for some reason
	free(recvBuf);
}

void ViceMonitorConnect(const char* ip, uint32_t port)
{
	if (viceMon != nullptr) {
		return;	// already going
	}

	viceMon = new ViceMonitorConnection(ip, port);
	if (viceMon == nullptr) {
		return;	// couldn't create
	}

	IBCreateThread(&threadHandle, 16384, ViceMonitorConnection::ViceMonitorThread, viceMon);
};

void SendViceMonitorLine(const char* message, int size)
{
	if (viceMon == nullptr) {
		sInitialCommand.copy(strref(message, strl_t(size)));
		ViceMonitorConnect("127.0.0.1", 6510);
	} else {
		send(s, message, size, NULL);
	}
}

