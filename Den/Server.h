#pragma once
#include <winsock2.h>
#include <tchar.h>
#include <ctime>
#include <list>
using namespace std;

HINSTANCE instanse;
HWND hWindow;
HWND hLabelBriefing;
HWND hLabelClientList;
HWND hLabelClientCount;
HWND hLabelProgress;
HWND hLabelOutTime;
HWND hTextBoxStep;
HWND hTextBoxClientCount;
HWND hTextBoxProgress;
HWND hTextBoxOutTime;
HWND hListBoxClientList;
HWND hButtonSolve;
HWND hLabelCPULoad;
HWND hTextBoxCPULoad;
HWND hButtonSetCPU;

typedef enum {
	PART,
	END,
	RESULT,
	CPU_COUNT
}MESSAGE_TYPE;
typedef struct {
	int		partNumber;
	double	low;
	double	high;
	double	step;
	double	cpuLoad;
}INTEGRAL_PART;
typedef struct {
	MESSAGE_TYPE	messageType;
	double			partRezult;
	int				clientCPUCount;
	int				count;
	INTEGRAL_PART	parts[8];
}MESSAGE;
typedef struct {
	SOCKET socket;
	sockaddr_in saddr;
}SOCKET_AND_ADDR;

const int partCount = 100000;
const double low = 0.5;
const double high = 1.0;
const double partSize = (high - low) / partCount;

volatile bool started = false;
volatile int clientCount = 0;
volatile int partCounter;
volatile double integralRezult;

//int oldPartCounter = 0;
int oldProgress = 0;
int timeCounter = 0;
double cpuLoad = 0.1;

HANDLE startEvent;
list<INTEGRAL_PART> integralPartList;
CRITICAL_SECTION integralPartListCriticalSection;
CRITICAL_SECTION integralRezultCriticalSection;
CRITICAL_SECTION listBoxCriticalSection;
WSADATA wsd;
SOCKET serverSocket;
sockaddr_in serverSaddr;

#define LABEL_BRIEFING 0
#define LABEL_CLIENT_LIST 1
#define LABEL_CLIENT_COUNT 2
#define LABEL_CLIENT_PROGRESS 3
#define LABEL_CLIENT_OUT_TIME 4
#define TEXTBOX_STEP 5
#define TEXTBOX_CLIENT_COUNT 6
#define TEXTBOX_CLIENT_PROGRESS 7
#define TEXTBOX_CLIENT_OUT_TIME 8
#define LISTBOX_CLIENTLIST 9
#define BUTTON_SOLVE 10
#define CPU_LOAD 11
#define LABEL_CPU_LOAD 12
#define BUTTON_CPU_LOAD 12
#define PORT 5150
#define CHAR_BUFF_SIZE 32

DWORD WINAPI receiveThread(LPVOID Param);
DWORD WINAPI acceptThread(LPVOID Param);
DWORD WINAPI timerThread(LPVOID Param);
LRESULT CALLBACK windowsProcedure(HWND Window, UINT message, WPARAM wParam, LPARAM lParam);