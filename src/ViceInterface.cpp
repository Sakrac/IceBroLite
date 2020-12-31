#include "winsock2.h"
#include <ws2tcpip.h>
#include <inttypes.h>
#include <stdio.h>

#include "ViceBinInterface.h"
#include "platform.h"

class ViceConnection {
	enum { RECEIVE_SIZE = 1024*1024 };
public:
	static IBThreadRet ViceConnectThread(void *data);
	void connectionThread();

	IBMutex userMutex;

};

static SOCKET s;
static IBThread threadHandle;
static ViceConnection* viceCon = nullptr;

bool ViceConnected()
{
	return false;
}

bool ViceRunning()
{
	return false;
}

void ViceDisconnect()
{

}

void ViceConnect(const char* ip, uint32_t port)
{

}

void ViceBreak()
{

}

void ViceGo()
{

}

void ViceStep()
{

}

void ViceStepOver()
{

}

void ViceStepOut()
{

}

IBThreadRet ViceConnection::ViceConnectThread(void* data)
{
	((ViceConnection*)data)->connectionThread();
	return 0;
}

void ViceConnection::connectionThread()
{
	DWORD timeout = 100;// SOCKET_READ_TIMEOUT_SEC*1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	char* recvBuf = (char*)malloc(RECEIVE_SIZE);
	uint32_t lastRequestID = 0x10000000;
}

void StartVICEConnection()
{
	if (viceCon != nullptr) {
		return;	// already going
	}

	viceCon = new ViceConnection;
	if (viceCon == nullptr) {
		return;	// couldn't create
	}

	IBCreateThread(&threadHandle, 16384, ViceConnection::ViceConnectThread, viceCon);
};
