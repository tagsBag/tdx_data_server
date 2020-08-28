// Tdx_mock.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include "../Plugin/Plugin.h"

#include <cstdarg>
#include <stdio.h>

void DebugInfo(const char* format, ...)
{
#pragma warning(disable : 4996)
	char line[1024] = { 0 };
	va_list ap;
	va_start(ap, format);
	vsprintf(line, format, ap);
	va_end(ap);
	OutputDebugString(line);
	std::cout << line << std::endl;
#pragma warning(default : 4996)
}

PDATAIOFUNC	 g_pQuery;

int CALLBACK write_log(char * log)
{
	std::cout << log <<std::endl;
	return 1;
}

//获取回调函数
void RegisterDataInterface(PDATAIOFUNC pfn)
{
	g_pQuery = pfn;

	typedef void(*FF)(PDATAIOFUNC);
	HMODULE h = LoadLibrary("DataServer.dll");
	if (h != INVALID_HANDLE_VALUE) {
		DebugInfo("Loaded Plugin!");
		if (true)
		{
			typedef int(CALLBACK*Func_write_log)(char *);	///mid 回调函数定义
			typedef void(*FF)(Func_write_log);				///mid 注册函数定义
			FF RegisterWriteLog = (FF)GetProcAddress(h, "RegisterWriteLog");
			if (RegisterWriteLog)
			{
				DebugInfo("Register WriteLog");
				RegisterWriteLog(write_log);
			}
		}

		if (true)
		{
			FF func = (FF)GetProcAddress(h, "RegisterDataInterface");
			if (func) 
			{
				DebugInfo("Register DataInterface");
				func(pfn);
			}
		}
	}
	else {
		DebugInfo("Load TdxDataServer failed!");
	}
	return;
	typedef void(*FF)(PDATAIOFUNC);
	//HMODULE h = LoadLibrary("Plugin.dll");
	std::cout << "01\n";




	if (h != INVALID_HANDLE_VALUE) {
		DebugInfo("Loaded Plugin!");
		std::cout << "02\n";
		if (true)
		{
			FF func = (FF)GetProcAddress(h, "RegisterDataInterface");
			if (func) {
				std::cout << "03\n";

				DebugInfo("Register DataInterface");
				func(pfn);

				std::cout << "05\n";

			}
		}		
		
		if (true)
		{
			typedef void(*Func)(int &a);
			Func fAdd = (Func)GetProcAddress(h, "RegisterDataInterface01");
			if (fAdd)
			{
				DebugInfo("Register DataInterface");
				int i = 1;
				fAdd(i);
				std::cout << "04," << i << std::endl;
			}
		}
		if (true)
		{
			typedef int(*Func)(int a, int b);
			Func fAdd = (Func)GetProcAddress(h,"fun");
			if (fAdd)
			{
				DebugInfo("Register DataInterface");
				int b = fAdd(3, 1);
				std::cout << "05," << b <<std::endl;
			}

		}


	}
	else {
		DebugInfo("Load TdxDataServer failed!");
	}
}

long CALLBACK query(char * Code, short nSetCode, short DataType, void * pData, short nDataNum, NTime, NTime, BYTE nTQ, unsigned long)
{
	DebugInfo("------%s",Code);
	return 1;
}

int main()
{
	RegisterDataInterface(query);
	while (true)
	{
		//std::cout << "Hello World!\n"; 
		Sleep(5000);
	}
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
