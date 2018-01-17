#include "Client.h"

DWORD WINAPI integralThread(LPVOID lpParam)
{
	int threadNumber = (int)lpParam;
	while (1) {
		WaitForSingleObject(startEvents[threadNumber], INFINITE);
		INTEGRAL_PART integralRart = part[threadNumber];
		int start = 0, stop = 0, workTime = 0, sleepTime = 0, rest = 0;
		double partSum = 0;
		double cpu_load = integralRart.cpuLoad;
		double x = integralRart.low + integralRart.step;
		double prev_x = integralRart.low;
		while (x < integralRart.high) {
			int counter = 0;
			start = clock();
			while (counter < TACTS && x < integralRart.high) {
				partSum += exp(x * x) / x;
				x += integralRart.step;
				prev_x += integralRart.step;
				counter++;
			}
			stop = clock();
			workTime += stop - start;

			start = clock();
			rest = workTime * (1 / cpu_load) - workTime - sleepTime;

			if (rest >= 0) {
				Sleep(rest);
			}
			stop = clock();
			sleepTime += stop - start;
		}
		partSum *= integralRart.step;
		EnterCriticalSection(&cs);
		sum += partSum;
		LeaveCriticalSection(&cs);

		ResetEvent(startEvents[threadNumber]);
		EnterCriticalSection(&csCounter);
		threadCounter--;
		if (threadCounter <= 0) {
			SetEvent(endEvent);
		}
		LeaveCriticalSection(&csCounter);
	}
}
DWORD WINAPI reciveThread(LPVOID lpParam) {
	int workCount = 0;
	MESSAGE message;
	part = new INTEGRAL_PART[CPUCount];
	while (1) {
		if (recv(clientSocket, (char*)&message, sizeof(message), 0) >= 0) {
			sum = 0;
			switch (message.messageType) {
			case PART:
				SendMessage(hTextBoxState, WM_SETTEXT, 0, (LPARAM)L"Обработка");

				EnterCriticalSection(&csCounter);
				threadCounter = message.count;
				LeaveCriticalSection(&csCounter);
				ResetEvent(endEvent);
				for (int i = 0; i < message.count; i++) {
					part[i] = message.parts[i];
					SetEvent(startEvents[i]);
				}
				
				WaitForSingleObject(endEvent, INFINITE);

				workCount++;
				TCHAR wbuf[CHAR_BUFF_SIZE];
				_itow(workCount, wbuf, 10);
				SendMessage(hTextBoxCounter, WM_SETTEXT, 0, (LPARAM)wbuf);

				MESSAGE out;
				out.messageType = RESULT;
				out.partRezult = sum;
				send(clientSocket, (char*)&out, sizeof(out), 0);
				break;
			case END:
				SendMessage(hTextBoxState, WM_SETTEXT, 0, (LPARAM)L"Ожидание");
				break;
			default:
				MessageBox(hWindow, L"Неверные данные на клиенте", L"Ошибка", MB_OK);
				return 0;
			}
		}
		else {
			MessageBox(hWindow, L"Ошибка приема на клиенте", L"Ошибка", MB_OK);
			break;
		}
	}
	return 0;
}

LRESULT CALLBACK windowsProcedure(HWND Window, UINT message, WPARAM wParam, LPARAM lParam) {
	int id = LOWORD(wParam), event = HIWORD(wParam);
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR wbuf[CHAR_BUFF_SIZE];
	switch (message) {
	case WM_COMMAND:
		switch (id) {
		case BUTTON_CONNNECT:
			saddrClient.sin_family = AF_INET;
			TCHAR wbuf[CHAR_BUFF_SIZE];
			char buf[CHAR_BUFF_SIZE];
			GetWindowText(hTextBoxIP, wbuf, sizeof(wbuf));
			wcstombs(buf, wbuf, CHAR_BUFF_SIZE);
			saddrClient.sin_addr.s_addr = inet_addr(buf);
			GetWindowText(hTextBoxPort, wbuf, sizeof(wbuf));
			saddrClient.sin_port = htons(_wtoi(wbuf));
			if (connect(clientSocket, (sockaddr*)&saddrClient, sizeof(saddrClient)) == SOCKET_ERROR) {
				MessageBox(hWindow, L"Ошибка подключения", L"Ошибка", MB_OK);
				return 0;
			}
			MESSAGE message;
			message.messageType = CPU_COUNT;
			message.clientCPUCount = CPUCount;
			send(clientSocket, (char*)&message, sizeof(message), 0);

			for (int i = 0; i < CPUCount; i++) {
				startEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
				ResetEvent(startEvents[i]);
				CreateThread(0, 0, integralThread, (int*)i, 0, 0);
			}
			CreateThread(0, 0, reciveThread, 0, 0, 0);
			EnableWindow(hButtonConnect, 0);
			break;
		
		/*case BUTTON_SET_PERCENT:
			GetWindowText(hTextBoxPercent, wbuf, CHAR_BUFF_SIZE);
			int percent = _wtoi(wbuf);
			if (percent > 0 && percent <= 100) {
				CPULoad = (double)percent / 100;
			}
			break;*/
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
	//Инициализация класса окна
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hIcon = 0;
	wcex.hbrBackground = (HBRUSH)COLOR_BTNFACE;
	wcex.lpszMenuName = 0;
	wcex.hIconSm = 0;
	wcex.lpfnWndProc = windowsProcedure;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = L"client";
	RegisterClassEx(&wcex);

	//Инициализация окна и контролов
	instanse = hInstance;
	hWindow = CreateWindow(L"client", L"Клиент", WS_SYSMENU, 400, 0, 360, 88, NULL, NULL, hInstance, NULL);
	hLabelIP = CreateWindowEx(0, L"Static", L"IP:", WS_VISIBLE | WS_CHILD, 4, 4, 20, 20, hWindow, (HMENU)LABEL_IP, instanse, 0);
	hLabelPort = CreateWindowEx(0, L"Static", L"Порт:", WS_VISIBLE | WS_CHILD, 150, 4, 40, 20, hWindow, (HMENU)LABEL_PORT, instanse, 0);
	hLabelState = CreateWindowEx(0, L"Static", L"Состояние:", WS_VISIBLE | WS_CHILD, 4, 24, 80, 20, hWindow, (HMENU)LABEL_STATE, instanse, 0);
	hLabelCounter = CreateWindowEx(0, L"Static", L"Обработано:", WS_VISIBLE | WS_CHILD, 174, 24, 90, 20, hWindow, (HMENU)LABEL_COUNTER, instanse, 0);
	//hLabelPercent = CreateWindowEx(0, L"Static", L"%", WS_VISIBLE | WS_CHILD, 38, 48, 16, 20, hWindow, (HMENU)LABEL_PERCENT, instanse, 0);
	hTextBoxIP = CreateWindowEx(0, L"Edit", L"127.0.0.1", WS_VISIBLE | WS_CHILD | WS_BORDER, 24, 4, 120, 20, hWindow, (HMENU)TEXTBOX_IP, instanse, 0);
	hTextBoxPort = CreateWindowEx(0, L"Edit", L"5150", WS_VISIBLE | WS_CHILD | WS_BORDER, 188, 4, 46, 20, hWindow, (HMENU)TEXTBOX_PORT, instanse, 0);
	hTextBoxState = CreateWindowEx(0, L"Edit", L"Ожидание", WS_VISIBLE | WS_CHILD | WS_DISABLED, 80, 24, 80, 20, hWindow, (HMENU)TEXTBOX_STATE, instanse, 0);
	hTextBoxCounter = CreateWindowEx(0, L"Edit", L"0", WS_VISIBLE | WS_CHILD | WS_DISABLED, 264, 24, 60, 20, hWindow, (HMENU)TEXTBOX_COUNTER, instanse, 0);
	//hTextBoxPercent = CreateWindowEx(0, L"Edit", L"100", WS_VISIBLE | WS_CHILD | WS_BORDER, 4, 48, 30, 20, hWindow, (HMENU)TEXTBOX_PERCENT, instanse, 0);
	hButtonConnect = CreateWindowEx(0, L"Button", L"Подключиться", WS_VISIBLE | WS_CHILD | WS_BORDER, 238, 4, 104, 20, hWindow, (HMENU)BUTTON_CONNNECT, instanse, 0);
	//hButtonSetPercent = CreateWindowEx(0, L"Button", L"Установить", WS_VISIBLE | WS_CHILD | WS_BORDER, 58, 48, 84, 20, hWindow, (HMENU)BUTTON_SET_PERCENT, instanse, 0);
	ShowWindow(hWindow, cmdShow);
	UpdateWindow(hWindow);

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		MessageBox(hWindow, L"Ошибка загрузки Winsock", L"Ошибка", MB_OK);
		return 0;
	}

	if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == SOCKET_ERROR) {
		MessageBox(hWindow, L"Ошибка создания сокета", L"Ошибка", MB_OK);
		return 0;
	}
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	CPUCount = siSysInfo.dwNumberOfProcessors;
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&csCounter);
	startEvents = new HANDLE[CPUCount];
	endEvent = CreateEvent(NULL, TRUE, FALSE, L"END");

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}