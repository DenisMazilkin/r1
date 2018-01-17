#include "Server.h"

DWORD WINAPI receiveThread(LPVOID Param)
{
	SOCKET_AND_ADDR socketAndAddr = *(SOCKET_AND_ADDR*)Param;
	SOCKET socket = socketAndAddr.socket;
	sockaddr_in saddr = socketAndAddr.saddr;
	int clientCPUCount = 1;
	char buf[CHAR_BUFF_SIZE];
	char temp[CHAR_BUFF_SIZE];
	TCHAR wbuf[CHAR_BUFF_SIZE];

	MESSAGE mCPU;
	if (recv(socket, (char*)&mCPU, sizeof(mCPU), 0) < 0) {
		return 0;
	}
	else {
		switch (mCPU.messageType) {
		case CPU_COUNT: //�����������, ������� � ������� �����������
			clientCPUCount = mCPU.clientCPUCount;
			strcpy(buf, inet_ntoa(saddr.sin_addr)); //The inet_ntoa function converts an (Ipv4) Internet network address into an ASCII string
			strcat(buf, ":"); //����������� buf � :
			strcat(buf, itoa(ntohs(saddr.sin_port), temp, 10)); //itoa - ����������� ����� �������� � ������ ��������
			mbstowcs(wbuf, buf, CHAR_BUFF_SIZE); //����������� ������������������ ������������� �������� � ��������������� ������������������ ����������� ��������

			EnterCriticalSection(&listBoxCriticalSection);
			SendMessage(hListBoxClientList, LB_ADDSTRING, 0, (LPARAM)wbuf);
			LeaveCriticalSection(&listBoxCriticalSection);
			clientCount++;
			_itow(clientCount, wbuf, 10); //�������������� clientCount � wbuf, 10� ������� ���������
			SendMessage(hTextBoxClientCount, WM_SETTEXT, 0, (LPARAM)wbuf);
			break;
		}
	}

	while (1) {
		MESSAGE msg;
		MESSAGE in;
		int realPartCountToSend = 0;

		//�������� ������
		if (!WaitForSingleObject(startEvent, 0) == WAIT_OBJECT_0) {
			MESSAGE message;
			message.messageType = END;
			send(socket, (char*)&message, sizeof(message), 0);
			WaitForSingleObject(startEvent, INFINITE);
		}

		//���������� ������
		EnterCriticalSection(&integralPartListCriticalSection);
		for (int i = 0; i < clientCPUCount; i++) {
			if (integralPartList.size() != 0) {
				msg.parts[i] = integralPartList.front();
				integralPartList.pop_front();
				realPartCountToSend++;
			}
		}
		LeaveCriticalSection(&integralPartListCriticalSection);

		//����������
		if (realPartCountToSend > 0) {
			msg.count = realPartCountToSend;
			msg.messageType = PART;

			if ((send(socket, (char*)&msg, sizeof(msg), 0) >= 0) && (recv(socket, (char*)&in, sizeof(in), 0) >= 0)) {
				switch (in.messageType) {
				case RESULT:
					EnterCriticalSection(&integralRezultCriticalSection);
					integralRezult += in.partRezult;
					partCounter -= realPartCountToSend;
					LeaveCriticalSection(&integralRezultCriticalSection);
					break;
				}
			}
			else {
				//���������� ������� �����
				EnterCriticalSection(&integralPartListCriticalSection);
				for (int i = 0; i < realPartCountToSend; i++) {
					integralPartList.push_back(msg.parts[i]);
				}
				LeaveCriticalSection(&integralPartListCriticalSection);

				//������� �������
				strcpy(buf, inet_ntoa(saddr.sin_addr));
				strcat(buf, ":");
				strcat(buf, itoa(ntohs(saddr.sin_port), temp, 10));
				mbstowcs(wbuf, buf, CHAR_BUFF_SIZE);
				EnterCriticalSection(&listBoxCriticalSection);
				int num = SendMessage(hListBoxClientList, LB_FINDSTRING, -1, (LPARAM)wbuf);
				if (num >= 0) {
					SendMessage(hListBoxClientList, LB_DELETESTRING, num, 0);
					clientCount--;
				}
				LeaveCriticalSection(&listBoxCriticalSection);

				//���������� � ����
				if (clientCount == 0) {
					SendMessage(hTextBoxOutTime, WM_SETTEXT, 0, (LPARAM)_itow(0, wbuf, 10));
				}
				SendMessage(hTextBoxClientCount, WM_SETTEXT, 0, (LPARAM)_itow(0, wbuf, 10));
				break;
			}
		}
		if (partCounter == 0) {
			//���� ���, �� ����� �������
			started = false;
			ResetEvent(startEvent);
		}
	}
	return 0;
}
DWORD WINAPI acceptThread(LPVOID Param) {

	while (1){
		SOCKET socketClient;
		sockaddr_in saddrClient;
		int saddrSize = sizeof(saddrClient);
		if ((socketClient = accept(serverSocket, (sockaddr*)&saddrClient, &saddrSize)) == INVALID_SOCKET){
			MessageBox(hWindow, L"������ �������", L"������", MB_OK);
			return 0;
		}
		SOCKET_AND_ADDR socketAndAddr;
		socketAndAddr.socket = socketClient;
		socketAndAddr.saddr = saddrClient;
		CreateThread(0, 0, receiveThread, &socketAndAddr, 0, 0);
	}
	shutdown(serverSocket, SD_BOTH); //�������� �������� � ����� ������������
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}
DWORD WINAPI timerThread(LPVOID Param) {
	WaitForSingleObject(startEvent, INFINITE);
	while (1) {
		TCHAR wbuf[CHAR_BUFF_SIZE];

		//������ ���������
		int newProgress = ((double)(partCount - partCounter) / partCount) * 100;
		oldProgress = newProgress;
		SendMessage(hTextBoxProgress, WM_SETTEXT, 0, (LPARAM)_itow(oldProgress, wbuf, 10));

		//������ ������� 
		if (partCounter != partCount) {
			int tOut = (partCounter * timeCounter) / (partCount - partCounter);
			SendMessage(hTextBoxOutTime, WM_SETTEXT, 0, (LPARAM)_itow(tOut, wbuf, 10));
		}
		timeCounter++;
		if (WaitForSingleObject(startEvent, 0) == WAIT_OBJECT_0) {
			Sleep(1000);
		}
		else {		
			SendMessage(hButtonSolve, WM_SETTEXT, 0, (LPARAM)L"�����");
			swprintf(wbuf, L"%.6f", integralRezult);
			MessageBox(hWindow, wbuf, L"������", MB_OK);
			WaitForSingleObject(startEvent, INFINITE);
		}	
	}
}

LRESULT CALLBACK windowsProcedure(HWND Window, UINT message, WPARAM wParam, LPARAM lParam) {
	int id = LOWORD(wParam), event = HIWORD(wParam);
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message){
		case WM_COMMAND:
			switch (id){
			case BUTTON_SOLVE:
				if (!started) {
					TCHAR wbuf[CHAR_BUFF_SIZE];
					GetWindowText(hTextBoxStep, wbuf, sizeof(wbuf));
					double tempStep = _wtof(wbuf);
					if (tempStep != 0) {
						partCounter = partCount;
						timeCounter = 0;
						integralRezult = 0.0;
						_itow(oldProgress, wbuf, 10);
						SendMessage(hTextBoxProgress, WM_SETTEXT, 0, (LPARAM)wbuf);
						integralPartList.clear();
						for (int i = 0; i < partCount; ++i) {
							INTEGRAL_PART integralPart;
							integralPart.partNumber = (i + 1);
							integralPart.low = low + partSize * (i);
							integralPart.high = integralPart.low + partSize;
							integralPart.step = tempStep;
							integralPart.cpuLoad = cpuLoad;
							integralPartList.push_back(integralPart);
						}
						SetEvent(startEvent);
						started = true;
						SendMessage(hButtonSolve, WM_SETTEXT, 0, (LPARAM)L"����");
					}
				}
				else {
					ResetEvent(startEvent);
					started = false;
					oldProgress = 0;
					timeCounter = 0;
					TCHAR wbuf[CHAR_BUFF_SIZE];
					_itow(0, wbuf, 10);
					SendMessage(hTextBoxOutTime, WM_SETTEXT, 0, (LPARAM)wbuf);
					SendMessage(hButtonSolve, WM_SETTEXT, 0, (LPARAM)L"�����");
				}
				break;
			case BUTTON_CPU_LOAD:
				TCHAR wbuf[CHAR_BUFF_SIZE];
				GetWindowText(hTextBoxCPULoad, wbuf, CHAR_BUFF_SIZE);
				int percent = _wtoi(wbuf);
				if (percent > 0 && percent <= 100) {
					cpuLoad = (double)percent / 100;
				}
				break;
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(Window, &ps);
			EndPaint(Window, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(Window, message, wParam, lParam);
	}
	return 0;
}
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	//������������� ������ ����
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hIcon = 0;
	wcex.hbrBackground = 0;
	wcex.lpszMenuName = 0;
	wcex.hIconSm = 0;
	wcex.lpfnWndProc = windowsProcedure;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = L"server";
	RegisterClassEx(&wcex);

	//���� ���� � ��������
	instanse = hInstance;
	hWindow = CreateWindow(L"server", L"������", WS_SYSMENU, 0, 0, 248, 378, NULL, NULL, hInstance, NULL);
	hLabelBriefing = CreateWindowEx(0, L"Static", L"���:", WS_VISIBLE | WS_CHILD, 4, 4, 40, 20, hWindow, (HMENU)LABEL_BRIEFING, instanse, 0);
	hLabelClientList = CreateWindowEx(0, L"Static", L"�������:", WS_VISIBLE | WS_CHILD, 4, 24, 80, 20, hWindow, (HMENU)LABEL_CLIENT_LIST, instanse, 0);
	hLabelClientCount = CreateWindowEx(0, L"Static", L"����������:", WS_VISIBLE | WS_CHILD, 4, 244, 160, 20, hWindow, (HMENU)LABEL_CLIENT_COUNT, instanse, 0);
	hLabelProgress = CreateWindowEx(0, L"Static", L"���������:", WS_VISIBLE | WS_CHILD, 4, 264, 160, 20, hWindow, (HMENU)LABEL_CLIENT_PROGRESS, instanse, 0);
	hLabelOutTime = CreateWindowEx(0, L"Static", L"��������:", WS_VISIBLE | WS_CHILD, 4, 284, 160, 20, hWindow, (HMENU)LABEL_CLIENT_OUT_TIME, instanse, 0);
	hTextBoxStep = CreateWindowEx(0, L"Edit", L"0.000001", WS_VISIBLE | WS_CHILD | WS_BORDER, 44, 4, 120, 20, hWindow, (HMENU)TEXTBOX_STEP, instanse, 0);
	hTextBoxClientCount = CreateWindowEx(0, L"Edit", L"0", WS_VISIBLE | WS_CHILD | WS_DISABLED, 190, 244, 30, 16, hWindow, (HMENU)TEXTBOX_STEP, instanse, 0);
	hTextBoxProgress = CreateWindowEx(0, L"Edit", L"0", WS_VISIBLE | WS_CHILD | WS_DISABLED, 190, 264, 30, 20, hWindow, (HMENU)TEXTBOX_CLIENT_PROGRESS, instanse, 0);
	hTextBoxOutTime = CreateWindowEx(0, L"Edit", L"0", WS_VISIBLE | WS_CHILD | WS_DISABLED, 190, 284, 40, 20, hWindow, (HMENU)TEXTBOX_CLIENT_OUT_TIME, instanse, 0);
	hListBoxClientList = CreateWindowEx(0, L"ListBox", L"LBox", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | LBS_HASSTRINGS, 4, 44, 226, 196, hWindow, (HMENU)LISTBOX_CLIENTLIST, instanse, 0);
	hButtonSolve = CreateWindowEx(0, L"Button", L"�����", WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 4, 60, 20, hWindow, (HMENU)BUTTON_SOLVE, instanse, 0);
	hLabelCPULoad = CreateWindowEx(0, L"Static", L"��������:", WS_VISIBLE | WS_CHILD, 4, 304, 80, 20, hWindow, (HMENU)LABEL_CPU_LOAD, instanse, 0);
	hTextBoxCPULoad = CreateWindowEx(0, L"Edit", L"10", WS_VISIBLE | WS_CHILD | WS_BORDER, 84, 304, 40, 20, hWindow, (HMENU)CPU_LOAD, instanse, 0);
	hButtonSetCPU = CreateWindowEx(0, L"Button", L"����������", WS_VISIBLE | WS_CHILD | WS_BORDER, 128, 304, 80, 20, hWindow, (HMENU)BUTTON_CPU_LOAD, instanse, 0);
	ShowWindow(hWindow, cmdShow);
	UpdateWindow(hWindow);

	//������������� �������
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		MessageBox(hWindow, L"������ �������� Winsock", L"������", MB_OK);
		return 0;
	}
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == SOCKET_ERROR) {
		MessageBox(hWindow, L"������ �������� ������", L"������", MB_OK);
		return 0;
	}
	serverSaddr.sin_family = AF_INET;
	serverSaddr.sin_addr.s_addr = ADDR_ANY;
	serverSaddr.sin_port = htons(PORT);
	if (bind(serverSocket, (sockaddr*)&serverSaddr, sizeof(serverSaddr)) == SOCKET_ERROR) {
		MessageBox(hWindow, L"������ ���������� ������", L"������", MB_OK);
		return 0;
	}
	startEvent = CreateEvent(NULL, TRUE, FALSE, L"START");
	InitializeCriticalSection(&integralPartListCriticalSection);
	InitializeCriticalSection(&integralRezultCriticalSection);
	InitializeCriticalSection(&listBoxCriticalSection);
	clientCount = 0;
	listen(serverSocket, 4);
	CreateThread(0, 0, &acceptThread, 0, 0, 0);
	CreateThread(0, 0, &timerThread, 0, 0, 0);

	//���� ���������
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}