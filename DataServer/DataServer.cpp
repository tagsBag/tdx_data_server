// DataServer.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "../Plugin/Plugin.h"
#include <cstdarg>
#include <stdio.h>
#include <WinSock2.h>
#include <fstream>
#include <atlstr.h>
#include <atlcoll.h>
#include <sstream>
#pragma comment(lib, "ws2_32")

using namespace std;

typedef int(CALLBACK*Func_write_log)(char *);
Func_write_log	 g_p_write_log = NULL;

void DebugInfo(const char* format, ...)
{
#pragma warning(disable : 4996)
    char line[1024]={0};
    va_list ap;	
    va_start(ap, format);
    vsprintf(line, format, ap);
    va_end(ap);
    OutputDebugString(line);
	if (g_p_write_log != NULL)
	{
		g_p_write_log(line);
	}
#pragma warning(default : 4996)
}

#define FDS_SIZE (10)
typedef int FDS[FDS_SIZE];
void fds_copy(FD_SET *set, const FDS fds) {
    FD_ZERO(set);
    for (int i = 0; i < FDS_SIZE; i++) {
        if (fds[i] != INVALID_SOCKET)
            FD_SET(fds[i], set);
    }
}

void fds_clear(FDS fds) {
    for (int i = 0; i < FDS_SIZE; i++) {
        fds[i] = INVALID_SOCKET;
    }
}

int fds_add(FDS fds, int fd) {
    for (int i = 0; i < FDS_SIZE; i++) 
	{
        if (fds[i] == INVALID_SOCKET) 
		{
            fds[i] = fd;
            return i;
        }
    }
    return -1;
}

void fds_del(FDS fds, int fd) {
    for (int i = 0; i < FDS_SIZE; i++) {
        if (fds[i] == fd) {
            fds[i] = INVALID_SOCKET;
            return;
        }
    }
}

struct Buff {
    char *beg;
    char *r;
    char *w;
    char *end;
};

void buff_clear(struct Buff *buff) {
    buff->r = buff->w = buff->beg;
    *(buff->beg) = '\0';
}

bool buff_alloc(struct Buff *buff, int cap) {
    memset(buff, 0, sizeof(struct Buff));
    if (buff->beg = (char *)malloc(cap)) {
        buff->end = buff->beg+cap-1;
        *(buff->end) = '\0';
        buff_clear(buff);
        return true;
    }
    return false;
}

void buff_free(struct Buff *buff) {
    if (buff->beg) {
        free(buff->beg);
        memset(buff, 0, sizeof(struct Buff));
    }
}

char* buff_r(struct Buff *buff) {
    return buff->r;
}

char* buff_w(struct Buff *buff) {
    return buff->w;
}

void buff_inc_r(struct Buff *buff, int count) {
    buff->r += count;
}

void buff_inc_w(struct Buff *buff, int count) {
    buff->w += count;
    *(buff->w) = '\0';
}

int buff_can_r(struct Buff *buff) {
    return buff->w - buff->r;
}

int buff_can_w(struct Buff *buff) {
    return buff->end - buff->w;
}


const BYTE INVALID_DATA[]={0xF8,0xF8,0xF8,0xF8};
const NTime INVALID_TIME={0};

typedef CAtlMap<CString, CString> StringMap;
typedef CAtlArray<CString> StringArray;
void parse_request(const CString req, StringMap& ret) {
    int pos = 0;
    ret.RemoveAll();
    while (pos != -1) {
        CString tok = req.Tokenize("\r\n", pos);
        if (!tok.IsEmpty()) {
            int colon = tok.Find(':');
            if (colon != -1) {
                CString name = tok.Mid(0, colon).Trim();
                CString value = tok.Mid(colon+1).Trim();
                if (!name.IsEmpty() && !value.IsEmpty()) {
                    ret[name] = value;
                }
            }
        }
    }
}

void print_map(const StringMap& m) {
    POSITION pos = m.GetStartPosition();
    while (pos != NULL) {
        CString k, v;
        m.GetNextAssoc(pos, k, v);
        DebugInfo("%s -> %s\n", k.GetString(), v.GetString());
    }
}

void print_time(CString what, NTime tm) {
    DebugInfo("%s %d-%d-%d %d:%d:%d\n", what.GetString(), tm.year, tm.month, tm.day, 
        tm.hour, tm.minute, tm.second);
}

void split(StringArray& strs, CString str, char by) {
    strs.RemoveAll();
    while (!str.IsEmpty()) {
        int i = str.Find(by);
        if (i == -1) {
            strs.Add(str);
            break;
        }
        if (i != 0)
            strs.Add(str.Mid(0, i));
        str = str.Mid(i+1);
    }
}

NTime parse_time(CString str) {
    NTime tm = INVALID_TIME;
    StringArray strs;
    split(strs, str, ' ');
    if (strs.GetCount() != 2)
        return tm;
    CString date = strs[0];
    CString time = strs[1];
    split(strs, date, '-');
    if (strs.GetCount() != 3)
        return tm;
    tm.year = atoi(strs[0].GetString());
    tm.month = atoi(strs[1].GetString());
    tm.day = atoi(strs[2].GetString());
    split(strs, time, ':');
    if (strs.GetCount() != 3)
        return INVALID_TIME;
    tm.hour = atoi(strs[0].GetString());
    tm.minute = atoi(strs[1].GetString());
    tm.second = atoi(strs[2].GetString());
    return tm;
}

PDATAIOFUNC	 g_pQuery = NULL;

const char* CODE = "Code";
const char* MARKET = "Market";
const char* TYPE = "Type";
const char* FROM = "From"; // Time From
const char* TO = "To";

void response(const StringMap& req, struct Buff *buff) {
    buff_clear(buff);
    if (req.Lookup(CODE) == NULL ||
        req.Lookup(MARKET) == NULL ||
        req.Lookup(TYPE) == NULL) {
        DebugInfo("invalid request\n");
        return;
    }
    const StringMap::CPair *p = NULL;
    p = req.Lookup(CODE);
    CString code = p->m_value;
    p = req.Lookup(MARKET);
    short market = atoi(p->m_value.GetString());
    p = req.Lookup(TYPE);
    short type = atoi(p->m_value.GetString());
    DebugInfo("query %s, %s, %d\n", code.GetString(), market==0 ? "Shenzhen" : "Shanghai", type);

    NTime from = INVALID_TIME, to = INVALID_TIME;
    p = req.Lookup(FROM);
    if (p) {
        from = parse_time(p->m_value);
    }
    print_time(FROM, from);
    p = req.Lookup(TO);
    if (p) {
        to = parse_time(p->m_value);
    }
    print_time(TO, to);

    long data_num = g_pQuery(
        (char*)code.GetString(),
        market,
        type,
        NULL,
        ASK_ALL,
        from,
        to,
        1,
		0);

	if (true)
	{
		LPHISDAT pHisDat = new HISDAT[data_num];  //数据缓冲区
		g_pQuery(
			(char*)code.GetString(),
			market,
			type,
			pHisDat,
			ASK_ALL,
			from,
			to,
			1,
			0);

		ostringstream oss;
		for (int i = 0; i < data_num; i++)
		{
			char s[128];
			sprintf(s, "%4d-%02d-%02d %02d:%02d:%02d", pHisDat[i].Time.year, pHisDat[i].Time.month, pHisDat[i].Time.day, pHisDat[i].Time.hour, pHisDat[i].Time.minute, pHisDat[i].Time.second);
			sprintf(s, "%d-%d-%d %d:%d:%d", pHisDat[i].Time.year, pHisDat[i].Time.month, pHisDat[i].Time.day, pHisDat[i].Time.hour, pHisDat[i].Time.minute, pHisDat[i].Time.second);
			DebugInfo("%d-%d-%d %d:%d:%d", pHisDat[i].Time.year, pHisDat[i].Time.month, pHisDat[i].Time.day, pHisDat[i].Time.hour, pHisDat[i].Time.minute, pHisDat[i].Time.second);
			oss<<s<<"\t";
			oss<<pHisDat[i].Open << "\t";
			oss << pHisDat[i].High << "\t";
			oss << pHisDat[i].Low << "\t";
			oss << pHisDat[i].Close << "\t";
			oss << pHisDat[i].fVolume << "\t";
			oss << "\r\n";
		}
		//int x = sprintf(buff_w(buff), "data length: %d", data_num);
		int x = sprintf(buff_w(buff), oss.str().c_str());

		buff_inc_w(buff, x);
	}


	if (false)
	{	///mid 经测试第二版数据查询会导致tdx奔溃
		LPREPORTDAT2 pHisData2 = new REPORTDAT2[data_num];

		g_pQuery(
			(char*)code.GetString(),
			market,
			type,
			pHisData2,
			ASK_ALL,
			from,
			to,
			1,
			0);

		ostringstream oss;
		for (int i = 0; i < data_num; i++)
		{
			oss << pHisData2[i].Open << "\t";
			oss << pHisData2[i].Close << "\t";
			oss << "\r\n";
		}
		//int x = sprintf(buff_w(buff), "data length: %d", data_num);
		int x = sprintf(buff_w(buff), oss.str().c_str());

		buff_inc_w(buff, x);




	}
}

int start_server(unsigned short *pport) {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        DebugInfo("create socket failed\n");
		DebugInfo("error no: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    struct sockaddr_in addr = {0};
    int len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;
    if (bind(fd, (sockaddr *)&addr, len)) {
        DebugInfo("bind socket failed\n");
        closesocket(fd);
        return INVALID_SOCKET;
    }

    if (getsockname(fd, (sockaddr *)&addr, &len)) {
        DebugInfo("getsockname failed\n");
        closesocket(fd);
        return INVALID_SOCKET;
    }
    if (listen(fd, 10)) {
        DebugInfo("listen failed\n");
        closesocket(fd);
        return INVALID_SOCKET;
    }

    *pport = ntohs(addr.sin_port);
    return fd;
}

void save_server_port(unsigned short port) {
    ofstream ofs("C:/TdxDataServer/server-port.txt");
    ofs << port << endl;
    DebugInfo("server at: %d\n", port);
}

// accept -> put into read q -> clear rbuff -> read all -> shutdown receive ->
// remove from read q -> put into write q -> clear wbuff -> prepare response -> 
// write all -> close socket -> remove from write q
void server_loop(int svr_fd) {
    bool dosth = false;
    FDS rfds, wfds;				///mid 自定义整数数组
    fds_clear(rfds);
    fds_clear(wfds);
    int rfds_cap = FDS_SIZE;
    FD_SET rfdset, wfdset;
    struct Buff rbuffs[FDS_SIZE], wbuffs[FDS_SIZE];
    for (int i = 0; i < FDS_SIZE; i++) 
	{
        buff_alloc(rbuffs+i, 512);
        buff_alloc(wbuffs+i, 4096);
    }
    while (true) 
	{
        dosth = false;
        fds_copy(&rfdset, rfds);		///mid 将 rfds 中信号 数据拷贝 到 rfdset,rfdset按位标记,rfds按整数值标记
        fds_copy(&wfdset, wfds);
        FD_SET(svr_fd, &rfdset);

		DebugInfo("selecting");

        if (select(0, &rfdset, &wfdset, NULL, NULL) > 0) 
		{
			///mid 01接受连接
			DebugInfo("selected");

			if (FD_ISSET(svr_fd, &rfdset))
			{
				if(rfds_cap > 0) 
				{
					struct sockaddr_in addr = {0};
					int len = sizeof(addr);
					int fd = accept(svr_fd, (sockaddr *)&addr, &len);
					if (fd != INVALID_SOCKET) {
						DebugInfo("accept connection from: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
						int idx = fds_add(rfds, fd);
						rfds_cap--;
						buff_clear(rbuffs+idx);
						dosth = true;
					}
				}
				else
				{
					DebugInfo("rfds_cap < 1");
				}
			}
			else
			{
				DebugInfo("svr_fd not set");
			}

			///mid 02接收数据
            for (int i= 0; i < FDS_SIZE; i++) 
			{
				int fd = rfds[i];
				if (fd != INVALID_SOCKET && FD_ISSET(fd, &rfdset)) 
				{
					char count[512]{0};
					int len = recv(fd, count, 512, 0);
					DebugInfo("header received: %d bytes\n", len);
					DebugInfo(count);
					int to_receive = atoi(count);
					DebugInfo("to receive: %d bytes\n", to_receive);

					DebugInfo("begine to receive body\n");
					int received = 0;
					while (received < to_receive)
					{	///mid 接收数据
						DebugInfo("receiving,received:%d,to_received:%d\n",received,to_receive);
						len = recv(fd, buff_w(rbuffs+i), buff_can_w(rbuffs+i), 0);
						DebugInfo("received: %d bytes\n", len);
						if (len > 0) 
						{	///mid 标记结束位
							buff_inc_w(rbuffs+i, len);
							received += len;
						}
					}

					DebugInfo("body received,total:%d\n",received);

                    if (buff_can_w(rbuffs+i) >= 0) 
					{	///mid 数据接收完毕
                        if (to_receive < 0)
						{
                            DebugInfo("recv error: %d\n", WSAGetLastError());
							break;
                        }
                        if (buff_can_w(rbuffs+i) == 0) 
						{
                            DebugInfo("recv buffer is full\n");
                        }
                        shutdown(fd, SD_RECEIVE);
                        fds_del(rfds, fd);

                        int idx = fds_add(wfds, fd);
                        StringMap fields;
                        parse_request(buff_r(rbuffs+i), fields);		///mid 解析命令
                        response(fields, wbuffs+idx);					///mid 向tdx请求数据
						DebugInfo("response at: %d is %d\n", i, fd);
                    }
					else
					{
						DebugInfo("buff_can_w(rbuffs+i) < 0");
					}
                    dosth = true;
                }
            }

			///mid 03发送数据
            for (int i= 0; i < FDS_SIZE; i++) 
			{
                int fd = wfds[i];
                if (fd != INVALID_SOCKET && FD_ISSET(fd, &wfdset)) 
				{
					DebugInfo("开始发送数据");
                    int len = send(fd, buff_r(wbuffs+i), buff_can_r(wbuffs+i), 0);
                    if (len >= 0) 
					{
                        buff_inc_r(wbuffs+i, len);
                    } else 
					{
                        DebugInfo("send error: %d\n", WSAGetLastError());
                        buff_clear(wbuffs+i);
                    }

                    if (buff_can_r(wbuffs+i) == 0) 
					{
                        DebugInfo("send done\n");
                        closesocket(fd);
                        fds_del(wfds, fd);
                        rfds_cap++;
                    }
                    dosth = true;
                }
            }
        }
        if (!dosth) {
            DebugInfo("something wrong...\n");
            Sleep(1000);
        }
    }
}

DWORD WINAPI ThreadFuncFirst(LPVOID param)
{
	if (g_p_write_log != NULL)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}

    unsigned short port = 0;
    int svr_fd = start_server(&port);
    if (svr_fd != INVALID_SOCKET) {
        save_server_port(port);
        server_loop(svr_fd);
        closesocket(svr_fd);
    }
	if (g_p_write_log != NULL)
	{
		WSACleanup();
	}
    return 0;
}
/*
//TODO: response
NTime tmpTime={0};
short nDataNum = 5;
LPHISDAT pHisDat = new HISDAT[nDataNum];  //数据缓冲区
long readnum = g_pQuery(buff_r(rbuffs+i),1,PER_DAY,pHisDat,nDataNum,tmpTime,tmpTime,1,0);
for(int i=0;i < readnum;i++)
{
    int x = sprintf(buff_w(wbuffs+idx), "close: %f\n", pHisDat[i].Close);
    buff_inc_w(wbuffs+idx, x);
}
delete []pHisDat;pHisDat=NULL;
*/

extern "C"  __declspec(dllexport)
void RegisterWriteLog(Func_write_log pf_write_log)
{	///mid 需要先于 RegisterDataInterface 调用,以能 write_log
	g_p_write_log = pf_write_log;
	pf_write_log("write_log 注册成功");
}

extern "C"  __declspec(dllexport)
void RegisterDataInterface(PDATAIOFUNC pf_query) 
{
	DebugInfo("tdx data server registered!");

    if (g_pQuery == NULL) 
	{
        g_pQuery = pf_query;
        DWORD dwThreadID = 0;
        HANDLE handleFirst = CreateThread(NULL, 0, ThreadFuncFirst, 0, 0, &dwThreadID);
    }
	DebugInfo("pf_query 注册成功");

}

