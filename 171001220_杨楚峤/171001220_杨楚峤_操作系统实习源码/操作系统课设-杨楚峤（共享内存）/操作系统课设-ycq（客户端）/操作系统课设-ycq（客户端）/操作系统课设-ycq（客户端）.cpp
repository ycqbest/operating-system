// ����ϵͳ����-����.cpp : �������̨Ӧ�ó������ڵ㡣
//���������Linux����ϵͳ����ļ�ϵͳ������ʽ
//ģ��windowsΪ�ļ���Ȩ��
//����ΪC/C++����
//
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include<iostream>
#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include<string>
#include<stack>
#include<string.h>
#include<time.h>
#include<fstream>
#include<iomanip>

#include"thread"
#include"mutex"

using  namespace std;
#pragma comment(linker,"/STACK:102400000,1024000")
#define N 100 //ͨ�������С

//��λȫ��Ϊ�ֽ�B��
//���̿ռ��ܼ�1024*2048
//ÿ8KB����һ��inode��ÿ��i�ڵ�Ĵ�С��128��
//i�ڵ�ռ��ܼ�128*512��ռ����3.1%����ռ��64�̿�
//�̿�Ų��ùܣ�ֻ��Ҫ��סi�ڵ�Ĵ���˳�򼴿�
//��д�ź������
#define SIG_SIZE 4096
#define disksize 2400 * 512//���̴�С
#define blocksize 2400//�̿��С
#define blockcount 512 //�̿����
#define inodesize 600//i�ڵ��С
#define inodecount 32//i�ڵ����(1-4,8-32)

//�����飬�ṹ���СΪ1024��ռ��1�̿飨��ӳ���̿ռ����������
typedef struct {
	//���̴�С
	//�̿��С
	//�̿����
	//i�ڵ��С
	//i�ڵ����
	int useddisk;//���ô��̿ռ�
	int remaindisk;//ʣ����̿ռ�
	int usedblock;//�����̿�����
	int remainblock;//ʣ���̿�����
	int usedinode;//����i�ڵ�����
	int remaininode;//ʣ��i�ڵ�����

	int block_inodemap[blockcount + inodecount];//���λʾͼ
												//int inodemap[inodecount];//��λʾͼ
}SuperBlock;

//δ����Ŀ¼�i�ڵ㺬ȫ����Ϣ
//i�ڵ㣨�ļ���Ϣ��ϸ��¼��
typedef struct {
	int x;//�ļ��ڲ���ʶ��
	string name;//�ļ���
	int size;//�ļ���С
	string user_name;//�ļ�������
	int type;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
	int user_quanxian;//�����û�Ȩ�� (r=2,w=1)
	int size_num;//ռ���̿����� (0Ϊȫ������)
	int address[N];//�����ȡ��ŵ�ַ���ɲ������洢��
	int father;//��Ŀ¼�ڲ���ʶ��

	time_t atime;//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
	time_t mtime;//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
	time_t ctime;//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime
}INode;

//�û���Ϊ������ã�����ռ�洢�ռ��С����������ڴ棩
typedef struct {
	string name; //�û���
	int password; //����(��ϣ����)
	int UID;//�û���ţ�������ͬ��ŵľ�����ͬȨ�ޣ�
			//����ʼ����1Ϊroot��2Ϊgroup1��0Ϊ�����û���
}User;

string disk_name = "disk.dat";//��������ļ�����
int curdir;//��ǰĿ¼
int path[0xFF];//·����¼
char curpath[0xFF];//��ǰ��ַ�ַ���
string key;
char cmd[0xFF];//����
char cmd1[0xF];//����1
char cmd2[0xFF];//����2
char cmd3[0xFF];//����3
string cmd11;
string cmd111;
int cmd22;


//mutex mu;//��
SuperBlock superblock;//�����飨1��block��
INode inode[inodecount];//�ڵ����飨64��block��
User user;//��ǰ���û�

void head();//ϵͳͷ��
void cmdhead();//����ͷ��
void msg(const char* str);//���ͳһ��ʽ
unsigned int BKDRHash(char* key, unsigned int len);//��ϣ��������
void star(char *key);//�����*����
int login();//�û���¼
void useradd();//����û�
int judge();//�ж�yesno
int addfloat(int x, int y);//С����λ
void WriteSuperBlock();//������д���ļ�
void ReadSuperBlock();//�ļ���ȡ������
void Writedata(int Index, char *content);//���ݿ�д���ļ�
void Readdata(int Index, char *content);//�ļ���ȡ���ݿ�
void showsb();//���������Ϣ
void init_sb();//��ʼ��������
void WriteINode(int Index, INode &inode);//i�ڵ�д���ļ�
void ReadINode(int Index, INode &inode);//�ļ���ȡi�ڵ�
void init_id();//��ʼ��i�ڵ�
void init_root();//��ʼ����Ŀ¼
void diskloading();//�������
void diskupdating();//���´���
void format();//��ʽ������
void Backup();//���ݴ����ļ�
void recovery();//�ָ������ļ���Ϣ
void help();//ϵͳ֧������ܽ���
void pwd();//��ʾ��ǰ·��(����·��)
int bianli(string name, int curdir1);//Ѱ�ҷ���������i�ڵ�
int UID(int curdir1);//�ж���Ⱥ�û�(������Ⱥ��)
void cd(char *curpath1);//�л���ǰ·��(֧�־���·�������·��)
int chmod(string config, int quanxian);//���ĵ�ǰĿ¼���ļ�Ȩ��
int get_inode();//����i�ڵ�
void free_inode(int i);//�ͷ�i�ڵ�
int get_block();//�����̿�
void free_block(int i);// �ͷ��̿�
int mkdir(string name);//������ǰĿ¼����Ŀ¼
void createfile(string config);//�������ļ�
void time();//��ʾ��ǰʱ��
void _time(time_t &m_time);//ʱ��ת��
void dir();//��ʾ��ǰĿ¼�µ���Ϣ���ļ�
void exit();//�˳�ϵͳ����
void more(string name);//��ʾ��ǰ·�����ļ�����
int rm(string name, int mulu);//ɾ����ǰĿ¼��ĳ���ļ�
int rmdir(string name, int mulu);//ɾ����ǰ·����ĳ��Ŀ¼
void copy(string name, char *curpath1);//���Ƶ�ǰ�ļ�����һ·��
void rename(string name, string name1);//��ǰ·�����ļ�����Ŀ¼������
void xcopy(string name, char *curpath1);//����Ŀ¼����һ��·������һ��·��(��ɾ��Ŀ¼���ƣ���Ҫ�ݹ�ɾ��)
void import(char *curpath1, int mulu);//�ӱ��ص����ļ�
void find(char *curpath1);//Ѱ���ļ��е�һ���ַ�
void  attrib(string name);//��ʾ��ǰĿ¼���ļ�Ȩ��
void input();//���뺯��		 
void charstring(char *config);//charתstring����
void charstring1(char *config);//charתstring����
void intstring(char *config);//charתint����
void ver();//�汾��Ϣ
void choice();//����ѡ��
void flush();//�������
int BF(char S[], char T[]);//BF�ַ���ƥ��
void pexport(char *curpath1, char *curpath2);

//�ļ�ӳ����һ��ʵ�ֽ��̼䵥���˫��ͨ�ŵĻ��ơ�
//�����������������ؽ��̼��໥ͨ�š�Ϊ�˹����ļ����ڴ棬
//���еĽ��̱���ʹ����ͬ���ļ�ӳ������ֻ��Ǿ����
char szBuffer[] = "signal";
HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIG_SIZE, (LPCWSTR)szBuffer);
int *signal = (int *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIG_SIZE);

//������
int main()
{
	head();//ϵͳͷ��
	login();//��¼
	while (1)
	{
		input();
		choice();
		flush();//�����
	}  
	system("pause");
	return 0;
}

//�и��Ĵ��̵Ĳ���һ��Ҫ��������һ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//�и��Ĵ��̵Ĳ���һ��Ҫ��������һ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//*******************************************************
//ϵͳͷ����ϵͳ��飩
void head() {
	cout << "--------------------------------------------------" << endl;
	cout << "             Virtual  File  System" << endl;
	cout << "--------------------------------------------------" << endl;
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);//������ɫ
	cout << " *       *       *****       *****        *******" << endl;
	cout << "   *   *       *           *       *     *" << endl;
	cout << "     *        *           *         *     *******" << endl;
	cout << "     *         *           *       *             *" << endl;
	cout << "     *           *****       *****        ******* " << endl;
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);//���ð�ɫ
	cout << "                                        Ver.1.1.0" << endl;
	cout << "                                  Directed by M.W." << endl;
	cout << "--------------------------------------------------" << endl;
	cout << "--------------------------------------------------" << endl;
}

//����ͷ��
void cmdhead() {
	//cout << "test"<< endl;
	cout << endl << "[" << user.name << "@localhost" << curpath << "]#";
	//printf("\n[%s@localhost %s]# ",user.name, inode[curdir].name);
}
//���ͳһ��ʽ

void msg(const char* str)
{
	int i, M;
	M = strlen(str);
	for (i = 0; i<50; i++)   printf("-");
	printf("\n----");
	for (i = 0; i<(50 - 8 - M) / 2; i++)   printf(" ");
	printf("%s", str);
	for (i = 0; i<(50 - 8 - M) / 2; i++)   printf(" ");
	printf("----\n");
	for (i = 0; i<50; i++)   printf("-");
	printf("\n");
}

//��ϣ��������
unsigned int BKDRHash(char* key, unsigned int len)
{
	unsigned int s = 131;
	unsigned int h = 0;
	unsigned int i = 0;
	for (i = 0; i < len; key++, i++)
	{
		h = (h * s) + (*key);
	}
	return h;
}

//�����*����
void star(char *key)
{
	char *p;
	p = key;
	while (*p = _getch())
	{
		if (*p == 0x0d) //�س�
		{
			*p = '\0'; //������Ļس���ת���ɿո�
			break;
		}
		else if (*p == 8)//�˸�
		{
			cout << char(8) << " " << char(8);//�˸�ɾ��
			p--;
		}
		else {
			printf("*");   //�������������"*"����ʾ
			p++;
		}
	}

}

//�û���¼
int login() {
	string user_name;
	int user_password;//(���ܺ�)
	char key[100];
	int count = 0;
	int flag = 0;

	do {
		fstream myfile("user.txt", ios::in | ios::out);//���ļ�
		if (!myfile.is_open())
		{
			printf("\nCan't open file %s.\n", "user.txt");
			printf("This filesystem not exist!\n");
			system("pause");
			exit(0);
		}

		msg("�������½��Ϣ");
		printf("Username:");
		cin >> user_name;
		printf("Password:");
		star(key);
		//��¼key����
		int i = 0;
		while (key[i] != '\0')
		{
			i++;
		}
		user_password = BKDRHash(key, i);
		while (!myfile.eof())//û���ļ�βѭ��
		{
			myfile >> user.name;
			myfile >> user.password;
			myfile >> user.UID;
			//cout << user_name;
			//cout << user_password;
			//cout << user.name;
			//cout << user.password;
			//�û��������붼��ȷ
			if (user.name == user_name && user.password == user_password)
			{
				myfile.close();
				printf("\n");
				cout << "'" << user.name << "'";
				printf(" login succeed at ");
				time();
				//cout << "--------------------------------------------------" << endl;
				flag = 1;
				break;
			}
		}
		if (flag == 0)//�����û�������ȷ
		{
			printf("\nThis user or password is incorrect!\n");
			//cout << "--------------------------------------------------" << endl;
			myfile.close();
		}
		else if (flag == 1)
		{
			break;
		}

	} while (1);
	diskloading();//���������Ϣ
	cin.ignore();
	return 0;
}

//����û�
void useradd()
{
	//����û�Ȩ��
	if (user.UID != 1)
	{
		printf("Ȩ�޲���....");
		return;
	}
	User user1;//���ӵ��û�
	fstream myfile("user.txt", ios::in | ios::out | ios::app);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	while (1)
	{
		msg("���������û���Ϣ");
		string user_name;
		string user_UID;
		char key1[100];
		char key2[100];
		int flag = 0;
		printf("Username:");
		cin >> user_name;

		while (!myfile.eof())//û���ļ�βѭ��
		{
			myfile >> user1.name;
			myfile >> user1.password;
			myfile >> user1.UID;
			myfile.close();
			if (user1.name == user_name)
			{
				cout << "���û��Ѵ���" << endl;
				flag = 1;
				break;
			}
		}
		if (flag == 0)//�����½��û�
		{
			while (true) {
				printf("password:");
				star(key1);
				printf("\n");
				printf("password again:");
				star(key2);
				if (!strcmp(key1, key2)) break;
				printf("\n�������벻һ�£�����������\n");
			}
			printf("\n��ͨ�û�0������Ա1����Ⱥ��������Ⱥ��......\n");
			printf("UID:");
			cin >> user_UID;

			//��¼key����
			int i = 0;
			while (key1[i] != '\0')
			{
				i++;
			}
			int user_password = BKDRHash(key1, i);
			//cout << user_name;
			//cout << user_password;
			fstream myfile("user.txt", ios::in | ios::out | ios::app);//���ļ�
			if (!myfile.is_open())
			{
				printf("\nCan't open file %s.\n", "user.txt");
				printf("This filesystem not exist!\n");
				system("pause");
				exit(0);
			}
			myfile << user_name << " " << user_password << " " << user_UID << "\n";
			myfile.close();
			cout << "Succeed......" << endl;
			break;
		}
	}
}

//�ж�yesno
int judge()
{
	char c[10];
	scanf("%s", c);
	getchar();
	if (!strcmp(c, "Y") || !strcmp(c, "y") || !strcmp(c, "yes") || !strcmp(c, "Yes") || !strcmp("YES", c))
		return 1;
	else
		return 0;
}

//С����λ
int addfloat(int x, int y)
{
	float a;
	a = (float)x / y;
	int b;
	b = x / y;
	if (a > b)
		return b + 1;
	else
		return b;
}

//������д���ļ�
void WriteSuperBlock()
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}

	myfile.write((char*)&superblock, 2400);
	myfile.close();
	*signal = 0;
}

//�ļ���ȡ������
void ReadSuperBlock()
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	myfile.read((char*)&superblock, 2400);
	myfile.close();
	*signal = 0;
}

//���������Ϣ
void showsb()
{
	ReadSuperBlock();
	cout << "���ÿռ䣺" << superblock.useddisk << "�ֽ�" << endl;
	cout << "ʣ��ռ䣺" << superblock.remaindisk << "�ֽ�" << endl;
	cout << "�����̿飺" << superblock.usedblock << endl;
	cout << "ʣ���̿飺" << superblock.remainblock << endl;
	cout << "���ýڵ㣺" << superblock.usedinode << endl;//ctime��ʱ��ת��Ϊ����ʾ�ĸ�ʽ���
	cout << "ʣ��ڵ㣺" << superblock.remaininode << endl;
	////���λʾͼ
	//for (int i = 0; i<512; i++)//������+i�ڵ�
	//{
	//	cout<<superblock.block_inodemap[i] ;
	//}
	//cout << endl;
	//cout << superblock.block_inodemap[512];
	//cout << endl;
	//for (int i = 513; i<592; i++)//������+i�ڵ�
	//{
	//	cout << superblock.block_inodemap[i];
	//}
}

//��ʼ��������
void init_sb()
{
	superblock.useddisk = 2400 * 9;//���ô��̿ռ�
	superblock.remaindisk = disksize - superblock.useddisk;//ʣ����̿ռ�
	superblock.usedblock = 9;//�����̿�����
	superblock.remainblock = blockcount - superblock.usedblock;//ʣ���̿�����
	superblock.usedinode = 1;//����i�ڵ�����(��Ŀ¼ռ��һ���ڵ�)
	superblock.remaininode = 31;//ʣ��i�ڵ�����

								//��λʾͼ
	for (int i = 0; i<3; i++)//������+i�ڵ�
	{
		superblock.block_inodemap[i] = 1;
	}
	for (int i = 2; i<blockcount; i++)
	{
		superblock.block_inodemap[i] = 0;
	}
	//��λʾͼ
	superblock.block_inodemap[blockcount] = 1;//��Ŀ¼
	for (int i = 513; i<592; i++)
	{
		superblock.block_inodemap[i] = 0;
	}

	WriteSuperBlock();
}

//i�ڵ�д���ļ�
void WriteINode(int Index, INode &inode)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}

	myfile.seekp(2400 + sizeof(INode)*Index);
	myfile.write((char *)&inode, sizeof(INode));
	myfile.close();
	*signal = 0;
}

//���ݿ�д���ļ�
void Writedata(int Index, char *content)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}

	myfile.seekp(blocksize*Index);
	int t = 0;
	for (int j = 0; j<2400; j++)
	{
		if (content[t] != '\0')
		{
			myfile.write((char*)&content[t], 1);
			t++;
		}
		else
		{
			myfile.write((char*)&content[t], 1);
			break;
		}
	}
	//myfile.write((char *)&content, strlen(content));
	myfile.close();
	*signal = 0;
}

//�ļ���ȡ���ݿ�
void Readdata(int Index, char *content)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	myfile.seekp(blocksize*Index);
	int count = 0;
	for (int j = 0; j<2400; j++)
	{
		myfile.read((char *)&content[count], 1);
		if (strcmp(&content[count], "\0") == 0)
			break;
		count++;
	}
	//myfile.read((char *)&content, 1200);
	myfile.close();
	*signal = 0;
}

//�ļ���ȡi�ڵ�
void ReadINode(int Index, INode &inode)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	myfile.seekp(2400 + sizeof(INode)*Index);
	myfile.read((char *)&inode, sizeof(INode));
	myfile.close();
	*signal = 0;
}

//��ʼ��i�ڵ�
void init_id()
{
	for (int i = 0; i < inodecount; i++)
	{
		inode[i].x = -1;//�ļ��ڲ���ʶ��
		inode[i].name = "1";//�ļ���
		inode[i].size = 0;//�ļ���С
		inode[i].user_name = "1";//�ļ�������
		inode[i].type = -1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
		inode[i].user_quanxian = 3;//rw
		inode[i].size_num = addfloat(inode[i].size, blocksize);//ռ���̿�����
		inode[i].address[0] = -1;//�����ȡ������ݿ��ַ���ɲ������洢��
								 //inode[i].y;//�ļ���дȨ�ޣ�r��w��rw��
		inode[i].father = -1;//��Ŀ¼�ڲ���ʶ��

							 //inode[i].atime;//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
							 //inode[i].mtime;//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
							 //inode[i].ctime;//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

		WriteINode(i, inode[i]);//д���ļ�
	}
}

//��ʼ����Ŀ¼(����ڵ㵫��������)
void init_root()
{
	curdir = 0;//��ǰĿ¼Ϊ��Ŀ¼
	path[0] = 0;
	curpath[0] = '\\';
	curpath[1] = '\0';//��·��

	inode[0].x = 0;//�ļ��ڲ���ʶ��
	inode[0].name = ".";//�ļ���
	inode[0].size = 0;//�ļ���С
	inode[0].user_name = "root";//�ļ�������
	inode[0].type = 0;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
	inode[0].user_quanxian = 3;//rw
							   //inode[0].user_quanxian[N];//�û�����Ȩ��
	inode[0].size_num = addfloat(inode[0].size, blocksize);//ռ���̿�����
														   //inode[0].address[N];//�����ȡ��ŵ�ַ���ɲ������洢��
	inode[0].father = -2;//��Ŀ¼�ڲ���ʶ����root����·����

	time(&inode[0].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
	time(&inode[0].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
	time(&inode[0].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

	WriteINode(0, inode[0]);//д���ļ�
}

//�������
void diskloading()
{
	cout << "--------------------------------------------------" << endl;
	printf("Please Waiting ,Checking disk integrity...\n");
	//�򿪴��̶�������
	Sleep(1000);
	printf("Please Waiting ,Reading system data...\n");
	cout << "--------------------------------------------------" << endl;
	//���볬����
	ReadSuperBlock();
	//����i�ڵ�
	for (int i = 0; i<inodecount; i++)
	{
		ReadINode(i, inode[i]);
	}
	//�������i�ڵ�(succeed!)
	//cout << inode[0].name << endl;

	//��Ŀ¼
	curdir = 0;//��ǰĿ¼Ϊ��Ŀ¼
	path[0] = 0;
	path[1] = -1;
	curpath[0] = '\/';//��·��
	curpath[1] = '\0';//��·��
}

//���´���
void diskupdating()
{
	printf("Please Waiting ,Checking disk integrity...\n");
	//�򿪴��̶�������

	printf("Please Waiting ,Reading system data...\n");
	cout << "--------------------------------------------------" << endl;
	//���볬����
	ReadSuperBlock();
	//����i�ڵ�
	for (int i = 0; i<inodecount; i++)
	{
		ReadINode(i, inode[i]);
	}
	//�������i�ڵ�(succeed!)
	//cout << inode[0].name << endl;

}

//��ʽ������
void format()
{
	printf("Are you sure you want to format the disk? \n The formatted file will not be recoverable.��Yes/No��");
	if (!judge())
		return;
	remove("disk.dat");
	ofstream myfile("disk.dat", fstream::out);
	myfile.close();
	//��ʼ�������鲢д��������ļ�
	//cout <<"test" << endl;
	init_sb();
	//showsb();
	//��ʼ��i�ڵ㲢д��������ļ�
	//cout << "test" << endl;
	init_id();
	//������Ŀ¼��д��������ļ�(��Ŀ¼Ҫ��������-2)
	//cout << "test" << endl;
	init_root();

	//��ʽ��ʣ��Ĵ��̿ռ�(���ݿ�)
	myfile.open("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	myfile.seekp(2400 * 9);//��λ�����ݿ���ʼ��ַ
	myfile << setw(disksize - 2400 * 9) << '\0';
	myfile.close();
	printf("Filesystem created successful.\n");
	diskloading();//�����������
	login();//��¼
}

//���ݴ����ļ��������ָ���
void Backup()
{
	while (*signal == 1) {
		Sleep(500);
	}
	*signal = 1;
	FILE *SourceFile, *BackupFile;
	SourceFile = fopen("disk.dat", "rb");
	BackupFile = fopen("backup.dat", "wb");
	void *buf = malloc(1);
	while (!feof(SourceFile))
	{
		fread(&buf, 1, 1, SourceFile);
		fwrite(&buf, 1, 1, BackupFile);
	}

	fclose(SourceFile);
	fclose(BackupFile);
	printf("�������\n");
	*signal = 0;
}

//�ָ������ļ���Ϣ
void recovery()
{
	while (*signal == 1) {
		Sleep(500);
	}
	*signal = 1;
	FILE *SourceFile, *BackupFile;
	remove("disk.dat");
	ofstream myfile("disk.dat", fstream::out);
	myfile.close();
	SourceFile = fopen("disk.dat", "wb");
	BackupFile = fopen("backup.dat", "rb");
	void *buf = malloc(1);
	while (!feof(BackupFile))
	{
		fread(&buf, 1, 1, BackupFile);
		fwrite(&buf, 1, 1, SourceFile);
	}

	fclose(SourceFile);
	fclose(BackupFile);
	printf("�ָ����\n");

	*signal = 0;
}

//��ʾ��ǰ·��(����·��)
void pwd()
{
	printf("��ǰ·��: %s\n", curpath);
}

//Ѱ�ҷ���������i�ڵ�
int bianli(string name, int curdir1)
{
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir1 && inode[i].name == name)
		{
			return i;
		}
	}
	return -1;
}

//�ж���Ⱥ�û�(������Ⱥ��)
int UID(int curdir1)
{
	fstream myfile("user.txt", ios::in | ios::out);//���ļ�
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	string user_name;
	int user_password;
	int user_UID;
	while (!myfile.eof())//û���ļ�βѭ��
	{
		myfile >> user_name;
		myfile >> user_password;
		myfile >> user_UID;

		if (inode[curdir1].user_name == user_name)
		{
			myfile.close();
			return user_UID;
		}
	}
	myfile.close();
	return -1;
}

//�л���ǰ·��(֧�־���·�������·��)
void cd(char *curpath1)
{
	//char curpath1[0xFF];
	int curdir1 = 0;
	int flag = 0;
	//printf("��ǰ·��: %s\n", curpath);
	//printf("path: ");
	//scanf("%s", curpath1);
	//����·��������·���Ǵ�/��Ҳ����Ϊ��Ŀ¼����ʼ�ģ�
	if (curpath1[0] == '\/'&&strlen(curpath1) == 1)
	{
		curdir = 0;
		path[0] = 0;
		path[1] = '\0';
		curpath[0] = '\/';
		curpath[1] = '\0';
	}
	else if (curpath1[0] == '\/')
	{
		//cout << curdir;
		//strcpy(curpath,curpath1);
		int n = 0;
		int j = 1;
		int i;
		string temp;
		string temp1;
		do
		{
			//cout << curdir1;
			for (i = n + 1; i < strlen(curpath1); i++)
			{
				if (curpath1[i] != '\/')
				{
					temp += curpath1[i];
				}
				else break;
			}
			//temp += '\0';
			if (bianli(temp, curdir1) != -1)
			{
				n = i;
				int t = bianli(temp, curdir1);
				if (UID(t) == user.UID || user.UID == 1 || inode[t].user_quanxian >= 2)
				{
					curdir1 = bianli(temp, curdir1);//��ǰĿ¼��¼
					time(&inode[curdir1].atime);//����·����Ŀ¼�ķ���ʱ��
												//cout << curdir1;
					path[j] = curdir1;//·���ڵ��¼
					j++;
					path[j] = -1;
					temp = temp1;
				}
				else
				{
					printf("Insufficient Authority.......\n");
					flag = 1;
					break;
				}
			}
			else
			{
				printf("Wrong Path.......\n");
				flag = 1;
				break;
			}
		} while (n<strlen(curpath1) - 1);
		if (flag == 0)//δ����ֵ
		{
			strcpy(curpath, curpath1);
			curdir = curdir1;
			//cout << curdir;
		}
	}

	//���·�������·������ . �� .. ��ʼ�ģ�
	else if (curpath1[0] == '.'&&curpath1[1] != '.')//(�ص���Ŀ¼)
	{
		curdir = 0;
		path[0] = 0;
		path[1] = '\0';
		curpath[0] = '\/';
		curpath[1] = '\0';
	}
	else if (curpath1[0] == '.'&&curpath1[1] == '.')//(�ص��ϼ�Ŀ¼)
	{
		if (inode[curdir].father == -2) return;
		curdir = inode[curdir].father;
		for (int j = 0; j < sizeof(path); j++)
		{
			if (path[j] == -1)
			{
				j--;
				path[j] = -1;
				break;
			}
		}
		for (int j = strlen(curpath); curpath[j] == '\/'; j--)
		{
			curpath[j] = '\0';
		}
	}
	else
	{
		printf("Format Error.......\n");
	}

}

//���ĵ�ǰĿ¼���ļ�Ȩ��(chmod 3 file)ϵͳĬ��Ȩ��Ϊ3�����Լ����ܸ��ģ�������������������
//��֧�����ָ��������û�Ȩ��,r=1,w=2
int chmod(string config, int quanxian)
{
	int flag = 0;
	//�ҵ����ļ����ڲ���ʶ��
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == config)
		{
			flag = 1;
			//����û�Ȩ��(root�����ļ�������)
			if (user.UID != 1 && inode[i].user_name != user.name)
			{
				printf("Insufficient authority....");
				return 0;
			}

			inode[i].user_quanxian = quanxian;
			WriteINode(i, inode[i]);
			printf("succeed.....\n");
			diskupdating();
			break;
		}
	}
	if (flag == 0)
		printf("File not Found....\n");
	return 0;
}

//����i�ڵ�
int get_inode()
{
	for (int i = 512; i<592; i++)
		if (superblock.block_inodemap[i] == 0)	//��i�����ɱ�����
		{
			superblock.block_inodemap[i] = 1;
			superblock.remaininode = superblock.remaininode - 1;
			superblock.usedinode = superblock.usedinode + 1;

			WriteSuperBlock();

			return i;
		}
	return -1;
}

//�ͷ�i�ڵ�
void free_inode(int i)
{
	superblock.block_inodemap[i] = 0;
	superblock.remaininode = superblock.remaininode + 1;
	superblock.usedinode = superblock.usedinode - 1;
	WriteSuperBlock();
}

//�����̿�
int get_block()
{
	for (int i = 2; i<512; i++)
		if (superblock.block_inodemap[i] == 0)	//��i���̿�ɱ�����
		{
			superblock.block_inodemap[i] = 1;
			superblock.remainblock = superblock.remainblock - 1;
			superblock.usedblock = superblock.usedblock + 1;
			superblock.useddisk = 2400 * superblock.usedblock;//���ô��̿ռ�
			superblock.remaindisk = disksize - superblock.useddisk;//ʣ����̿ռ�

			WriteSuperBlock();
			return i;
		}
	return -1;
}

// �ͷ��̿�
void free_block(int i)
{
	superblock.block_inodemap[i] = 0;
	superblock.remainblock = superblock.remainblock + 1;
	superblock.usedblock = superblock.usedblock - 1;
	superblock.useddisk = superblock.useddisk - 2400;//���ô��̿ռ�
	superblock.remaindisk = disksize - superblock.useddisk;//ʣ����̿ռ�

	WriteSuperBlock();
}

//������ǰĿ¼����Ŀ¼
int mkdir(string name)
{
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].type == 0 && inode[i].name == name)
		{
			printf("This name already exists.....\n");
			return 0;
		}
	}
	int i = get_inode() - 512;
	if (i != -1 - 512)
	{
		//cout << i;
		inode[i].x = i;//�ļ��ڲ���ʶ��
		inode[i].name = name;//�ļ���
		inode[i].size = 0;//�ļ���С
		inode[i].user_name = user.name;//�ļ�������
		inode[i].type = 0;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���

		inode[i].size_num = addfloat(inode[i].size, blocksize);//ռ���̿�����
		inode[i].user_quanxian = 3;//�����û�Ȩ��
		inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��
		time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
		time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
		time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime
							  //cout << i;
		WriteINode(i, inode[i]);
		printf("succeed.....\n");
		diskupdating();
		return 0;
	}
	else
	{
		printf("Inode is full, failed.....\n");
		return 0;
	}
}

//�������ļ�
void createfile(string config)
{
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].type == 1 && inode[i].name == config)
		{
			printf("This name already exists.....\n");
			return;
		}
	}
	int i = get_inode() - 512;
	if (i != -1 - 512)
	{
		cin.clear();
		fflush(stdin);
		char *content = '\0';
		int length = 0;
		cout << "Do you want to enter the contents of the file?(Y\\N)";
		if (judge())
		{
			cout << "Please enter the contents of the file:";
			//�ȷ���һ���̿飬��������ٷ���һ���̿�
			//���ʣ�����ݿ��ڴ涼������������
			//�Ȱ����ݴ浽�������ж�
			content = new char[superblock.remainblock * blocksize];//ʣ������ܿռ�
			cin.getline(content, superblock.remainblock * blocksize, '\n');
			length = strlen(content);
			content[length] = '\0';

			if (addfloat(length, blocksize) > superblock.remainblock)
			{
				cout << "Insufficient disk space, file creation failed....." << endl << endl;
				free_inode(i + 512);
				return;
			}
			else {
				//�����̿�
				//cout << length;
				//cout << addfloat(length, blocksize);
				for (int j = 0; j < addfloat(length, blocksize); j++)
				{
					//cout << addfloat(length, blocksize);
					inode[i].size_num = j + 1;//ռ���̿�����
					inode[i].address[j] = get_block();//�����ȡ��ŵ�ַ���ɲ������洢��
													  //cout << inode[i].size_num;
				}
				//���ַ����ֽ⣡
				char content1[1200][1024];
				for (int j = 0; j < inode[i].size_num - 1; j++)
				{
					strncpy(content1[j], content, 1024);
					for (int ii = 0; ii < strlen(content); ii++) {
						content[ii] = content[ii + 1024];
					}
				}
				strcpy(content1[inode[i].size_num - 1], content);

				//���ݿ�д���ļ�()

				for (int j = 0; j < inode[i].size_num; j++)
				{
					Writedata(inode[i].address[j], content1[j]);
				}

				//�����ļ���Ϣ
				inode[i].x = i;//�ļ��ڲ���ʶ��
				inode[i].name = config;//�ļ���
				inode[i].size = length;//�ļ���С
				inode[i].user_name = user.name;//�ļ�������
				inode[i].type = 1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
				inode[i].user_quanxian = 3;
				inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��

				time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
				time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
				time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

				WriteINode(i, inode[i]);

				int pp = inode[i].size;
				//ֻҪ���Ǹ��ڵ�Ͱְ֣�������������
				if (inode[i].father != -2)
				{
					inode[inode[i].father].size = inode[inode[i].father].size + pp;
					WriteINode(inode[i].father, inode[inode[i].father]);
					i = inode[i].father;
				}
				inode[inode[i].father].size = inode[inode[i].father].size + pp;
				WriteINode(inode[i].father, inode[inode[i].father]);
				printf("succeed.....\n");
				diskupdating();
			}
		}

		else
		{
			//�����ļ���Ϣ
			inode[i].x = i;//�ļ��ڲ���ʶ��
			inode[i].name = config;//�ļ���
			inode[i].size = 0;//�ļ���С
			inode[i].user_name = user.name;//�ļ�������
			inode[i].type = 1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��
			inode[i].size_num = 0;
			time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
			time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
			time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

			WriteINode(i, inode[i]);
			printf("succeed.....\n");
			diskupdating();
		}

	}
	else
	{
		printf("Inode is full, failed.....\n");
		return;
	}
}

//��ʾ��ǰʱ��
void time()
{
	char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	printf("%d\/%d\/%d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
	printf("%s %d:%d:%d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
}

//ʱ��ת��
void _time(time_t &m_time) {
	time_t timep = m_time;
	struct tm *p;
	if (timep < 0)
	{
		time(&timep);
		printf("Unlethel Time error dectected\n");
	}
	p = localtime(&timep); //ȡ�õ���ʱ��
	printf("%02d\/%02d\/%02d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
	printf(" %02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
}

//�ļ���С��������������������������������������������������������������������������������������
//��ʾ��ǰĿ¼�µ���Ϣ���ļ�
void dir()
{
	//mu.lock();
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir)
		{
			_time(inode[i].ctime);
			if (inode[i].type == 0)
				cout << "      " << inode[i].user_name << "      <DIR>     " << inode[i].size << "       " << inode[i].name << endl;
			else
				cout << "      " << inode[i].user_name << "              " << inode[i].size << "       " << inode[i].name << endl;
		}
	}
	//��Ŀ¼����ʱ��ı�
	time(&inode[curdir].atime);
	//���ļ�����ʱ��ı�
	//time(&inode[i].atime);
	WriteINode(curdir, inode[curdir]);
	//mu.unlock();
}

//�˳�ϵͳ����
void exit()
{
	msg("Waiting Next Use <YCOS> 2019.06");
}

//��ʾ��ǰ·�����ļ�����
void more(string name)
{
	int flag = 0;
	int i;
	for (i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].type == 1 && inode[i].name == name)
		{
			flag = 1;
			break;
		}
	}
	if (flag == 0)
	{
		cout << "File not found......" << endl;
		return;
	}
	//��Ŀ¼����ʱ��ı�
	time(&inode[curdir].atime);
	WriteINode(curdir, inode[curdir]);//д���ļ�
									  //���ļ�����ʱ��ı�
	time(&inode[i].atime);
	WriteINode(i, inode[i]);//д���ļ�
	char *content = new char[superblock.remainblock * blocksize];
	strcpy(content, "\0");
	char content1[1200][1024];
	for (int j = 0; j < inode[i].size_num; j++)
	{
		Readdata(inode[i].address[j], content);
		strcpy(content1[j], content);
	}
	for (int j = 0; j < inode[i].size_num; j++)
	{
		cout << content1[j] << endl;
	}
}

//ɾ����ǰĿ¼��ĳ���ļ�
int rm(string name, int mulu)
{
	//�ҵ����ļ�
	int flag = 0;
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == mulu && inode[i].type == 1 && inode[i].name == name)
		{
			flag = i;
			break;
		}
	}

	//�ͷ����ݿ�,�ж��ļ�ռ�ü������ݿ�
	for (int i = 0; i < inode[flag].size_num; i++)
	{
		free_block(inode[flag].address[i]);

		fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//���ļ�
		if (!myfile.is_open())
		{
			printf("\nCan't open file %s.\n", "disk.dat");
			printf("This filesystem not exist!\n");
			system("pause");
			exit(0);
		}
		myfile.seekp(2400 * inode[flag].address[i]);//��λ�����ݿ���ʼ��ַ
		myfile << setw(2400);
		myfile.close();
	}

	//�ͷ�i�ڵ�
	free_inode(flag + 512);
	inode[inode[flag].father].size = inode[inode[flag].father].size - inode[flag].size;
	inode[flag].x = -1;//�ļ��ڲ���ʶ��
	inode[flag].name = "";//�ļ���
	inode[flag].size = 0;//�ļ���С
	inode[flag].user_name = "";//�ļ�������
	inode[flag].type = -1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
	inode[flag].user_quanxian = 3;//rw
	inode[flag].size_num = addfloat(inode[flag].size, blocksize);//ռ���̿�����
	inode[flag].address[0] = -1;//�����ȡ������ݿ��ַ���ɲ������洢��
								//inode[i].y;//�ļ���дȨ�ޣ�r��w��rw��
	inode[flag].father = -1;//��Ŀ¼�ڲ���ʶ��

							//inode[i].atime;//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
							//inode[i].mtime;//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
							//inode[i].ctime;//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime
	WriteINode(inode[flag].father, inode[inode[flag].father]);
	WriteINode(flag, inode[flag]);//д���ļ�

	time(&inode[mulu].atime);
	time(&inode[mulu].ctime);
	WriteINode(mulu, inode[mulu]);//д���ļ�
	diskupdating();
	return 0;
}

//ɾ����ǰ·����ĳ��Ŀ¼
int rmdir(string name, int mulu)
{
	int flag = 0;//��¼Ҫɾ����Ŀ¼���

				 //�����õ���ǰ���
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == mulu && inode[i].type == 0 && inode[i].name == name)
		{
			flag = i;
			break;
		}
	}

	//Ŀ¼Ϊ��ֱ��ɾ��
	int baba[N];
	int flag1 = 0;//��¼�Ƿ��ѱ�ɾ��
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == flag)
		{
			baba[flag1] = i;//��Ҫ��¼��ǰ��Щ�ļ��İְ�������������������
			flag1++;

		}
	}
	//ֱ��ɾ��Ŀ¼
	if (flag1 == 0)
	{
		inode[flag].x = -1;//�ļ��ڲ���ʶ��
		inode[flag].name = "";//�ļ���
		inode[flag].size = 0;//�ļ���С
		inode[flag].user_name = "";//�ļ�������
		inode[flag].type = -1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
		inode[flag].user_quanxian = 3;//rw
		inode[flag].size_num = addfloat(inode[flag].size, blocksize);//ռ���̿�����
		inode[flag].address[0] = -1;//�����ȡ������ݿ��ַ���ɲ������洢��
									//inode[i].y;//�ļ���дȨ�ޣ�r��w��rw��
		inode[flag].father = -1;//��Ŀ¼�ڲ���ʶ��

								//inode[i].atime;//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
								//inode[i].mtime;//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
								//inode[i].ctime;//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

		WriteINode(flag, inode[flag]);//д���ļ�
		free_inode(flag + 512);//�ͷ�i�ڵ�
	}


	//Ŀ¼������Ҫ�ݹ�ɾ��������,ע�����ݿ��ɾ��
	else
	{
		for (int i = 0; i < flag1; i++)
		{
			//�ļ�����rm
			if (inode[baba[i]].type == 1)
			{
				rm(inode[baba[i]].name, inode[baba[i]].father);
			}

			//Ŀ¼����rmdir
			else if (inode[baba[i]].type == 0)
			{
				rmdir(inode[baba[i]].name, inode[baba[i]].father);
			}
		}
		inode[flag].x = -1;//�ļ��ڲ���ʶ��
		inode[flag].name = "";//�ļ���
		inode[flag].size = 0;//�ļ���С
		inode[flag].user_name = "";//�ļ�������
		inode[flag].type = -1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
		inode[flag].user_quanxian = 3;//rw
		inode[flag].size_num = addfloat(inode[flag].size, blocksize);//ռ���̿�����
		inode[flag].address[0] = -1;//�����ȡ������ݿ��ַ���ɲ������洢��
									//inode[i].y;//�ļ���дȨ�ޣ�r��w��rw��
		inode[flag].father = -1;//��Ŀ¼�ڲ���ʶ��

								//inode[i].atime;//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
								//inode[i].mtime;//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
								//inode[i].ctime;//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

		WriteINode(flag, inode[flag]);//д���ļ�
		free_inode(flag + 512);//�ͷ�i�ڵ�
	}
	//��Ŀ¼����ʱ��ı�
	time(&inode[mulu].atime);
	time(&inode[mulu].ctime);
	WriteINode(mulu, inode[mulu]);//д���ļ�

								  //���ļ�����ʱ��ı�
								  //time(&inode[i].atime);
	diskupdating();
	return 0;
}

//���Ƶ�ǰĿ¼���ļ�����һ·��
void copy(string name, char *curpath1)
{
	//�ж��û�Ȩ�ޣ�������������������������
	//����ͬ���ļ����滻/ȡ������
	//������ͬ���ļ��ĸ��ƿ��Ե�������ڵ�
	int i;
	int flag = 0;
	//���ҵ�Ŀ���ļ�
	for (i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == name&& inode[i].type == 1)
		{
			flag = i;
			break;
		}
	}

	if (flag == 0) cout << "�ļ������ڣ�" << endl;

	else//���Ը���
	{
		cd(curpath1);//�ҵ�����·��


					 //����ڵ㴴�����ļ�//�ļ����ݸ���
		for (int i = 0; i < inodecount; i++)
		{
			if (inode[i].father == curdir && inode[i].type == 1 && inode[i].name == name)
			{
				printf("This name already exists.....\n");
				return;
			}
		}

		//diskupdating();
		i = get_inode() - 512;

		//
		if (i != -1 - 512)
		{
			//����i�ڵ���Ϣ
			inode[i].x = i;//�ļ��ڲ���ʶ��
						   //cout<< name;
			inode[i].name = name;//�ļ���
								 //Sleep(2000);	
			inode[i].size = inode[flag].size;//�ļ���С

			inode[i].user_name = user.name;//�ļ�������
			inode[i].type = 1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��

			inode[i].size_num = inode[flag].size_num;//ռ���̿�����
			if (superblock.remainblock < inode[flag].size_num)
			{
				cout << "�洢�ռ䲻��......" << endl;
				return;
			}
			for (int j = 0; j < inode[flag].size_num; j++)
			{
				inode[i].address[j] = get_block();
			}

			time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
			time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
			time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

			WriteINode(i, inode[i]);
			int pp = inode[i].size;
			//ֻҪ���Ǹ��ڵ�Ͱְ֣�������������
			if (inode[i].father != -2)
			{
				inode[inode[i].father].size = inode[inode[i].father].size + pp;
				WriteINode(inode[i].father, inode[inode[i].father]);
				i = inode[i].father;
			}
			inode[inode[i].father].size = inode[inode[i].father].size + pp;
			WriteINode(inode[i].father, inode[inode[i].father]);


			//�ļ�����д�����ݿ�
			char *content = new char[superblock.remainblock * blocksize];
			strcpy(content, "\0");
			char content1[1200][1024];
			for (int j = 0; j < inode[flag].size_num; j++)
			{
				Readdata(inode[flag].address[j], content);
				Writedata(inode[i].address[j], content);

			}
			printf("succeed.....\n");
			diskupdating();
		}
		else
		{
			printf("Inode is full, failed.....\n");
			return;
		}
	}
}

//�ƶ���ǰĿ¼�ļ�
void move(string name, char *curpath1)
{
	//��¼��ǰ·��
	int curdir1 = curdir;
	char curpath0[0xFF];
	strcpy(curpath0, curpath);
	//����copy��Ŀ��·��
	copy(name, curpath1);
	cd(curpath0);
	//rmɾ�����ļ�
	rm(name, curdir1);
}

//�ƶ���ǰĿ¼Ŀ¼
void movedir(string name, char *curpath1)
{
	//��¼��ǰ·��
	int curdir1 = curdir;
	char curpath0[0xFF];
	strcpy(curpath0, curpath);
	//����copy��Ŀ��·��
	xcopy(name, curpath1);
	cd(curpath0);
	//rmɾ�����ļ�
	rmdir(name, curdir1);
}

//��ǰ·�����ļ�����Ŀ¼������
void rename(string name, string name1)
{
	int i, flag;
	for (i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == name)
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)
	{
		inode[i].name = name1;
		time(&inode[i].atime);
		time(&inode[i].ctime);
		WriteINode(i, inode[i]);//д���ļ�
	}
	else
	{
		cout << "File not found......" << endl;
	}
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Ϊʲô����Ķ�ջ�쳣
//����Ŀ¼��һ��·������һ��·��(��ɾ��Ŀ¼���ƣ���Ҫ�ݹ�ɾ��)
void xcopy(string name, char *curpath1)
{
	char nowpath[100];
	strcpy(nowpath, curpath);
	int i;
	int flag = -1;
	//���ҵ�Ŀ��Ŀ¼
	for (i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == name&& inode[i].type == 0)
		{
			flag = i;
			break;
		}
	}
	if (flag == -1)
	{
		cout << "�ļ������ڣ�" << endl;
		return;
	}

	else//���Ը���
	{
		cd(curpath1);//�ҵ�����·��

					 //Ŀ¼Ϊ��ֱ�Ӹ��ƽڵ㼴��
		int baba[N];
		int flag1 = 0;//��¼�Ƿ��ѱ�����
		for (int i = 0; i < inodecount; i++)
		{
			if (inode[i].father == flag)
			{
				baba[flag1] = i;//��Ҫ��¼��ǰ��Щ�ļ��İְ�������������������
				flag1++;
			}
		}

		i = get_inode() - 512;
		if (i != -1 - 512)
		{
			//����i�ڵ���Ϣ
			inode[i].x = i;//�ļ��ڲ���ʶ��
						   //cout << inode[flag].name;
						   //cout << inode[i].name;
			inode[i].name = inode[flag].name;//�ļ���
			inode[i].size = inode[flag].size;//�ļ���С
			inode[i].user_name = user.name;//�ļ�������
			inode[i].type = 0;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��
									 //inode[curdir].size = inode[curdir].size + inode[flag].size;
			inode[i].size_num = inode[flag].size_num;//ռ���̿�����
			if (superblock.remainblock < inode[flag].size_num)
			{
				cout << "�洢�ռ䲻��......" << endl;
				return;
			}
			for (int j = 0; j < inode[flag].size_num; j++)
			{
				inode[i].address[j] = get_block();
			}

			time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
			time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
			time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

			WriteINode(i, inode[i]);
			int ff = i;
			//WriteINode(curdir, inode[curdir]);
			int pp = inode[i].size;
			//ֻҪ���Ǹ��ڵ�Ͱְ֣�������������
			if (inode[i].father != -2)
			{
				inode[inode[i].father].size = inode[inode[i].father].size + pp;
				WriteINode(inode[i].father, inode[inode[i].father]);
				i = inode[i].father;
			}
			inode[inode[i].father].size = inode[inode[i].father].size + pp;
			WriteINode(inode[i].father, inode[inode[i].father]);
			i = ff;
			diskupdating();
		}
		else
		{
			printf("Inode is full, failed.....\n");
			return;
		}

		if (flag1 != 0)//�к���
		{
			char curpath11[100];
			strcpy(curpath11, curpath1);
			char p[100];
			strcpy(p, inode[i].name.c_str());
			char p1[100];
			strcpy(p1, inode[flag].name.c_str());

			strcat(curpath11, "/");
			strcat(curpath11, p);
			if (strcmp(nowpath, "/") != 0)
			{
				strcat(nowpath, "/");
				strcat(nowpath, p1);
			}
			else
			{
				strcat(nowpath, p1);
			}
			for (int i = 0; i < flag1; i++)
			{
				//�ļ�����copy
				if (inode[baba[i]].type == 1)
				{
					cd(nowpath);
					copy(inode[baba[i]].name, curpath11);
					inode[curdir].size = inode[curdir].size - inode[baba[i]].size;
					WriteINode(curdir, inode[curdir]);
				}

				//Ŀ¼����xcopy
				else if (inode[baba[i]].type == 0)
				{
					cd(nowpath);
					xcopy(inode[baba[i]].name, curpath11);
					//inode[curdir].size = inode[curdir].size - inode[baba[i]].size;
					int ff = baba[i];
					int pp = inode[baba[i]].size;
					//ֻҪ���Ǹ��ڵ�Ͱְ֣�������������
					if (inode[baba[i]].father != -2)
					{
						inode[inode[baba[i]].father].size = inode[inode[baba[i]].father].size - pp;
						WriteINode(inode[baba[i]].father, inode[inode[baba[i]].father]);
						baba[baba[i]] = inode[baba[i]].father;
					}
					inode[inode[baba[i]].father].size = inode[inode[baba[i]].father].size - pp;
					WriteINode(inode[baba[i]].father, inode[inode[baba[i]].father]);
					baba[i] = ff;

				}
			}
		}
		printf("succeed.....\n");
		diskupdating();
	}
}

//�ӱ��ص����ļ�
void import(char *curpath1, int mulu)
{

	char *data;
	int filesize;
	// ���ļ���d:\a.txt��
	FILE *file;
	if ((file = fopen(curpath1, "r")) == NULL)
	{
		printf("Can't open %s\n", curpath1);
		//Sleep(2000);
		exit(0);
	}
	// ����ļ���С
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	// �����ڴ�
	data = (char*)malloc(filesize + 1);
	// ��ȡ�ļ�
	fread(data, sizeof(char), filesize, file);
	data[filesize] = 0;
	fclose(file);
	// ������ݣ���������ݸ�ʲô�������ˣ�
	//printf("%s", data);

	//�����ļ���
	char p[100];
	int j = 0;
	for (int i = 4; i < strlen(curpath1); i++)
	{
		if (curpath1[i] != '\\' && curpath1[i] != '\0')
		{
			p[j] = curpath1[i];
			j++;
		}
		if (curpath1[i] == '\\')
		{
			i = i + 1;
			*p = '\0';
			j = 0;
		}

	}
	p[j] = '\0';
	/*string config;
	for (int i = 0; i < strlen(p); i++)
	{
	config += p[i];
	}
	config += '\0';*/
	//����i�ڵ�
	/*for (int i = 0; i < inodecount; i++)
	{
	if (inode[i].father == curdir && inode[i].type == 1 && inode[i].name == p)
	{
	printf("This name already exists.....\n");
	return;
	}
	}*/

	//Sleep(2000);
	int i = get_inode() - 512;
	if (i != -1 - 512)
	{
		inode[i].name = p;//�ļ���
		inode[i].user_name = user.name;//�ļ�������
		cin.clear();
		//fflush(stdin);
		char *content = '\0';
		int length = 0;
		//cout << "Do you want to enter the contents of the file?(Y\\N)";
		//cout << "Please enter the contents of the file:";
		//�ȷ���һ���̿飬��������ٷ���һ���̿�
		//���ʣ�����ݿ��ڴ涼������������
		//�Ȱ����ݴ浽�������ж�
		content = new char[superblock.remainblock * blocksize];//ʣ������ܿռ�
		strcpy(content, data);
		//cin.getline(content, superblock.remainblock * blocksize, '\n');
		length = strlen(content);
		content[length] = '\0';

		if (addfloat(length, blocksize) > superblock.remainblock)
		{
			cout << "Insufficient disk space, file creation failed....." << endl << endl;
			free_inode(i + 512);
			return;
		}
		else {
			//�����̿�
			//cout << length;
			//cout << addfloat(length, blocksize);
			for (int j = 0; j < addfloat(length, blocksize); j++)
			{
				//cout << addfloat(length, blocksize);
				inode[i].size_num = j + 1;//ռ���̿�����
				inode[i].address[j] = get_block();//�����ȡ��ŵ�ַ���ɲ������洢��
												  //cout << inode[i].size_num;
			}
			char content1[1200][2400];

			for (int j = 0; j < inode[i].size_num - 1; j++)
			{
				strncpy(content1[j], content, 2400);
				for (int m = 0; m < strlen(content); m++)
				{
					content[m] = content[m + 2400];
				}
			}
			strcpy(content1[inode[i].size_num - 1], content);
			//���ݿ�д���ļ�()

			for (int j = 0; j < inode[i].size_num; j++)
			{
				Writedata(inode[i].address[j], content1[j]);
			}

			//�����ļ���Ϣ
			inode[i].x = i;//�ļ��ڲ���ʶ��
						   //
			inode[i].size = length;//�ļ���С

			inode[i].type = 1;//�ļ����ͣ�0ΪĿ¼��1Ϊ�ļ���
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//��Ŀ¼�ڲ���ʶ��

			time(&inode[i].atime);//����ʱ��,ÿ�β鿴�ļ����ݵ�ʱ������
			time(&inode[i].mtime);//�޸�ʱ��,ֻ���޸����ļ������ݣ��Ż�����ļ���mtime
			time(&inode[i].ctime);//�޸�ʱ��,���ļ��������޸��ļ��������Ȳ����κβ���������ctime

			WriteINode(i, inode[i]);

			inode[inode[i].father].size = inode[inode[i].father].size + inode[i].size;
			WriteINode(inode[i].father, inode[inode[i].father]);

			printf("succeed.....\n");
			diskupdating();
		}

	}
	else
	{
		printf("Inode is full, failed.....\n");
		return;
	}
	return;
}

//BF�ַ���ƥ��
int BF(char S[], char T[]) {
	int i = 0, j = 0;//iָ��S��jָ��T
	int length_s = strlen(S), length_t = strlen(T);
	//printf("%d  %d\n",length_s,length_t);
	while (i<length_s&&j<length_t) {
		//printf("%c %c\n",S[i],T[j]);
		if (S[i] == T[j]) {
			i++;
			j++;
		}
		else {
			i = i - j + 1;//i���ݵ���һ��Ԫ�ؿ�ʼƥ�� 
			j = 0;
		}
	}
	//ƥ�����
	if (j >= length_t) {
		i = i - j;//i���ݵ���ǰƥ��Ŀ�ʼ�ط� 
		return i;
	}
	return 0;

}

//Ѱ���ļ��е�һ���ַ�
void find(char *curpath1)
{
	int fff = 1;
	int ffff = 1;
	//���ظ����ݿ��Ӧ���ļ��ڵ�
	for (int i = 2; i<blockcount; i++)//�����������ݿ�
	{
		fff = 1;
		char *content = new char[superblock.remainblock * blocksize];
		strcpy(content, "\0");
		Readdata(i, content);
		string father1;
		if (BF(content, curpath1) != 0)
		{//�ҵ�������������������������������������
			for (int ii = 0; ii < inodecount; ii++)
			{//�ҽڵ㣡������������1
				for (int iii = 0; iii < inode[ii].size_num; iii++)
				{//�ҽڵ������ݿ飡������������
					if (inode[ii].address[iii] == i)
					{
						father1 += inode[ii].name;
						father1 += " From ";
						while (inode[ii].father != 0)
						{
							father1 += inode[inode[ii].father].name;
							father1 += " From ";
							ii = inode[ii].father;
						}
						father1 += inode[inode[ii].father].name;
						cout << father1 << endl;
						fff = 0;
						ffff = 1;
						break;
					}
				}
				if (fff == 0) break;
			}
		}

	}
	if (ffff == 0) cout << "NOT FOUND......" << endl;
}

//��ʾ��ǰĿ¼���ļ����������û�����������
void attrib(string name)
{
	int flag = 0;
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir)
		{
			flag = 1;
			if (inode[i].user_quanxian == 3)
				cout << "rw" << "\t" << curpath << inode[i].name << endl;
			else if (inode[i].user_quanxian == 2)
				cout << "r" << "\t" << curpath << inode[i].name << endl;
			else if (inode[i].user_quanxian == 3)
				cout << "w" << "\t" << curpath << inode[i].name << endl;
		}
	}
	if (flag == 0)
	{
		cout << "��ǰĿ¼�����ļ�..." << endl;
	}
}

//���뺯��
void input()
{
	cmdhead();

	cin.getline(cmd, 250);

	//��ֵ�һ����
	int i = 0;
	while ((cmd[i] != '\0') && (cmd[i] != ' '))
	{
		cmd1[i] = cmd[i];
		i++;
	}
	cmd1[i] = '\0';
	//��ֵڶ�����
	int j = 0;
	if (cmd[i - 1] != '\0')
	{
		i++;
		while ((cmd[i] != '\0') && (cmd[i] != ' '))
		{
			cmd2[j] = cmd[i];
			i++;
			j++;
		}
		cmd2[j] = '\0';
	}
	//��ֵ�������
	j = 0;
	if (cmd[i - 1] != '\0')
	{
		i++;
		while ((cmd[i] != '\0') && (cmd[i] != ' '))
		{
			cmd3[j] = cmd[i];
			i++;
			j++;
		}
		cmd3[j] = '\0';
	}
}

//charתstring����
void charstring(char *config)
{
	cmd11 = "";
	for (int i = 0; i < strlen(config); i++)
	{
		cmd11 += config[i];
	}
	//cmd11 += '\0';
}
//charתstring����
void charstring1(char *config)
{
	cmd111 = "";
	for (int i = 0; i < strlen(config); i++)
	{
		cmd111 += config[i];
	}
	//cmd111 += '\0';
}

//charתint����
void intstring(char *config)
{
	cmd22 = (int)(config - 48);
}

//�汾��Ϣ
void ver()
{
	cout << "ver.1.1.0  Made by Y.C.Q. Directed by M.W." << endl;
}

//ϵͳ֧������ܽ���
void help()
{
	printf("ϵͳ֧������ܽ���:\n\n");
	printf("(30+3��ָ��)\n\n");
	printf("���������      �����ʽ               ���ý���\n");
	printf("cls            cls                   �����Ļ\n");
	printf("help           help                  �汾��Ϣ���������\n");
	printf("login          login                 ��¼һ���µ��˻�\n");
	printf("useradd        useradd               ���һ���µ��˻�\n");
	printf("exit           exit                  �������η���\n");
	printf("format         format                ��ʽ���������\n");
	printf("showsb         showsb                ��ʾ����ʹ�����\n");
	printf("time           time                  ��ʾ��ǰϵͳʱ��\n");
	printf("ver            ver                   ��ʾ��ǰϵͳ�汾\n");
	printf("pwd            pwd                   ��ʾ��ǰ·��\n");
	printf("mkdir          mkdir dirname         ����һ����Ŀ¼\n");
	printf("dir\\ls         dir\\ls                ��ʾ��ǰĿ¼�µ���Ϣ���ļ�\n");
	printf("cd             cd path               �л�·��\n");
	printf("mkfile         mkfile filename       ����һ�����ļ�\n");
	printf("rmdir          rmdir dirname         ɾ����ǰ·����һ��Ŀ¼���ݹ飩\n");
	printf("rm\\del         rmdir\\del filename    ɾ����ǰ·����һ���ļ�\n");
	printf("cat\\more       cat\\more filename     ��ʾ�ļ�����\n");
	printf("rename         rename name           ������һ���ļ���Ŀ¼\n");
	printf("chmod          chmod name mode       �޸��ļ���Ŀ¼�ķ���Ȩ��\n");
	printf("attrib         attrib                ��ʾ��ǰĿ¼�������ļ�Ŀ¼����\n");
	printf("find           find txt              Ѱ�Ұ���ָ���ı����ļ�\n");
	printf("copy           copy filename path    ��һ���ļ���������һĿ¼\n");
	printf("xcopy          xcopy dirname path    ��һ��Ŀ¼��������һĿ¼���ݹ飩\n");
	printf("export         export filename path  ������������ݵ������������\n");
	printf("import         import path           ��������̵��뵽�����������\n");
	printf("backup         backup                ���������������\n");
	printf("recover        recover               �ָ������������\n");
	printf("diskupdate     diskupdate            �����������\n");
	printf("move           move filename path    �ƶ��ļ���ĳ��·��\n");
	printf("movedir        movedir dirname path  �ƶ�Ŀ¼��ĳ��·��\n");
}

//����ѡ��
void choice()
{
	//����
	if (!strcmp(cmd1, "cls"))
	{
		system("cls");
		head();
	}
	//�����ĵ�
	else if (!strcmp(cmd1, "help"))
	{
		help();
	}
	//���µ�¼
	else if (!strcmp(cmd1, "login"))
	{
		login();
	}
	//����û�
	else if (!strcmp(cmd1, "useradd"))
	{
		useradd();
	}
	//����Ȩ��
	else if (!strcmp(cmd1, "chmod"))
	{
		charstring(cmd2);
		intstring(cmd3);
		chmod(cmd11, cmd22);
	}
	//�˳�ϵͳ
	else if (!strcmp(cmd1, "exit"))
	{
		exit();
		Sleep(2000);
		exit(0);
	}
	//��ʽ������
	else if (!strcmp(cmd1, "format"))
	{
		format();
	}
	//���´���
	else if (!strcmp(cmd1, "diskupdate"))
	{
		diskupdating();
	}
	//����ʹ�����
	else if (!strcmp(cmd1, "showsb"))
	{
		showsb();
	}
	//��ʾʱ��
	else if (!strcmp(cmd1, "time"))
	{
		time();
	}
	//��ʾ�汾
	else if (!strcmp(cmd1, "ver"))
	{
		ver();
	}
	//��ʾ��Ŀ¼/�ļ�
	else if (!strcmp(cmd1, "dir"))
	{
		dir();
	}
	//��ʾ��Ŀ¼/�ļ�(ͬdir)
	else if (!strcmp(cmd1, "ls"))
	{
		dir();
	}
	//��ʾ��ǰ·��
	else if (!strcmp(cmd1, "pwd"))
	{
		pwd();
	}
	//�л�·��
	else if (!strcmp(cmd1, "cd"))
	{
		cd(cmd2);
	}
	//����Ŀ¼
	else if (!strcmp(cmd1, "mkdir"))
	{
		charstring(cmd2);
		mkdir(cmd11);
	}
	//�����ļ�
	else if (!strcmp(cmd1, "mkfile"))
	{
		charstring(cmd2);
		createfile(cmd11);
	}
	//ɾ��Ŀ¼�����ݹ飩
	else if (!strcmp(cmd1, "rmdir"))
	{
		charstring(cmd2);
		rmdir(cmd11, curdir);
	}
	//ɾ���ļ�
	else if (!strcmp(cmd1, "rm"))
	{
		charstring(cmd2);
		rm(cmd11, curdir);
	}
	//ɾ���ļ�(ͬrm)
	else if (!strcmp(cmd1, "del"))
	{
		rm(cmd2, curdir);
	}
	//�������ļ�
	else if (!strcmp(cmd1, "rename"))
	{
		charstring(cmd2);
		charstring1(cmd3);
		rename(cmd11, cmd111);
	}
	//��ʾ�ļ�����
	else if (!strcmp(cmd1, "more"))
	{
		charstring(cmd2);
		more(cmd11);
	}
	//��ʾ�ļ����ݣ�ͬmore��
	else if (!strcmp(cmd1, "cat"))
	{
		more(cmd2);
	}
	//�����ļ�
	else if (!strcmp(cmd1, "copy"))
	{
		charstring(cmd2);
		copy(cmd11, cmd3);
	}
	//����Ŀ¼�����ݹ飩
	else if (!strcmp(cmd1, "xcopy"))
	{
		charstring(cmd2);
		xcopy(cmd11, cmd3);
	}
	//�ƶ��ļ�
	else if (!strcmp(cmd1, "move"))
	{
		charstring(cmd2);
		move(cmd11, cmd3);
	}
	//�ƶ�Ŀ¼
	else if (!strcmp(cmd1, "movedir"))
	{
		charstring(cmd2);
		movedir(cmd11, cmd3);
	}
	//�ӱ��ش��̸������ݵ�����Ĵ���������
	else if (!strcmp(cmd1, "import"))
	{
		import(cmd2, curdir);
	}
	//������Ĵ����������������ݵ����ش���
	else if (!strcmp(cmd1, "export"))
	{
		pexport(cmd2, cmd3);
	}
	//Ѱ���ļ��е�һ���ַ�
	else if (!strcmp(cmd1, "find"))
	{
		find(cmd2);
	}
	//��ʾ��ǰĿ¼���ļ�Ȩ��
	else if (!strcmp(cmd1, "attrib"))
	{
		attrib(cmd2);
	}
	//���ݴ���
	else if (!strcmp(cmd1, "backup"))
	{
		Backup();
	}
	//�ָ�����
	else if (!strcmp(cmd1, "recover"))
	{
		recovery();
	}
	else
	{
		printf("Invalid command........\n");
	}
}

//�������
void flush()
{
	strcpy(cmd, "\0");
	strcpy(cmd1, "\0");
	strcpy(cmd2, "\0");
	strcpy(cmd3, "\0");
	cmd11 = "";
	cmd22 = 0;
}

//����ǰĿ¼�µ�a.txt����������C��(��֪�Ƿ���Ҫɾ�����ļ�)
void pexport(char *curpath1, char *curpath2)
{
	//�ڱ���c�̽���ͬ���ļ�
	char name[100];
	strcpy(name, curpath2);
	if (strcmp(curpath2, "C:\\"))
	{
		strcat(name, "\\");
	}
	strcat(name, curpath1);
	FILE *file;
	if ((file = fopen(name, "w")) == NULL)
	{
		printf("Can't open %s\n", name);
		//Sleep(2000);
		exit(0);
	}
	//��ȡ���ļ����ݽ���д��c���ļ�
	int i;
	int flag = -1;
	//���ҵ�Ŀ���ļ�
	for (i = 0; i < inodecount; i++)
	{
		const char *p = inode[i].name.c_str();
		if (inode[i].father == curdir && !strcmp(p, curpath1) && inode[i].type == 1)
		{
			flag = i;
			break;
		}
	}
	if (flag == -1)
	{
		cout << "�ļ������ڣ�" << endl;
		return;
	}
	char *content = new char[superblock.remainblock * blocksize];
	strcpy(content, "\0");
	char content1[1200][1024];
	for (int j = 0; j < inode[i].size_num; j++)
	{
		Readdata(inode[i].address[j], content);
		strcpy(content1[j], content);
	}

	//д���ļ�
	fwrite(content, strlen(content), 1, file);
	fclose(file);

}

