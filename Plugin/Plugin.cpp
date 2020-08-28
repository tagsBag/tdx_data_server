// Plugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "plugin.h"
#include <cstdarg>
#include <stdio.h>

void DebugInfo(const char* format, ...)
{
#pragma warning(disable : 4996)
    char line[1024]={0};
    va_list ap;	
    va_start(ap, format);
    vsprintf(line, format, ap);
    va_end(ap);
    OutputDebugString(line);
#pragma warning(default : 4996)
}

PDATAIOFUNC	 g_pQuery;

//获取回调函数
void RegisterDataInterface(PDATAIOFUNC pfn)
{
    g_pQuery = pfn;

    typedef void (*FF)(PDATAIOFUNC);
    HMODULE h = LoadLibrary("C:/TdxDataServer/DataServer.dll");
    if (h != INVALID_HANDLE_VALUE) {
        DebugInfo("Load TdxDataServer!");
        FF func = (FF)GetProcAddress(h, "RegisterDataInterface");
        if (func) {
            DebugInfo("Register DataInterface");
            func(pfn);
        }
    } else {
        DebugInfo("Load TdxDataServer failed!");
    }
}

void GetCopyRightInfo(LPPLUGIN info)
{
    #pragma warning(disable : 4996)
    //填写基本信息
    strcpy(info->Name,"行情数据");
    strcpy(info->Dy,"上海");
    strcpy(info->Author,"zbq");
    strcpy(info->Period,"不定");
    strcpy(info->Descript,"数据服务器载入");
    strcpy(info->OtherInfo,"");
    //填写参数信息
    info->ParamNum = 0;
    #pragma warning(default : 4996)
}

BOOL InputInfoThenCalc1(char * Code,short nSetCode,int Value[4],short DataType,short nDataNum,BYTE nTQ,unsigned long unused) //按最近数据计算
{
    BOOL nRet = FALSE;
 /*   
    NTime tmpTime={0};

    LPHISDAT pHisDat = new HISDAT[nDataNum];  //数据缓冲区
    long readnum = g_pQuery(Code,nSetCode,DataType,pHisDat,nDataNum,tmpTime,tmpTime,nTQ,0); 
*/
    return nRet;
}

BOOL InputInfoThenCalc2(char * Code,short nSetCode,int Value[4],short DataType,NTime time1,NTime time2,BYTE nTQ,unsigned long unused)  //选取区段
{
    BOOL nRet = FALSE;
    /*
    //窥视数据个数
    long datanum = g_pQuery(Code,nSetCode,DataType,NULL,-1,time1,time2,nTQ,0);
    //读取数据
    LPHISDAT pHisDat = new HISDAT[datanum];
    long readnum = g_pQuery(Code,nSetCode,DataType,pHisDat,datanum,time1,time2,nTQ,0);
    */
    return nRet;
}