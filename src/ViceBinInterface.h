#pragma once

enum VICECommandTypes {
	VICE_MemGet = 0x01,	// side effects, start address, end address, memspace
	VICE_MemSet,
	VICE_CheckpointGet = 0x11,
	VICE_CheckpointSet,
	VICE_CheckpointDelete,
	VICE_CheckpointList,
	VICE_CheckpointToggle,
	VICE_ConditionSet = 0x22,
	VICE_RegistersGet = 0x31,
	VICE_RegistersSet,
	VICE_Dump = 0x41,
	VICE_Undump,
	VICE_ResourceGet = 0x51,
	VICE_ResourceSet,
	VICE_JAM = 0x61,	// this is a response only command
	VICE_Stopped,
	VICE_Resumed,
	VICE_Step = 0x71,
	VICE_KeyboardFeed,
	VICE_StepOut,
	VICE_Ping = 0x81,
	VICE_BanksAvailable,
	VICE_RegistersAvailable,
	VICE_DisplayGet,
	VICE_Exit = 0xaa,	// Exit actyally means Resume
	VICE_Quit = 0xbb,
	VICE_Reset = 0xcc,
	VICE_AutoStart = 0xdd,
};

#define Get4Bytes(x) (x[0] + (((uint32_t)x[1])<<8) + (((uint32_t)x[2])<<16) + (((uint32_t)x[3])<<24))
#define Get2Bytes(x) (x[0] + (((uint16_t)x[1])<<8))

// Command
struct VICEBinHeader {
	uint8_t STX;			// byte 0: 0x02 (STX)
	uint8_t API_ID;			// byte 1 : API version ID(currently 0x01)
	uint8_t length[4];		// byte 2 - 5 : length of body (excluding header)
	uint8_t requestID[4];	// byte 6 - 9 : request id
	uint8_t commandType;	// byte 10 : The numeric command type

	void Setup(uint32_t len, uint32_t req, VICECommandTypes cmd)
	{
		STX = 2;
		API_ID = 1;
		length[0] = (uint8_t)len;
		length[1] = (uint8_t)(len>>8);
		length[2] = (uint8_t)(len>>16);
		length[3] = (uint8_t)(len>>24);
		requestID[0] = (uint8_t)req;
		requestID[1] = (uint8_t)(req >> 8);
		requestID[2] = (uint8_t)(req >> 16);
		requestID[3] = (uint8_t)(req >> 24);
		commandType = (uint8_t)cmd;
	}
	uint32_t GetReqID()
	{
		return (uint32_t)requestID[0] + (((uint32_t)requestID[1]) << 8) + (((uint32_t)requestID[2]) << 16) + (((uint32_t)requestID[3]) << 24);
	}
	uint32_t GetLength()
	{
		return (uint32_t)length[0] + (((uint32_t)length[1]) << 8) + (((uint32_t)length[2]) << 16) + (((uint32_t)length[3]) << 24);
	}
	uint32_t GetSize()
	{
		return GetLength() + sizeof(VICEBinHeader);
	}

};

struct VICEBinResponse {
	uint8_t STX;			// byte 0: 0x02 (STX)
	uint8_t API_ID;			// byte 1 : API version ID(currently 0x01)
	uint8_t length[4];		// byte 2 - 5 : length of body (excluding header)
	uint8_t commandType;	// byte 6 : The numeric command type
	uint8_t errorCode;		// byte 7 : error code
	uint8_t requestID[4];	// byte 8-11 : request ID or -1 for debugger initiated

	uint32_t GetLength() {
		return (uint32_t)length[0] + (((uint32_t)length[1]) << 8) + (((uint32_t)length[2]) << 16) + (((uint32_t)length[3]) << 24);
	}
	uint32_t GetSize() {
		return GetLength() + sizeof(VICEBinResponse);
	}
	uint32_t GetReqID()
	{
		return (uint32_t)requestID[0] + (((uint32_t)requestID[1]) << 8) + (((uint32_t)requestID[2]) << 16) + (((uint32_t)requestID[3]) << 24);
	}
};

enum VICERegID {
	VICE_Acc,
	VICE_X,
	VICE_Y,
	VICE_PC,			// 16 bits
	VICE_SP,
	VICE_FL,
	VICE_LIN = 0x35,	// 16 bits
	VICE_CYC,			// 16 bits (??)
	VICE_00,
	VICE_01
};

enum VICEBinResponseErrors {
	VICEResponse_OK,
	VICEResponse_DoesntExist,
	VICEResponse_InvalidMemSpace,
	VICEResponse_IncorrectLength = 0x80,
	VICEResponse_InvalidParam,
	VICEResponse_InvalidAPI_ID,
	VICEResponse_Invalid_Cmd,
	VICEResponse_General_Failure = 0x8f
};

enum VICECheckpointOperations {
	VICE_LoadMem = 1,
	VICE_StoreMem = 2,
	VICE_Exec = 4
};

enum VICEDisplayFormats {
	VICEDisplay_Indexed,	// 8 bit indexed
	VICEDisplay_RGB,		// 24 bit
	VICEDisplay_BGR,		// 24 bit
	VICEDisplay_RGBA,		// 32 bit
	VICEDisplay_BGRA,		// 32 bit
};

struct VICEBinGetMemory : public VICEBinHeader {
	uint8_t sideEffects;
	uint8_t startAddress[2];
	uint8_t endAddress[2];
	uint8_t memSpace;


};

// same struct for getting and setting, setting is appended by bytes to set
struct VICEBinMemGetSet : public VICEBinHeader {
	uint8_t sideEffects;
	uint8_t startAddress[2];
	uint8_t endAddress[2];
	uint8_t memSpace;
	uint8_t bankID[2];

	uint16_t GetStart() {
		return (uint32_t)startAddress[0] + (((uint32_t)startAddress[1]) << 8);
	}
	uint16_t GetEnd() {
		return (uint32_t)endAddress[0] + (((uint32_t)endAddress[1]) << 8);
	}
	uint16_t GetBank() {
		return (uint32_t)bankID[0] + (((uint32_t)bankID[1]) << 8);
	}

	void Setup(uint32_t req, bool sideFX, bool get, uint16_t start, uint16_t end, uint16_t bank, VICEMemSpaces space)
	{
		VICEBinHeader::Setup(8 + (get ? 0 : (end-start+1)), req, get ? VICE_MemGet : VICE_MemSet);
		sideEffects = (uint8_t)sideFX;
		startAddress[0] = (uint8_t)start; startAddress[1] = (uint8_t)(start >> 8);
		endAddress[0] = (uint8_t)end; endAddress[1] = (uint8_t)(end >> 8);
		bankID[0] = (uint8_t)bank; bankID[1] = (uint8_t)(bank >> 8);
		memSpace = space;
	}

	VICEBinMemGetSet(uint32_t req, bool sideFX, bool get, uint16_t start, uint16_t end, uint16_t bank, VICEMemSpaces space = VICE_MainMemory)
	{
		Setup(req, sideFX, get, start, end, bank, space);
	}
};

struct VICEBinMemGetResponse : public VICEBinResponse {
	uint8_t bytes[2];
	uint8_t data[1];
};

struct VICEBinCheckpointList : public VICEBinResponse {
	uint8_t count[4];
	uint32_t GetCount() {
		return Get4Bytes(count);
	}
};
// same struct for get and delete
// also used for checkpoint condition follwed by condition string
struct VICEBinCheckpoint : public VICEBinHeader {
	uint8_t number[4];
	void SetNumber(uint32_t bkNumber) {
		number[0] = (uint8_t)bkNumber;
		number[1] = (uint8_t)(bkNumber >> 8);
		number[2] = (uint8_t)(bkNumber >> 16);
		number[3] = (uint8_t)(bkNumber >> 24);
	}
	uint32_t GetNumber() {
		return (uint32_t)number[0] + (((uint32_t)number[1]) << 8) + (((uint32_t)number[2]) << 16) + (((uint32_t)number[3]) << 24);
	}
};

struct VICEBinCheckpointToggle : public VICEBinCheckpoint {
	uint8_t enabled;
};

struct VICEBinCheckpointSet : public VICEBinHeader {
	uint8_t startAddress[2];
	uint8_t endAddress[2];
	uint8_t stopWhenHit;
	uint8_t enabled;
	uint8_t operation;
	uint8_t temporary;

	void SetStart(uint16_t start) {
		startAddress[0] = (uint8_t)start;
		startAddress[1] = (uint8_t)(start >> 8);
	}
	void SetEnd(uint16_t end)
	{
		endAddress[0] = (uint8_t)end;
		endAddress[1] = (uint8_t)(end >> 8);
	}

	uint16_t GetStart() {
		return (uint32_t)startAddress[0] + (((uint32_t)startAddress[1]) << 8);
	}
	uint16_t GetEnd() {
		return (uint32_t)endAddress[0] + (((uint32_t)endAddress[1]) << 8);
	}
};

struct VICEBinCheckpointResponse : public VICEBinResponse {
	uint8_t number[4];
	uint8_t wasHit;	// current hit BP?
	uint8_t startAddress[2];
	uint8_t endAddress[2];
	uint8_t stopWhenHit;
	uint8_t enabled;
	uint8_t operation;
	uint8_t temporary;
	uint8_t hitCount[4];
	uint8_t ignoreCount[4];
	uint8_t hasCondition;

	uint32_t GetNumber() {
		return (uint32_t)number[0] + (((uint32_t)number[1]) << 8) + (((uint32_t)number[2]) << 16) + (((uint32_t)number[3]) << 24);
	}
	uint16_t GetStart() {
		return (uint32_t)startAddress[0] + (((uint32_t)startAddress[1]) << 8);
	}
	uint16_t GetEnd() {
		return (uint32_t)endAddress[0] + (((uint32_t)endAddress[1]) << 8);
	}
	uint32_t GetCount() {
		return (uint32_t)hitCount[0] + (((uint32_t)hitCount[1]) << 8) + (((uint32_t)hitCount[2]) << 16) + (((uint32_t)hitCount[3]) << 24);
	}
	uint32_t GetIgnored() {
		return (uint32_t)ignoreCount[0] + (((uint32_t)ignoreCount[1]) << 8) + (((uint32_t)ignoreCount[2]) << 16) + (((uint32_t)ignoreCount[3]) << 24);
	}

};

// used for get and available listing
struct VICEBinRegisters : public VICEBinHeader {
	uint8_t memSpace;

	void Setup(uint32_t req, bool names, VICEMemSpaces space = VICE_MainMemory)
	{
		VICEBinHeader::Setup(1, req, names ? VICE_RegistersAvailable : VICE_RegistersGet);
		memSpace = (uint8_t)space;
	}

	VICEBinRegisters(uint32_t req, bool names, VICEMemSpaces space = VICE_MainMemory) {
		Setup(req, names, space);
	}
};

struct VICEBinRegisterResponse : public VICEBinResponse {
	struct regInfo {
		uint8_t registerSize; // size of register value + 1
		uint8_t registerID;
		uint8_t registerValue[2];
		uint16_t GetValue16() { return registerValue[0] + (((uint16_t)registerValue[1]) << 8); }
		uint8_t GetValue8() { return registerValue[0]; }
	};
	uint8_t numRegs[2];
	regInfo aRegs[1];
	uint16_t GetCount() {
		return (uint32_t)numRegs[0] + (((uint32_t)numRegs[1]) << 8);
	}
};

struct VICEBinRegisterAvailableResponse : public VICEBinResponse {
	struct regInfo {
		uint8_t registerSize; // size of register value + 1
		uint8_t registerID;
		uint8_t registerBits;
		uint8_t registerNameLen;
		uint8_t registerName[1];
	};
	uint8_t numRegs[2];
	regInfo aRegs;
	uint16_t GetCount()
	{
		return (uint32_t)numRegs[0] + (((uint32_t)numRegs[1]) << 8);
	}
};


struct VICEBinRegisterSetSingle {
	uint8_t itemSize; // excluding self
	uint8_t regID;
	uint8_t value[2]; // 1 or 2 bytes
};

struct VICEBinRegisterSet : public VICEBinHeader {
	uint8_t memSpace;
	uint8_t count[2];
	VICEBinRegisterSetSingle regs[1];
};


// for JAM, Stopped, Resumed
struct VICEBinStopResponse : public VICEBinResponse {
	uint8_t	programCounter[2];
	uint16_t GetPC() {
		return (uint32_t)programCounter[0] + (((uint32_t)programCounter[1]) << 8);
	}
};

struct VICEBinStep : public VICEBinHeader {
	uint8_t stepOver;
	uint8_t numSteps[2];
	void SetStepCount(uint16_t steps) {
		numSteps[0] = (uint8_t)steps;
		numSteps[1] = (uint8_t)(steps >> 8);
	}
	void Setup(uint32_t reqID, bool stpOver, uint16_t numSteps = 1) {
		VICEBinHeader::Setup(3, reqID, VICE_Step);
		stepOver = stpOver;
		SetStepCount(numSteps);
	}
};

// send keyboard, use escape codes for return etc.
struct VICEBinKeyboard : public VICEBinHeader {
	uint8_t length;
};

struct VICEBinDisplay : public VICEBinHeader {
	uint8_t useVICII;
	uint8_t format;

	void Setup(uint32_t reqID, VICEDisplayFormats fmt) {
		VICEBinHeader::Setup(2, reqID, VICE_DisplayGet);
		useVICII = 1;
		format = (uint8_t)fmt;
	}

	VICEBinDisplay(uint32_t reqID, VICEDisplayFormats fmt) {
		Setup(reqID, fmt);
	}
};

struct VICEBinDisplayResponse : public VICEBinResponse {
	uint8_t lengthField[4];	// Length of fields before reserved area
	uint8_t lengthBeforeReserved[4];
	uint8_t lengthDisplay[4]; // Length of display buffer
	uint8_t width[2]; // Debug width of display buffer(uncropped)  The largest width the screen gets
	uint8_t height[2]; // Debug height of display buffer(uncropped) The largest height the screen gets.
	uint8_t offsScreenX[2]; // X offset to the inner part of the screen.
	uint8_t offsScreenY[2]; // Y offset to the inner part of the screen.
	uint8_t widthScreen[2]; // Width of the inner part of the screen.
	uint8_t heightScreen[2]; // Height of the inner part of the screen.
	uint8_t bitsPerPixel;	// Bits per pixel of display buffer, 8, 24 or 32
	uint8_t lengthReserved[4]; // Length of the reserved area
	// display buffer bytes follow

	uint32_t GetLengthField() { return Get4Bytes(lengthField); }
	uint32_t GetLengthDisplay() { return Get4Bytes(lengthDisplay); }
	uint32_t GetWidthImage() { return Get2Bytes(width); }
	uint32_t GetHeightImage() { return Get2Bytes(height); }
	uint32_t GetLeftScreen() { return Get2Bytes(offsScreenX); }
	uint32_t GetTopScreen() { return Get2Bytes(offsScreenY); }
	uint32_t GetWidthScreen() { return Get2Bytes(widthScreen); }
	uint32_t GetHeightScreen() { return Get2Bytes(heightScreen); }
};

struct VICEBinAutoStart : public VICEBinHeader {
	uint8_t startImmediately;
	uint8_t fileIndex[2]; // The index of the file to execute, if a disk image. 0x00 is the default value.
	uint8_t fileNameLength;
	// filename follows
	char filename[PATH_MAX_LEN];
};

struct VICEBinReset : public VICEBinHeader {
	uint8_t resetType; // 0x00: Soft reset system, 0x01 : Hard reset system, 0x08 - 0x0b : Reset drives 8 - 11
};