#include "winsock2.h"
#include <ws2tcpip.h>
#include <inttypes.h>
#include <stdio.h>
#include <queue>
#include <vector>
#include <assert.h>

#include "6510.h"

#include "ViceInterface.h"
#include "ViceBinInterface.h"
#include "platform.h"
#include "struse/struse.h"

//send(s, sendCmd, sendCmdLen, 0);

class ViceConnection {
	enum { RECEIVE_SIZE = 1024*1024 };
	struct ViceMessage {
		int size;	// data follows after len..
	};

	char ipAddress[32];
	uint32_t ipPort;
	bool connected;
	bool stopped;

	std::queue<ViceMessage*> toSend;
public:
	ViceConnection(const char* ip, uint32_t port);
	~ViceConnection();

	static IBThreadRet ViceConnectThread(void *data);
	void connectionThread();

	void updateGetMemory(VICEBinMemGetResponse* resp);

	void updateRegisters(VICEBinRegisterResponse* resp);

	void updateRegisterNames(VICEBinRegisterAvailableResponse* resp);

	bool open();
	bool openConnection();
	void AddMessage(uint8_t *message, int size);

	bool isConnected() { return connected; }
	bool isStopped() { return stopped; }

	IBMutex msgSendMutex;

};

struct GetMemoryRequest {
	uint32_t requestID;
	uint16_t start, end, bank;
	uint8_t space;
};

static SOCKET s;
static IBThread threadHandle;
static ViceConnection* viceCon = nullptr;
static uint32_t lastRequestID = 0x10000000;
static std::vector<GetMemoryRequest> sMemRequests;
static IBMutex userRequestMutex;

ViceConnection::ViceConnection(const char* ip, uint32_t port) : ipPort(port), connected(false), stopped(false)
{
	IBMutexInit(&msgSendMutex, "VICE Send Message Mutex");
	strcpy_s(ipAddress, ip);
}

ViceConnection::~ViceConnection()
{
	IBMutexDestroy(&msgSendMutex);
}

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

bool ViceGetMemory(uint16_t start, uint16_t end, VICEMemSpaces mem)
{
	if (viceCon && viceCon->isConnected() && viceCon->isStopped()) {
		VICEBinMemGetSet getNem(++lastRequestID, false, true, start, end, 0, mem);
		GetMemoryRequest reqInfo = { lastRequestID, start, end, 0, (uint8_t)mem };
		IBMutexLock(&userRequestMutex);
		sMemRequests.push_back(reqInfo);
		IBMutexRelease(&userRequestMutex);
#ifdef _DEBUG
		strown<128> msg("Requested VICE Memory $");
		msg.append_num(start, 4, 16).append("-$").append_num(end, 4, 16).append("\n");
		OutputDebugStringA(msg.c_str());
#endif
		viceCon->AddMessage((uint8_t*)&getNem, sizeof(getNem));
		return true;
	}
	return false;
}

void ViceConnection::connectionThread()
{
	char* recvBuf = (char*)malloc(RECEIVE_SIZE);
	if (!recvBuf) { return; }

	// Open the connection
	if (!open()) { return; }

	DWORD timeout = 100;// SOCKET_READ_TIMEOUT_SEC*1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));


	bool activeConnection = true;
	{
//		VICEBinRegisters regAvailMsg(++lastRequestID, true);
		VICEBinRegisters regMsg(++lastRequestID, false);
//		AddMessage((uint8_t*)&regMsg, sizeof(regMsg));
//		AddMessage((uint8_t*)&regAvailMsg, sizeof(regAvailMsg));
		AddMessage((uint8_t*)&regMsg, sizeof(regMsg));
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

		// messages to receive
		int bytesReceived = recv(s, recvBuf + bufferRead, RECEIVE_SIZE, 0);
		if (bytesReceived == SOCKET_ERROR) {
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
				Sleep(100);
			} else {
				activeConnection = false;
				break;
			}
		} else {
			bufferRead += bytesReceived;

			if (bufferRead >= sizeof(VICEBinResponse) && recvBuf[0] == 2) {
				VICEBinResponse* resp = (VICEBinResponse*)recvBuf;
				uint32_t bytes = resp->GetSize();
				if (bufferRead >= bytes) {
#ifdef _DEBUG
					strown<128> msg("Got VICE respnse: $");
					msg.append_num(resp->commandType, 2, 16).append(" ReqID:").append_num(resp->GetReqID(), 8, 16);
					msg.append(" err: ").append_num(resp->errorCode,2,16).append("\n");

					OutputDebugStringA(msg.c_str());
#endif
					switch (resp->commandType) {
						case VICE_RegistersGet:
							updateRegisters((VICEBinRegisterResponse*)resp);
							break;
						case VICE_RegistersAvailable:
							updateRegisterNames((VICEBinRegisterAvailableResponse*)resp);
							break;
						case VICE_MemGet:
#ifdef _DEBUG
							OutputDebugStringA("Got Memory!");
#endif
							updateGetMemory((VICEBinMemGetResponse*)resp);
							break;
						case VICE_Stopped:
							stopped = true;
							break;
					}
					if (bufferRead > bytes) {
						memmove(recvBuf, recvBuf + bytes, bufferRead - bytes);
						bufferRead -= bytes;
					} else {
						bufferRead = 0;
					}

				}
			}
		}

		// messages to send
		{
			ViceMessage* msg = nullptr;
			IBMutexLock(&msgSendMutex);
			if (toSend.size()) {
				msg = toSend.front(); toSend.pop();
			}
			IBMutexRelease(&msgSendMutex);
			if (msg) {
				send(s, (const char*)(&msg->size + 1), msg->size, 0);
				free(msg);
			}
		}


	}
}

void ViceConnection::updateGetMemory(VICEBinMemGetResponse* resp)
{
	IBMutexLock(&userRequestMutex);

	uint32_t id = resp->GetReqID();
	uint16_t start, end, bank;
	uint8_t space;
	bool found = false;
	for (size_t i = 0; i < sMemRequests.size(); ++i) {
		if (sMemRequests[i].requestID == id) {
			start = sMemRequests[i].start;
			end = sMemRequests[i].end;
			bank = sMemRequests[i].bank;
			space = sMemRequests[i].space;
			found = true;
			sMemRequests.erase(sMemRequests.begin() + i);
			break;
		}
	}
	IBMutexRelease(&userRequestMutex);

	assert(found);
	if (CPU6510* cpu = GetCPU((VICEMemSpaces)space)) {
		cpu->MemoryFromVICE(start, start + resp->bytes[0] + (((uint16_t)resp->bytes[1]) << 8) - 1, resp->data);
	}
}


void ViceConnection::updateRegisters(VICEBinRegisterResponse* resp)
{
	CPU6510* cpu = GetMainCPU();
	for (uint16_t r = 0, n = resp->GetCount(); r < n; ++r) {
		VICEBinRegisterResponse::regInfo& info = resp->aRegs[r];
		switch (info.registerID) {
			case VICE_Acc: cpu->regs.A = info.GetValue8(); break;
			case VICE_X: cpu->regs.X = info.GetValue8(); break;
			case VICE_Y: cpu->regs.Y = info.GetValue8(); break;
			case VICE_PC: cpu->regs.PC = info.GetValue16(); break;
			case VICE_SP: cpu->regs.SP = info.GetValue8(); break;
			case VICE_FL: cpu->regs.FL = info.GetValue8(); break;
			case VICE_LIN: cpu->regs.LIN = info.GetValue16(); break;
			case VICE_CYC: cpu->regs.CYC = info.GetValue16(); break;
			case VICE_00: cpu->regs.ZP00 = info.GetValue8(); break;
			case VICE_01: cpu->regs.ZP01 = info.GetValue8(); break;
		}
#ifdef _DEBUG
		strown<256> d;
		d.append("Reg: $").append_num(info.registerID, 2, 16);
		d.append(" Size: ").append_num(info.registerSize, 2, 10);
		d.append(" Value: $").append_num((uint16_t)info.registerValue[0] +
										(((uint16_t)info.registerValue[1])<<8), 4, 16);
		d.append("\n");
		OutputDebugStringA(d.c_str());
#endif
	}
}

void ViceConnection::updateRegisterNames(VICEBinRegisterAvailableResponse* resp)
{
	VICEBinRegisterAvailableResponse::regInfo* info = &resp->aRegs;
	for (uint16_t r = 0, n = resp->GetCount(); r < n; ++r) {
#ifdef _DEBUG
		strown<256> d;
		d.append("Reg: $").append_num(info->registerID, 2, 16);
		d.append(" Size: ").append_num(info->registerSize, 0, 10);
		d.append(" Bits: ").append_num(info->registerBits, 0, 10);
		strref name((const char*)info->registerName, info->registerNameLen);
		d.append(name).append('\n');
		OutputDebugStringA(d.c_str());
#endif
		info = (VICEBinRegisterAvailableResponse::regInfo *)
			((uint8_t*)info + sizeof(VICEBinRegisterAvailableResponse::regInfo) +
			 info->registerNameLen - 1);
	}

}


bool ViceConnection::open()
{
	WSADATA ws;
	long status = WSAStartup(0x0101, &ws);
	if (status != 0) { return false; }

//	memset(&addr, 0, sizeof(addr));
	s = socket(AF_INET, SOCK_STREAM, 0);

	// Make sure the user has specified a port
	if (ipPort < 0 || ipPort > 65535) { return false; }

	WSADATA wsaData = { 0 };
	int iResult = 0;

	DWORD dwRetval;

	struct sockaddr_in saGNI;
	char hostname[NI_MAXHOST];
	char servInfo[NI_MAXSERV];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return false;
	}
	//-----------------------------------------
	// Set up sockaddr_in structure which is passed
	// to the getnameinfo function
	saGNI.sin_family = AF_INET;

	inet_pton(AF_INET, ipAddress, &(saGNI.sin_addr.s_addr));

	//	saGNI.sin_addr.s_addr =
	//	InetPton(AF_INET, strIP, &ipv4addr)
	//	inet_addr(address);
	saGNI.sin_port = htons(ipPort);

	//-----------------------------------------
	// Call getnameinfo
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

void ViceConnection::AddMessage(uint8_t* message, int size)
{
#ifdef _DEBUG
	VICEBinHeader* hdr = (VICEBinHeader*)message;
	strown<128> str("Send VICE cmd: $");
	str.append_num(hdr->commandType, 2, 16).append(" ReqID: ").append_num(hdr->GetReqID(), 8, 16);
	str.append(" len: ").append_num(hdr->GetSize(), 0, 16).append(" / ").append_num(size, 0, 16).append("\n");
	OutputDebugStringA(str.c_str());
#endif

	ViceMessage* msg = (ViceMessage*)malloc(sizeof(ViceMessage) + size);
	msg->size = size;
	memcpy(&msg->size + 1, message, size);
	IBMutexLock(&msgSendMutex);
	toSend.push(msg);
	IBMutexRelease(&msgSendMutex);
}

IBThreadRet ViceConnection::ViceConnectThread(void* data)
{
	IBMutexInit(&userRequestMutex, "User request VICE operations");
	((ViceConnection*)data)->connectionThread();
	IBMutexDestroy(&userRequestMutex);
	return 0;
}


void ViceConnect(const char* ip, uint32_t port)
{
	if (viceCon != nullptr) {
		return;	// already going
	}

	viceCon = new ViceConnection(ip, port);
	if (viceCon == nullptr) {
		return;	// couldn't create
	}

	IBCreateThread(&threadHandle, 16384, ViceConnection::ViceConnectThread, viceCon);
};
