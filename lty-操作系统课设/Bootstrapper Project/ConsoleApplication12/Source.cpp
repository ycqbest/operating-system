#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <windows.h>
using namespace std;

#define BUF_SIZE 4U
char szName[] = "SignalVariable";    // 共享内存的名字

int main()
{
	// 创建共享文件句柄
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // 物理文件句柄
		NULL,                    // 默认安全级别
		PAGE_READWRITE,          // 可读可写
		0,                       // 高位文件大小
		BUF_SIZE,                // 地位文件大小
		(LPCWSTR)szName          // 共享内存名称
		);


	int *pBuf = (int*)MapViewOfFile(
		hMapFile,            // 共享内存的句柄
		FILE_MAP_ALL_ACCESS, // 可读写许可
		0,
		0,
		BUF_SIZE
		);

	printf("-----------------------------------------------\n");
	printf("           __________________________________\n");
	printf("          \/                   \n");
	printf("         \/              Virtual Disk System  \n");
	printf("        \/_______  _____     _____   _____         \n");
	printf("               \/ \/    \/ \/  \/       \/    \/   \n");
	printf("              \/ \/    \/ \/  \/       \/____\/  \n");
	printf(" ____________\/ \/____\/ \/  \/_____  \/    \/￣￣\n");
	printf("              \/                 \/    \/       \n");
	printf("             \/      Ver.     2.1.3     \n");
	printf("            \/      Build    2017.06    \n");
	printf("-----------------------------------------------\n\n");
	printf("-----------------------------------------------\n");
	printf("***           文件系统磁盘调度程序          ***\n");
	printf("-----------------------------------------------\n");
	printf("请不要关闭此程序，当所有终端均已断开连接后\n可以使用 exit 关闭程序\n");
	*pBuf = 0;
	char szInfo[0xFF] = { 0 };

	while (strcmp(szInfo,"exit"))
	{
		gets_s(szInfo); 
	}

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return 0;
}