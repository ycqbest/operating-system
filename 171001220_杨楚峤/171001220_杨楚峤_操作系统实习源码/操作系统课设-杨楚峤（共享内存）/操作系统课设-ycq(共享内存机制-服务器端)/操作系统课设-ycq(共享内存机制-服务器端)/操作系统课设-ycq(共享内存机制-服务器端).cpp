// 操作系统课设-ycq(共享内存机制-服务器端).cpp : 定义控制台应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <iostream>
#include <windows.h>
using namespace std;

#define BUF_SIZE 4096//共享信号量

int main()
{
	// 定义共享内存
	char szBuffer[] = "signal"; 


	// 创建共享文件句柄 
	HANDLE hMapFile = CreateFileMapping(		
		INVALID_HANDLE_VALUE,   // 物理文件句柄		
		NULL,   // 默认安全级别		
		PAGE_READWRITE,   // 可读可写		
		0,   // 高位文件大小		
		BUF_SIZE,   // 低位文件大小		
		L"ShareMemorySZHC"   // 共享内存名称	
	);

	// 映射缓存区视图 , 得到指向共享内存的指针

	int *lpBase = (int*)MapViewOfFile(
		hMapFile,            // 共享内存的句柄
		FILE_MAP_ALL_ACCESS, // 可读写许可
		0,
		0,
		BUF_SIZE
	);
	*lpBase = 0;
	cout <<"<YCOS>后台调度服务器exit退出"<<endl ;
	char szInfo[0xFF] = { 0 };

	while (strcmp(szInfo, "exit"))
	{
		gets_s(szInfo);
	}
	// 解除文件映射
	UnmapViewOfFile(lpBase);

	// 关闭内存映射文件对象句柄
	CloseHandle(hMapFile);

	
	
    return 0;
}

