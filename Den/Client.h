#pragma once
#include <winsock2.h>
#include <tchar.h>
#include <ctime>
#include <list>
#include <math.h>
using namespace std;

HINSTANCE instanse;
HWND hWindow;
HWND hLabelIP;
HWND hLabelPort;
HWND hLabelState;
HWND hLabelCounter;
HWND hLabelPercent;
HWND hTextBoxIP;
HWND hTextBoxPort;
HWND hTextBoxState;
HWND hTextBoxCounter;
HWND hTextBoxPercent;
HWND hButtonConnect;
HWND hButtonSetPercent;

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

#define LABEL_IP 0
#define LABEL_PORT 1
#define LABEL_STATE 2
#define LABEL_COUNTER 3
#define LABEL_PERCENT 4
#define TEXTBOX_IP 5
#define TEXTBOX_PORT 6
#define TEXTBOX_STATE 7
#define TEXTBOX_COUNTER 8
#define TEXTBOX_PERCENT 9
#define BUTTON_CONNNECT 10
#define BUTTON_SET_PERCENT 11
#define PORT 5150
#define CHAR_BUFF_SIZE 32
#define TACTS 6000000
#define DEFAULT_CPU_LOAD 10

WSADATA	wsd;
SOCKET clientSocket;
sockaddr_in	saddrClient;

list<INTEGRAL_PART> integralPartList;
volatile double sum;
//volatile double CPULoad = 10;
volatile int threadCounter;
CRITICAL_SECTION cs;
CRITICAL_SECTION integralPartListCS;

HANDLE *startEvents;
HANDLE endEvent;
INTEGRAL_PART *part;

int CPUCount;
CRITICAL_SECTION csCounter;
DWORD WINAPI integralThread(LPVOID lpParam);
DWORD WINAPI reciveThread(LPVOID lpParam);
LRESULT CALLBACK windowsProcedure(HWND Window, UINT message, WPARAM wParam, LPARAM lParam);