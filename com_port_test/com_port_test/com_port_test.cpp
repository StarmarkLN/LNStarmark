/*-------------------------------------------------------------------------
 * com_port_test.cpp : Демонстрация автоматов (отправка данных в ком-порт)
 *------------------------------------------------------------------------- 
 * Программа демонстрирует работу автомата, выводящего в 
 *  любой связанный ком-порт с портом программы (COM1)
 *	сгенерированные строки (цепочки чисел) 
 *  с заданной частотой
 *  Эти цепочки можно наблюдать в терминальной программе, настроенной 
 *  на порт назначения. 
 *  Скорость работы порта Baudrate = 115200
 *  Остальные параметры стандартные.
 * 
 *  A_Trigger() управляет приостановкой вывода
 * 
 *  A_Trigger_End() останавливает работу программы
 *					по типу останова в контроллерах
 *					переводя ее в вечный пустой цикл
 * 
 *  Автоматы триггеры устроены просто - в данной версии 
 *  на статических счетчиках
 * 
 *  В реальной программе автоматы триггеры можно строить 
 *  на логических таймерах программы
 *  или с использованием системных и харварных таймеров
 * 
 *  Но этот выбор - целиком за разработчиком ПО
 * 
 *  Триггеры показаны в данной версии программы только таймерные,
 *  но возможны триггеры также по любым событиям или прерываниям,
 *  а также - по значениям рабочих переменных программы
 * 
 *  Данная программа служит лишь памяткой для автора и выполнена 
 *  с достаточной для него степенью детализации и отладки.
 * 
 *  Стиль и общая архитектура программы более напоминает просо C
 *  ибо
 *  Основное назначение - демонстрационное.
 * 
 **/

#include "stdafx.h"
#include "Header.h"

HANDLE hSerial;

void Hi()
{
	cout << "Demonstration program for the operation of an automat A_Generator() " << endl
		<< "that depends on the operation of triggers A_Trigger() and A_Trigger_End()" << endl << endl;

	cout << "Author: StarmarkLN " << endl;
	cout << "e-mail: ln.starmark@gmail.com" << endl;
	cout << "e-mail: ln.starmark@ekatra.io" << endl << endl;
}

void Init_SerialPort(LPCTSTR _PortName, DWORD baud)
{
	hSerial = ::CreateFile(_PortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			cout << "Error: serial port does not exist.\n";
		}
		cout << "Error: some other error occurred.\n";
	}


	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		cout << "Error: getting state\n";
	}
	dcbSerialParams.BaudRate = baud;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		cout << "Error: setting serial port state\n";
	}
}

void ReadCOM()
{
      DWORD iSize;
      char sReceivedChar;
      while (true)
      {
            ReadFile(hSerial, &sReceivedChar, 1, &iSize, 0);
			if (iSize > 0)
				cout << sReceivedChar;
      }
}

string DataGenerator(int16_t arg)
{
	int16_t t_arg = arg;
	int16_t res[3];

	string txt = "{X Y Z}[";
	string sarg = "";
	string strval0 = "";
	string strval1 = "";
	string strval2 = "";
	
	res[0] = (int16_t)(round(1000.0 * sin((3 * 3.1415926 * arg) / 180 + 8.5)));
	res[1] = (int16_t)(round(310.0 * sin((7 * 3.1415926 * arg) / 180 + 18.5)));
	res[2] = (int16_t)(round(56.0 * sin((11 * 3.1415926 * arg) / 180 + 12.5)));

	sarg = to_string(arg);
	strval0 = to_string(res[0]);
	strval1 = to_string(res[1]);
	strval2 = to_string(res[2]);

	txt += sarg;
	txt += "] -> ";
	txt += strval0;
	txt += " ";
	txt += strval1;
	txt += " ";
	txt += strval2;
	txt += "\r\n";

	return txt;
}

/// <summary>
/// Автомат основной и 2 триггера, работающие автономно
/// в общем цикле программы
/// </summary>
/// <param name="waite"></param>
/// <param name="run"></param>
/// <param name="end"></param>
/// <returns></returns>
int A_Generator(uint64_t waite, bool run, bool end)
{
	static int next = 0;

	static uint64_t testcnt = 0;
	static int16_t arg = 0;

	switch (next)
	{
	case 0:
		Init_SerialPort(MY_COMM_PORT, MY_BAUDRATE);

		next = 1;

		break;

	case 1:
		testcnt = 0;
		arg = 0;

		if(run && !end)
		{
			next = 2;
		}
		else
        if(end)
		{
			BOOL iRet = FlushFileBuffers(hSerial);

			return 1000;
		}
		
		break;

	case 2:

		if(run && !end)
		{
			testcnt++;
			if (testcnt >= waite)
			{
				string data = DataGenerator(arg++);

				DWORD dwSize = data.length();
				DWORD dwBytesWritten;
				LPOVERLAPPED ov;

				char* dt = (char*)&data[0];

				BOOL iRet = WriteFile(hSerial, dt, dwSize, &dwBytesWritten, NULL);

				cout << dwSize << " Bytes generated " << dwBytesWritten << " Bytes sended" << endl;

				testcnt = 0;
			}
		}
		else
		{

			cout  << "Pause work program by A_Trigger()" << endl << endl;

			next = 1;
		}

		break;

	}
}

void A_Trigger(bool* trig, int64_t maxcnt)
{
	static int64_t cnt = 0;

	cnt++;
	if (cnt >= maxcnt)
	{
		*trig = !(*trig);
		cnt = 0;
	}
}

void A_Trigger_End(bool* trig, int64_t maxcnt)
{
	static int64_t cnt = 0;
	static int next = 0;

	switch (next)
	{
	case 0:
		cnt++;
		if (cnt >= maxcnt)
		{
			*trig = true;
			cnt = 0;

			next = 1;
		}
		break;

	case 1:
		cout << endl << "End work program by A_Trigger_End()" << endl;

		next = 2;
		break;
    
	case 2:
		break;
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	bool start = true;
	bool end = false;
	uint64_t val_waite = MAXTESTCNT;

	int res = -1;

	Hi();

	while(1)
	{

		/// <summary>
		/// Работает генератор строки с данными {X Y Z}
		/// с частотой, определенной через задержку val_waite
		/// Переключение триггера (bool start) переводит
		/// автомат A_Generanjh d другой режим - в данном случае
		/// в режим остановки
		/// Возможен более сложный триггер - многопозиционный
		/// </summary>
		/// <param name="argc"></param>
		/// <param name="argv"></param>
		/// <returns></returns>
		res = A_Generator(val_waite, start, end);

		/// <summary>
		/// Собственно - триггер.
		/// Работает по внутреннему счетчику
		/// и по срабатыванию - переключает переменную start в противоположное
		/// состояние (Имитация tuggle())
		/// Возможно построение других триггеров
		/// </summary>
		/// <param name="argc"></param>
		/// <param name="argv"></param>
		/// <returns></returns>
		A_Trigger(&start, MAXTRIGGERCNT);

		/// <summary>
		/// Должен же быть когда-то конец
		/// </summary>
		/// <param name="argc"></param>
		/// <param name="argv"></param>
		/// <returns></returns>
		A_Trigger_End(&end, MAXTRIGGEREND);

		//ReadCOM();

		if (res == 1000)
			return 0;
	}

	return 0;
}

