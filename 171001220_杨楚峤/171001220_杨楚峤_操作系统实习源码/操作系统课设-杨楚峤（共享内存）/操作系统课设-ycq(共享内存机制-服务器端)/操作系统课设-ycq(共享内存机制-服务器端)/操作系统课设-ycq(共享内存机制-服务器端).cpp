// ����ϵͳ����-ycq(�����ڴ����-��������).cpp : �������̨Ӧ�ó������ڵ㡣
//
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <iostream>
#include <windows.h>
using namespace std;

#define BUF_SIZE 4096//�����ź���

int main()
{
	// ���干���ڴ�
	char szBuffer[] = "signal"; 


	// ���������ļ���� 
	HANDLE hMapFile = CreateFileMapping(		
		INVALID_HANDLE_VALUE,   // �����ļ����		
		NULL,   // Ĭ�ϰ�ȫ����		
		PAGE_READWRITE,   // �ɶ���д		
		0,   // ��λ�ļ���С		
		BUF_SIZE,   // ��λ�ļ���С		
		L"ShareMemorySZHC"   // �����ڴ�����	
	);

	// ӳ�仺������ͼ , �õ�ָ�����ڴ��ָ��

	int *lpBase = (int*)MapViewOfFile(
		hMapFile,            // �����ڴ�ľ��
		FILE_MAP_ALL_ACCESS, // �ɶ�д���
		0,
		0,
		BUF_SIZE
	);
	*lpBase = 0;
	cout <<"<YCOS>��̨���ȷ�����exit�˳�"<<endl ;
	char szInfo[0xFF] = { 0 };

	while (strcmp(szInfo, "exit"))
	{
		gets_s(szInfo);
	}
	// ����ļ�ӳ��
	UnmapViewOfFile(lpBase);

	// �ر��ڴ�ӳ���ļ�������
	CloseHandle(hMapFile);

	
	
    return 0;
}

