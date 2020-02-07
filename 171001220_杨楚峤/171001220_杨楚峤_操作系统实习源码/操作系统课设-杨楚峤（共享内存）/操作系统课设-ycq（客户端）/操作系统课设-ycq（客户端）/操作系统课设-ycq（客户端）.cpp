// 操作系统课设-杨楚峤.cpp : 定义控制台应用程序的入口点。
//本课设仿照Linux操作系统设计文件系统基本格式
//模仿windows为文件赋权限
//语言为C/C++混用
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
#define N 100 //通用数组大小

//单位全部为字节B；
//磁盘空间总计1024*2048
//每8KB设置一个inode，每个i节点的大小是128；
//i节点空间总计128*512，占磁盘3.1%，共占据64盘块
//盘块号不用管，只需要记住i节点的创建顺序即可
//读写信号量宽度
#define SIG_SIZE 4096
#define disksize 2400 * 512//磁盘大小
#define blocksize 2400//盘块大小
#define blockcount 512 //盘块块数
#define inodesize 600//i节点大小
#define inodecount 32//i节点块数(1-4,8-32)

//超级块，结构体大小为1024，占据1盘块（反映磁盘空间总体情况）
typedef struct {
	//磁盘大小
	//盘块大小
	//盘块块数
	//i节点大小
	//i节点块数
	int useddisk;//已用磁盘空间
	int remaindisk;//剩余磁盘空间
	int usedblock;//已用盘块数量
	int remainblock;//剩余盘块数量
	int usedinode;//已用i节点数量
	int remaininode;//剩余i节点数量

	int block_inodemap[blockcount + inodecount];//块点位示图
												//int inodemap[inodecount];//点位示图
}SuperBlock;

//未设置目录项，i节点含全部信息
//i节点（文件信息详细记录）
typedef struct {
	int x;//文件内部标识符
	string name;//文件名
	int size;//文件大小
	string user_name;//文件所有者
	int type;//文件类型（0为目录，1为文件）
	int user_quanxian;//其他用户权限 (r=2,w=1)
	int size_num;//占用盘块数量 (0为全部共享)
	int address[N];//随机存取存放地址（可不连续存储）
	int father;//父目录内部标识符

	time_t atime;//访问时间,每次查看文件内容的时候会更新
	time_t mtime;//修改时间,只有修改了文件的内容，才会更新文件的mtime
	time_t ctime;//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime
}INode;

//用户（为方便调用，而且占存储空间较小，将其放入内存）
typedef struct {
	string name; //用户名
	int password; //密码(哈希加密)
	int UID;//用户组号（具有相同组号的具有相同权限）
			//（初始化：1为root，2为group1，0为其他用户）
}User;

string disk_name = "disk.dat";//虚拟磁盘文件名称
int curdir;//当前目录
int path[0xFF];//路径记录
char curpath[0xFF];//当前地址字符串
string key;
char cmd[0xFF];//命令
char cmd1[0xF];//命令1
char cmd2[0xFF];//命令2
char cmd3[0xFF];//命令3
string cmd11;
string cmd111;
int cmd22;


//mutex mu;//锁
SuperBlock superblock;//超级块（1块block）
INode inode[inodecount];//节点数组（64块block）
User user;//当前的用户

void head();//系统头部
void cmdhead();//命令头部
void msg(const char* str);//输出统一格式
unsigned int BKDRHash(char* key, unsigned int len);//哈希处理密码
void star(char *key);//密码加*处理
int login();//用户登录
void useradd();//添加用户
int judge();//判断yesno
int addfloat(int x, int y);//小数进位
void WriteSuperBlock();//超级块写入文件
void ReadSuperBlock();//文件读取超级块
void Writedata(int Index, char *content);//数据块写入文件
void Readdata(int Index, char *content);//文件读取数据块
void showsb();//输出磁盘信息
void init_sb();//初始化超级块
void WriteINode(int Index, INode &inode);//i节点写入文件
void ReadINode(int Index, INode &inode);//文件读取i节点
void init_id();//初始化i节点
void init_root();//初始化根目录
void diskloading();//载入磁盘
void diskupdating();//更新磁盘
void format();//格式化磁盘
void Backup();//备份磁盘文件
void recovery();//恢复磁盘文件信息
void help();//系统支持命令功能介绍
void pwd();//显示当前路径(绝对路径)
int bianli(string name, int curdir1);//寻找符合条件的i节点
int UID(int curdir1);//判断组群用户(返回组群号)
void cd(char *curpath1);//切换当前路径(支持绝对路径和相对路径)
int chmod(string config, int quanxian);//更改当前目录下文件权限
int get_inode();//申请i节点
void free_inode(int i);//释放i节点
int get_block();//申请盘块
void free_block(int i);// 释放盘块
int mkdir(string name);//创建当前目录下子目录
void createfile(string config);//创建子文件
void time();//显示当前时间
void _time(time_t &m_time);//时间转化
void dir();//显示当前目录下的信息与文件
void exit();//退出系统命令
void more(string name);//显示当前路径下文件内容
int rm(string name, int mulu);//删除当前目录下某个文件
int rmdir(string name, int mulu);//删除当前路径下某个目录
void copy(string name, char *curpath1);//复制当前文件到另一路径
void rename(string name, string name1);//当前路径下文件或者目录重命名
void xcopy(string name, char *curpath1);//复制目录，从一个路径到另一个路径(和删除目录类似，需要递归删除)
void import(char *curpath1, int mulu);//从本地导入文件
void find(char *curpath1);//寻找文件中的一段字符
void  attrib(string name);//显示当前目录下文件权限
void input();//输入函数		 
void charstring(char *config);//char转string类型
void charstring1(char *config);//char转string类型
void intstring(char *config);//char转int类型
void ver();//版本信息
void choice();//命令选择
void flush();//清空命令
int BF(char S[], char T[]);//BF字符串匹配
void pexport(char *curpath1, char *curpath2);

//文件映射是一种实现进程间单向或双向通信的机制。
//它允许两个或多个本地进程间相互通信。为了共享文件或内存，
//所有的进程必须使用相同的文件映射的名字或是句柄。
char szBuffer[] = "signal";
HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SIG_SIZE, (LPCWSTR)szBuffer);
int *signal = (int *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SIG_SIZE);

//主函数
int main()
{
	head();//系统头部
	login();//登录
	while (1)
	{
		input();
		choice();
		flush();//命令缓存
	}  
	system("pause");
	return 0;
}

//有更改磁盘的操作一定要重新载入一遍!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//有更改磁盘的操作一定要重新载入一遍!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//*******************************************************
//系统头部（系统简介）
void head() {
	cout << "--------------------------------------------------" << endl;
	cout << "             Virtual  File  System" << endl;
	cout << "--------------------------------------------------" << endl;
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);//设置绿色
	cout << " *       *       *****       *****        *******" << endl;
	cout << "   *   *       *           *       *     *" << endl;
	cout << "     *        *           *         *     *******" << endl;
	cout << "     *         *           *       *             *" << endl;
	cout << "     *           *****       *****        ******* " << endl;
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);//设置白色
	cout << "                                        Ver.1.1.0" << endl;
	cout << "                                  Directed by M.W." << endl;
	cout << "--------------------------------------------------" << endl;
	cout << "--------------------------------------------------" << endl;
}

//命令头部
void cmdhead() {
	//cout << "test"<< endl;
	cout << endl << "[" << user.name << "@localhost" << curpath << "]#";
	//printf("\n[%s@localhost %s]# ",user.name, inode[curdir].name);
}
//输出统一格式

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

//哈希处理密码
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

//密码加*处理
void star(char *key)
{
	char *p;
	p = key;
	while (*p = _getch())
	{
		if (*p == 0x0d) //回车
		{
			*p = '\0'; //将输入的回车键转换成空格
			break;
		}
		else if (*p == 8)//退格
		{
			cout << char(8) << " " << char(8);//退格删除
			p--;
		}
		else {
			printf("*");   //将输入的密码以"*"号显示
			p++;
		}
	}

}

//用户登录
int login() {
	string user_name;
	int user_password;//(加密后)
	char key[100];
	int count = 0;
	int flag = 0;

	do {
		fstream myfile("user.txt", ios::in | ios::out);//打开文件
		if (!myfile.is_open())
		{
			printf("\nCan't open file %s.\n", "user.txt");
			printf("This filesystem not exist!\n");
			system("pause");
			exit(0);
		}

		msg("请输入登陆信息");
		printf("Username:");
		cin >> user_name;
		printf("Password:");
		star(key);
		//记录key长度
		int i = 0;
		while (key[i] != '\0')
		{
			i++;
		}
		user_password = BKDRHash(key, i);
		while (!myfile.eof())//没到文件尾循环
		{
			myfile >> user.name;
			myfile >> user.password;
			myfile >> user.UID;
			//cout << user_name;
			//cout << user_password;
			//cout << user.name;
			//cout << user.password;
			//用户名和密码都正确
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
		if (flag == 0)//密码用户名不正确
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
	diskloading();//载入磁盘信息
	cin.ignore();
	return 0;
}

//添加用户
void useradd()
{
	//检查用户权限
	if (user.UID != 1)
	{
		printf("权限不足....");
		return;
	}
	User user1;//增加的用户
	fstream myfile("user.txt", ios::in | ios::out | ios::app);//打开文件
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	while (1)
	{
		msg("请输入新用户信息");
		string user_name;
		string user_UID;
		char key1[100];
		char key2[100];
		int flag = 0;
		printf("Username:");
		cin >> user_name;

		while (!myfile.eof())//没到文件尾循环
		{
			myfile >> user1.name;
			myfile >> user1.password;
			myfile >> user1.UID;
			myfile.close();
			if (user1.name == user_name)
			{
				cout << "该用户已存在" << endl;
				flag = 1;
				break;
			}
		}
		if (flag == 0)//可以新建用户
		{
			while (true) {
				printf("password:");
				star(key1);
				printf("\n");
				printf("password again:");
				star(key2);
				if (!strcmp(key1, key2)) break;
				printf("\n密码输入不一致，请重新输入\n");
			}
			printf("\n普通用户0、管理员1、组群请输入组群号......\n");
			printf("UID:");
			cin >> user_UID;

			//记录key长度
			int i = 0;
			while (key1[i] != '\0')
			{
				i++;
			}
			int user_password = BKDRHash(key1, i);
			//cout << user_name;
			//cout << user_password;
			fstream myfile("user.txt", ios::in | ios::out | ios::app);//打开文件
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

//判断yesno
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

//小数进位
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

//超级块写入文件
void WriteSuperBlock()
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//文件读取超级块
void ReadSuperBlock()
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//输出磁盘信息
void showsb()
{
	ReadSuperBlock();
	cout << "已用空间：" << superblock.useddisk << "字节" << endl;
	cout << "剩余空间：" << superblock.remaindisk << "字节" << endl;
	cout << "已用盘块：" << superblock.usedblock << endl;
	cout << "剩余盘块：" << superblock.remainblock << endl;
	cout << "已用节点：" << superblock.usedinode << endl;//ctime将时间转换为可显示的格式输出
	cout << "剩余节点：" << superblock.remaininode << endl;
	////块点位示图
	//for (int i = 0; i<512; i++)//超级块+i节点
	//{
	//	cout<<superblock.block_inodemap[i] ;
	//}
	//cout << endl;
	//cout << superblock.block_inodemap[512];
	//cout << endl;
	//for (int i = 513; i<592; i++)//超级块+i节点
	//{
	//	cout << superblock.block_inodemap[i];
	//}
}

//初始化超级块
void init_sb()
{
	superblock.useddisk = 2400 * 9;//已用磁盘空间
	superblock.remaindisk = disksize - superblock.useddisk;//剩余磁盘空间
	superblock.usedblock = 9;//已用盘块数量
	superblock.remainblock = blockcount - superblock.usedblock;//剩余盘块数量
	superblock.usedinode = 1;//已用i节点数量(根目录占据一个节点)
	superblock.remaininode = 31;//剩余i节点数量

								//块位示图
	for (int i = 0; i<3; i++)//超级块+i节点
	{
		superblock.block_inodemap[i] = 1;
	}
	for (int i = 2; i<blockcount; i++)
	{
		superblock.block_inodemap[i] = 0;
	}
	//点位示图
	superblock.block_inodemap[blockcount] = 1;//根目录
	for (int i = 513; i<592; i++)
	{
		superblock.block_inodemap[i] = 0;
	}

	WriteSuperBlock();
}

//i节点写入文件
void WriteINode(int Index, INode &inode)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//数据块写入文件
void Writedata(int Index, char *content)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//文件读取数据块
void Readdata(int Index, char *content)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//文件读取i节点
void ReadINode(int Index, INode &inode)
{
	while (*signal == 1) {
		Sleep(500);
	}
	fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
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

//初始化i节点
void init_id()
{
	for (int i = 0; i < inodecount; i++)
	{
		inode[i].x = -1;//文件内部标识符
		inode[i].name = "1";//文件名
		inode[i].size = 0;//文件大小
		inode[i].user_name = "1";//文件所有者
		inode[i].type = -1;//文件类型（0为目录，1为文件）
		inode[i].user_quanxian = 3;//rw
		inode[i].size_num = addfloat(inode[i].size, blocksize);//占用盘块数量
		inode[i].address[0] = -1;//随机存取存放数据块地址（可不连续存储）
								 //inode[i].y;//文件读写权限（r，w，rw）
		inode[i].father = -1;//父目录内部标识符

							 //inode[i].atime;//访问时间,每次查看文件内容的时候会更新
							 //inode[i].mtime;//修改时间,只有修改了文件的内容，才会更新文件的mtime
							 //inode[i].ctime;//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

		WriteINode(i, inode[i]);//写入文件
	}
}

//初始化根目录(申请节点但是无数据)
void init_root()
{
	curdir = 0;//当前目录为根目录
	path[0] = 0;
	curpath[0] = '\\';
	curpath[1] = '\0';//根路径

	inode[0].x = 0;//文件内部标识符
	inode[0].name = ".";//文件名
	inode[0].size = 0;//文件大小
	inode[0].user_name = "root";//文件所有者
	inode[0].type = 0;//文件类型（0为目录，1为文件）
	inode[0].user_quanxian = 3;//rw
							   //inode[0].user_quanxian[N];//用户访问权限
	inode[0].size_num = addfloat(inode[0].size, blocksize);//占用盘块数量
														   //inode[0].address[N];//随机存取存放地址（可不连续存储）
	inode[0].father = -2;//父目录内部标识符（root特有路径）

	time(&inode[0].atime);//访问时间,每次查看文件内容的时候会更新
	time(&inode[0].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
	time(&inode[0].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

	WriteINode(0, inode[0]);//写入文件
}

//载入磁盘
void diskloading()
{
	cout << "--------------------------------------------------" << endl;
	printf("Please Waiting ,Checking disk integrity...\n");
	//打开磁盘读入数据
	Sleep(1000);
	printf("Please Waiting ,Reading system data...\n");
	cout << "--------------------------------------------------" << endl;
	//载入超级块
	ReadSuperBlock();
	//载入i节点
	for (int i = 0; i<inodecount; i++)
	{
		ReadINode(i, inode[i]);
	}
	//测试输出i节点(succeed!)
	//cout << inode[0].name << endl;

	//根目录
	curdir = 0;//当前目录为根目录
	path[0] = 0;
	path[1] = -1;
	curpath[0] = '\/';//根路径
	curpath[1] = '\0';//根路径
}

//更新磁盘
void diskupdating()
{
	printf("Please Waiting ,Checking disk integrity...\n");
	//打开磁盘读入数据

	printf("Please Waiting ,Reading system data...\n");
	cout << "--------------------------------------------------" << endl;
	//载入超级块
	ReadSuperBlock();
	//载入i节点
	for (int i = 0; i<inodecount; i++)
	{
		ReadINode(i, inode[i]);
	}
	//测试输出i节点(succeed!)
	//cout << inode[0].name << endl;

}

//格式化磁盘
void format()
{
	printf("Are you sure you want to format the disk? \n The formatted file will not be recoverable.（Yes/No）");
	if (!judge())
		return;
	remove("disk.dat");
	ofstream myfile("disk.dat", fstream::out);
	myfile.close();
	//初始化超级块并写入二进制文件
	//cout <<"test" << endl;
	init_sb();
	//showsb();
	//初始化i节点并写入二进制文件
	//cout << "test" << endl;
	init_id();
	//建立根目录并写入二进制文件(父目录要有特殊标记-2)
	//cout << "test" << endl;
	init_root();

	//格式化剩余的磁盘空间(数据块)
	myfile.open("disk.dat", ios::in | ios::out | ios::binary);//打开文件
	if (!myfile.is_open())
	{
		printf("\nCan't open file %s.\n", "user.txt");
		printf("This filesystem not exist!\n");
		system("pause");
		exit(0);
	}
	myfile.seekp(2400 * 9);//定位到数据块起始地址
	myfile << setw(disksize - 2400 * 9) << '\0';
	myfile.close();
	printf("Filesystem created successful.\n");
	diskloading();//重新载入磁盘
	login();//登录
}

//备份磁盘文件（帮助恢复）
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
	printf("备份完成\n");
	*signal = 0;
}

//恢复磁盘文件信息
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
	printf("恢复完成\n");

	*signal = 0;
}

//显示当前路径(绝对路径)
void pwd()
{
	printf("当前路径: %s\n", curpath);
}

//寻找符合条件的i节点
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

//判断组群用户(返回组群号)
int UID(int curdir1)
{
	fstream myfile("user.txt", ios::in | ios::out);//打开文件
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
	while (!myfile.eof())//没到文件尾循环
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

//切换当前路径(支持绝对路径和相对路径)
void cd(char *curpath1)
{
	//char curpath1[0xFF];
	int curdir1 = 0;
	int flag = 0;
	//printf("当前路径: %s\n", curpath);
	//printf("path: ");
	//scanf("%s", curpath1);
	//绝对路径（绝对路径是从/（也被称为根目录）开始的）
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
					curdir1 = bianli(temp, curdir1);//当前目录记录
					time(&inode[curdir1].atime);//更改路径上目录的访问时间
												//cout << curdir1;
					path[j] = curdir1;//路径节点记录
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
		if (flag == 0)//未出错赋值
		{
			strcpy(curpath, curpath1);
			curdir = curdir1;
			//cout << curdir;
		}
	}

	//相对路径（相对路径是以 . 或 .. 开始的）
	else if (curpath1[0] == '.'&&curpath1[1] != '.')//(回到根目录)
	{
		curdir = 0;
		path[0] = 0;
		path[1] = '\0';
		curpath[0] = '\/';
		curpath[1] = '\0';
	}
	else if (curpath1[0] == '.'&&curpath1[1] == '.')//(回到上级目录)
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

//更改当前目录下文件权限(chmod 3 file)系统默认权限为3，需自己加密更改！！！！！！！！！！
//仅支持数字更改其他用户权限,r=1,w=2
int chmod(string config, int quanxian)
{
	int flag = 0;
	//找到该文件的内部标识号
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == config)
		{
			flag = 1;
			//检查用户权限(root或者文件所有者)
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

//申请i节点
int get_inode()
{
	for (int i = 512; i<592; i++)
		if (superblock.block_inodemap[i] == 0)	//第i个结点可被申请
		{
			superblock.block_inodemap[i] = 1;
			superblock.remaininode = superblock.remaininode - 1;
			superblock.usedinode = superblock.usedinode + 1;

			WriteSuperBlock();

			return i;
		}
	return -1;
}

//释放i节点
void free_inode(int i)
{
	superblock.block_inodemap[i] = 0;
	superblock.remaininode = superblock.remaininode + 1;
	superblock.usedinode = superblock.usedinode - 1;
	WriteSuperBlock();
}

//申请盘块
int get_block()
{
	for (int i = 2; i<512; i++)
		if (superblock.block_inodemap[i] == 0)	//第i个盘块可被申请
		{
			superblock.block_inodemap[i] = 1;
			superblock.remainblock = superblock.remainblock - 1;
			superblock.usedblock = superblock.usedblock + 1;
			superblock.useddisk = 2400 * superblock.usedblock;//已用磁盘空间
			superblock.remaindisk = disksize - superblock.useddisk;//剩余磁盘空间

			WriteSuperBlock();
			return i;
		}
	return -1;
}

// 释放盘块
void free_block(int i)
{
	superblock.block_inodemap[i] = 0;
	superblock.remainblock = superblock.remainblock + 1;
	superblock.usedblock = superblock.usedblock - 1;
	superblock.useddisk = superblock.useddisk - 2400;//已用磁盘空间
	superblock.remaindisk = disksize - superblock.useddisk;//剩余磁盘空间

	WriteSuperBlock();
}

//创建当前目录下子目录
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
		inode[i].x = i;//文件内部标识符
		inode[i].name = name;//文件名
		inode[i].size = 0;//文件大小
		inode[i].user_name = user.name;//文件所有者
		inode[i].type = 0;//文件类型（0为目录，1为文件）

		inode[i].size_num = addfloat(inode[i].size, blocksize);//占用盘块数量
		inode[i].user_quanxian = 3;//其他用户权限
		inode[i].father = curdir;//父目录内部标识符
		time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
		time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
		time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime
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

//创建子文件
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
			//先分配一个盘块，如果不够再分配一个盘块
			//如果剩余数据块内存都不够则不能生成
			//先把内容存到缓冲区判断
			content = new char[superblock.remainblock * blocksize];//剩余磁盘总空间
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
				//申请盘块
				//cout << length;
				//cout << addfloat(length, blocksize);
				for (int j = 0; j < addfloat(length, blocksize); j++)
				{
					//cout << addfloat(length, blocksize);
					inode[i].size_num = j + 1;//占用盘块数量
					inode[i].address[j] = get_block();//随机存取存放地址（可不连续存储）
													  //cout << inode[i].size_num;
				}
				//将字符串分解！
				char content1[1200][1024];
				for (int j = 0; j < inode[i].size_num - 1; j++)
				{
					strncpy(content1[j], content, 1024);
					for (int ii = 0; ii < strlen(content); ii++) {
						content[ii] = content[ii + 1024];
					}
				}
				strcpy(content1[inode[i].size_num - 1], content);

				//数据块写入文件()

				for (int j = 0; j < inode[i].size_num; j++)
				{
					Writedata(inode[i].address[j], content1[j]);
				}

				//设置文件信息
				inode[i].x = i;//文件内部标识符
				inode[i].name = config;//文件名
				inode[i].size = length;//文件大小
				inode[i].user_name = user.name;//文件所有者
				inode[i].type = 1;//文件类型（0为目录，1为文件）
				inode[i].user_quanxian = 3;
				inode[i].father = curdir;//父目录内部标识符

				time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
				time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
				time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

				WriteINode(i, inode[i]);

				int pp = inode[i].size;
				//只要不是根节点就爸爸！！！！！！！
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
			//设置文件信息
			inode[i].x = i;//文件内部标识符
			inode[i].name = config;//文件名
			inode[i].size = 0;//文件大小
			inode[i].user_name = user.name;//文件所有者
			inode[i].type = 1;//文件类型（0为目录，1为文件）
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//父目录内部标识符
			inode[i].size_num = 0;
			time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
			time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
			time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

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

//显示当前时间
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

//时间转化
void _time(time_t &m_time) {
	time_t timep = m_time;
	struct tm *p;
	if (timep < 0)
	{
		time(&timep);
		printf("Unlethel Time error dectected\n");
	}
	p = localtime(&timep); //取得当地时间
	printf("%02d\/%02d\/%02d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
	printf(" %02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
}

//文件大小！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
//显示当前目录下的信息与文件
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
	//该目录访问时间改变
	time(&inode[curdir].atime);
	//该文件访问时间改变
	//time(&inode[i].atime);
	WriteINode(curdir, inode[curdir]);
	//mu.unlock();
}

//退出系统命令
void exit()
{
	msg("Waiting Next Use <YCOS> 2019.06");
}

//显示当前路径下文件内容
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
	//该目录访问时间改变
	time(&inode[curdir].atime);
	WriteINode(curdir, inode[curdir]);//写入文件
									  //该文件访问时间改变
	time(&inode[i].atime);
	WriteINode(i, inode[i]);//写入文件
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

//删除当前目录下某个文件
int rm(string name, int mulu)
{
	//找到该文件
	int flag = 0;
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == mulu && inode[i].type == 1 && inode[i].name == name)
		{
			flag = i;
			break;
		}
	}

	//释放数据块,判断文件占用几个数据块
	for (int i = 0; i < inode[flag].size_num; i++)
	{
		free_block(inode[flag].address[i]);

		fstream myfile("disk.dat", ios::in | ios::out | ios::binary);//打开文件
		if (!myfile.is_open())
		{
			printf("\nCan't open file %s.\n", "disk.dat");
			printf("This filesystem not exist!\n");
			system("pause");
			exit(0);
		}
		myfile.seekp(2400 * inode[flag].address[i]);//定位到数据块起始地址
		myfile << setw(2400);
		myfile.close();
	}

	//释放i节点
	free_inode(flag + 512);
	inode[inode[flag].father].size = inode[inode[flag].father].size - inode[flag].size;
	inode[flag].x = -1;//文件内部标识符
	inode[flag].name = "";//文件名
	inode[flag].size = 0;//文件大小
	inode[flag].user_name = "";//文件所有者
	inode[flag].type = -1;//文件类型（0为目录，1为文件）
	inode[flag].user_quanxian = 3;//rw
	inode[flag].size_num = addfloat(inode[flag].size, blocksize);//占用盘块数量
	inode[flag].address[0] = -1;//随机存取存放数据块地址（可不连续存储）
								//inode[i].y;//文件读写权限（r，w，rw）
	inode[flag].father = -1;//父目录内部标识符

							//inode[i].atime;//访问时间,每次查看文件内容的时候会更新
							//inode[i].mtime;//修改时间,只有修改了文件的内容，才会更新文件的mtime
							//inode[i].ctime;//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime
	WriteINode(inode[flag].father, inode[inode[flag].father]);
	WriteINode(flag, inode[flag]);//写入文件

	time(&inode[mulu].atime);
	time(&inode[mulu].ctime);
	WriteINode(mulu, inode[mulu]);//写入文件
	diskupdating();
	return 0;
}

//删除当前路径下某个目录
int rmdir(string name, int mulu)
{
	int flag = 0;//记录要删除的目录序号

				 //遍历得到当前序号
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == mulu && inode[i].type == 0 && inode[i].name == name)
		{
			flag = i;
			break;
		}
	}

	//目录为空直接删除
	int baba[N];
	int flag1 = 0;//记录是否已被删除
	for (int i = 0; i < inodecount; i++)
	{
		if (inode[i].father == flag)
		{
			baba[flag1] = i;//需要记录当前哪些文件的爸爸是它！！！！！！！
			flag1++;

		}
	}
	//直接删除目录
	if (flag1 == 0)
	{
		inode[flag].x = -1;//文件内部标识符
		inode[flag].name = "";//文件名
		inode[flag].size = 0;//文件大小
		inode[flag].user_name = "";//文件所有者
		inode[flag].type = -1;//文件类型（0为目录，1为文件）
		inode[flag].user_quanxian = 3;//rw
		inode[flag].size_num = addfloat(inode[flag].size, blocksize);//占用盘块数量
		inode[flag].address[0] = -1;//随机存取存放数据块地址（可不连续存储）
									//inode[i].y;//文件读写权限（r，w，rw）
		inode[flag].father = -1;//父目录内部标识符

								//inode[i].atime;//访问时间,每次查看文件内容的时候会更新
								//inode[i].mtime;//修改时间,只有修改了文件的内容，才会更新文件的mtime
								//inode[i].ctime;//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

		WriteINode(flag, inode[flag]);//写入文件
		free_inode(flag + 512);//释放i节点
	}


	//目录有内容要递归删除其内容,注意数据块的删除
	else
	{
		for (int i = 0; i < flag1; i++)
		{
			//文件调用rm
			if (inode[baba[i]].type == 1)
			{
				rm(inode[baba[i]].name, inode[baba[i]].father);
			}

			//目录调用rmdir
			else if (inode[baba[i]].type == 0)
			{
				rmdir(inode[baba[i]].name, inode[baba[i]].father);
			}
		}
		inode[flag].x = -1;//文件内部标识符
		inode[flag].name = "";//文件名
		inode[flag].size = 0;//文件大小
		inode[flag].user_name = "";//文件所有者
		inode[flag].type = -1;//文件类型（0为目录，1为文件）
		inode[flag].user_quanxian = 3;//rw
		inode[flag].size_num = addfloat(inode[flag].size, blocksize);//占用盘块数量
		inode[flag].address[0] = -1;//随机存取存放数据块地址（可不连续存储）
									//inode[i].y;//文件读写权限（r，w，rw）
		inode[flag].father = -1;//父目录内部标识符

								//inode[i].atime;//访问时间,每次查看文件内容的时候会更新
								//inode[i].mtime;//修改时间,只有修改了文件的内容，才会更新文件的mtime
								//inode[i].ctime;//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

		WriteINode(flag, inode[flag]);//写入文件
		free_inode(flag + 512);//释放i节点
	}
	//该目录访问时间改变
	time(&inode[mulu].atime);
	time(&inode[mulu].ctime);
	WriteINode(mulu, inode[mulu]);//写入文件

								  //该文件访问时间改变
								  //time(&inode[i].atime);
	diskupdating();
	return 0;
}

//复制当前目录的文件到另一路径
void copy(string name, char *curpath1)
{
	//判断用户权限！！！！！！！！！！！！！
	//存在同名文件，替换/取消操作
	//不存在同名文件的复制可以单独申请节点
	int i;
	int flag = 0;
	//先找到目标文件
	for (i = 0; i < inodecount; i++)
	{
		if (inode[i].father == curdir && inode[i].name == name&& inode[i].type == 1)
		{
			flag = i;
			break;
		}
	}

	if (flag == 0) cout << "文件不存在！" << endl;

	else//可以复制
	{
		cd(curpath1);//找到复制路径


					 //申请节点创建子文件//文件内容复制
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
			//复制i节点信息
			inode[i].x = i;//文件内部标识符
						   //cout<< name;
			inode[i].name = name;//文件名
								 //Sleep(2000);	
			inode[i].size = inode[flag].size;//文件大小

			inode[i].user_name = user.name;//文件所有者
			inode[i].type = 1;//文件类型（0为目录，1为文件）
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//父目录内部标识符

			inode[i].size_num = inode[flag].size_num;//占用盘块数量
			if (superblock.remainblock < inode[flag].size_num)
			{
				cout << "存储空间不足......" << endl;
				return;
			}
			for (int j = 0; j < inode[flag].size_num; j++)
			{
				inode[i].address[j] = get_block();
			}

			time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
			time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
			time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

			WriteINode(i, inode[i]);
			int pp = inode[i].size;
			//只要不是根节点就爸爸！！！！！！！
			if (inode[i].father != -2)
			{
				inode[inode[i].father].size = inode[inode[i].father].size + pp;
				WriteINode(inode[i].father, inode[inode[i].father]);
				i = inode[i].father;
			}
			inode[inode[i].father].size = inode[inode[i].father].size + pp;
			WriteINode(inode[i].father, inode[inode[i].father]);


			//文件内容写入数据块
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

//移动当前目录文件
void move(string name, char *curpath1)
{
	//记录当前路径
	int curdir1 = curdir;
	char curpath0[0xFF];
	strcpy(curpath0, curpath);
	//调用copy到目标路径
	copy(name, curpath1);
	cd(curpath0);
	//rm删除本文件
	rm(name, curdir1);
}

//移动当前目录目录
void movedir(string name, char *curpath1)
{
	//记录当前路径
	int curdir1 = curdir;
	char curpath0[0xFF];
	strcpy(curpath0, curpath);
	//调用copy到目标路径
	xcopy(name, curpath1);
	cd(curpath0);
	//rm删除本文件
	rmdir(name, curdir1);
}

//当前路径下文件或者目录重命名
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
		WriteINode(i, inode[i]);//写入文件
	}
	else
	{
		cout << "File not found......" << endl;
	}
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!为什么这里的堆栈异常
//复制目录从一个路径到另一个路径(和删除目录类似，需要递归删除)
void xcopy(string name, char *curpath1)
{
	char nowpath[100];
	strcpy(nowpath, curpath);
	int i;
	int flag = -1;
	//先找到目标目录
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
		cout << "文件不存在！" << endl;
		return;
	}

	else//可以复制
	{
		cd(curpath1);//找到复制路径

					 //目录为空直接复制节点即可
		int baba[N];
		int flag1 = 0;//记录是否已被复制
		for (int i = 0; i < inodecount; i++)
		{
			if (inode[i].father == flag)
			{
				baba[flag1] = i;//需要记录当前哪些文件的爸爸是它！！！！！！！
				flag1++;
			}
		}

		i = get_inode() - 512;
		if (i != -1 - 512)
		{
			//复制i节点信息
			inode[i].x = i;//文件内部标识符
						   //cout << inode[flag].name;
						   //cout << inode[i].name;
			inode[i].name = inode[flag].name;//文件名
			inode[i].size = inode[flag].size;//文件大小
			inode[i].user_name = user.name;//文件所有者
			inode[i].type = 0;//文件类型（0为目录，1为文件）
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//父目录内部标识符
									 //inode[curdir].size = inode[curdir].size + inode[flag].size;
			inode[i].size_num = inode[flag].size_num;//占用盘块数量
			if (superblock.remainblock < inode[flag].size_num)
			{
				cout << "存储空间不足......" << endl;
				return;
			}
			for (int j = 0; j < inode[flag].size_num; j++)
			{
				inode[i].address[j] = get_block();
			}

			time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
			time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
			time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

			WriteINode(i, inode[i]);
			int ff = i;
			//WriteINode(curdir, inode[curdir]);
			int pp = inode[i].size;
			//只要不是根节点就爸爸！！！！！！！
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

		if (flag1 != 0)//有孩子
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
				//文件调用copy
				if (inode[baba[i]].type == 1)
				{
					cd(nowpath);
					copy(inode[baba[i]].name, curpath11);
					inode[curdir].size = inode[curdir].size - inode[baba[i]].size;
					WriteINode(curdir, inode[curdir]);
				}

				//目录调用xcopy
				else if (inode[baba[i]].type == 0)
				{
					cd(nowpath);
					xcopy(inode[baba[i]].name, curpath11);
					//inode[curdir].size = inode[curdir].size - inode[baba[i]].size;
					int ff = baba[i];
					int pp = inode[baba[i]].size;
					//只要不是根节点就爸爸！！！！！！！
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

//从本地导入文件
void import(char *curpath1, int mulu)
{

	char *data;
	int filesize;
	// 打开文件“d:\a.txt”
	FILE *file;
	if ((file = fopen(curpath1, "r")) == NULL)
	{
		printf("Can't open %s\n", curpath1);
		//Sleep(2000);
		exit(0);
	}
	// 获得文件大小
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	// 分配内存
	data = (char*)malloc(filesize + 1);
	// 读取文件
	fread(data, sizeof(char), filesize, file);
	data[filesize] = 0;
	fclose(file);
	// 输出内容（你想对内容干什么都可以了）
	//printf("%s", data);

	//解析文件名
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
	//申请i节点
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
		inode[i].name = p;//文件名
		inode[i].user_name = user.name;//文件所有者
		cin.clear();
		//fflush(stdin);
		char *content = '\0';
		int length = 0;
		//cout << "Do you want to enter the contents of the file?(Y\\N)";
		//cout << "Please enter the contents of the file:";
		//先分配一个盘块，如果不够再分配一个盘块
		//如果剩余数据块内存都不够则不能生成
		//先把内容存到缓冲区判断
		content = new char[superblock.remainblock * blocksize];//剩余磁盘总空间
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
			//申请盘块
			//cout << length;
			//cout << addfloat(length, blocksize);
			for (int j = 0; j < addfloat(length, blocksize); j++)
			{
				//cout << addfloat(length, blocksize);
				inode[i].size_num = j + 1;//占用盘块数量
				inode[i].address[j] = get_block();//随机存取存放地址（可不连续存储）
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
			//数据块写入文件()

			for (int j = 0; j < inode[i].size_num; j++)
			{
				Writedata(inode[i].address[j], content1[j]);
			}

			//设置文件信息
			inode[i].x = i;//文件内部标识符
						   //
			inode[i].size = length;//文件大小

			inode[i].type = 1;//文件类型（0为目录，1为文件）
			inode[i].user_quanxian = 3;
			inode[i].father = curdir;//父目录内部标识符

			time(&inode[i].atime);//访问时间,每次查看文件内容的时候会更新
			time(&inode[i].mtime);//修改时间,只有修改了文件的内容，才会更新文件的mtime
			time(&inode[i].ctime);//修改时间,对文件更名，修改文件的属主等操作任何操作都更新ctime

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

//BF字符串匹配
int BF(char S[], char T[]) {
	int i = 0, j = 0;//i指向S，j指向T
	int length_s = strlen(S), length_t = strlen(T);
	//printf("%d  %d\n",length_s,length_t);
	while (i<length_s&&j<length_t) {
		//printf("%c %c\n",S[i],T[j]);
		if (S[i] == T[j]) {
			i++;
			j++;
		}
		else {
			i = i - j + 1;//i回溯到下一个元素开始匹配 
			j = 0;
		}
	}
	//匹配结束
	if (j >= length_t) {
		i = i - j;//i回溯到当前匹配的开始地方 
		return i;
	}
	return 0;

}

//寻找文件中的一段字符
void find(char *curpath1)
{
	int fff = 1;
	int ffff = 1;
	//返回该数据块对应的文件节点
	for (int i = 2; i<blockcount; i++)//遍历所有数据块
	{
		fff = 1;
		char *content = new char[superblock.remainblock * blocksize];
		strcpy(content, "\0");
		Readdata(i, content);
		string father1;
		if (BF(content, curpath1) != 0)
		{//找到！！！！！！！！！！！！！！！！！！
			for (int ii = 0; ii < inodecount; ii++)
			{//找节点！！！！！！！1
				for (int iii = 0; iii < inode[ii].size_num; iii++)
				{//找节点中数据块！！！！！！！
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

//显示当前目录下文件对于其他用户的所有属性
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
		cout << "当前目录下无文件..." << endl;
	}
}

//输入函数
void input()
{
	cmdhead();

	cin.getline(cmd, 250);

	//拆分第一部分
	int i = 0;
	while ((cmd[i] != '\0') && (cmd[i] != ' '))
	{
		cmd1[i] = cmd[i];
		i++;
	}
	cmd1[i] = '\0';
	//拆分第二部分
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
	//拆分第三部分
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

//char转string类型
void charstring(char *config)
{
	cmd11 = "";
	for (int i = 0; i < strlen(config); i++)
	{
		cmd11 += config[i];
	}
	//cmd11 += '\0';
}
//char转string类型
void charstring1(char *config)
{
	cmd111 = "";
	for (int i = 0; i < strlen(config); i++)
	{
		cmd111 += config[i];
	}
	//cmd111 += '\0';
}

//char转int类型
void intstring(char *config)
{
	cmd22 = (int)(config - 48);
}

//版本信息
void ver()
{
	cout << "ver.1.1.0  Made by Y.C.Q. Directed by M.W." << endl;
}

//系统支持命令功能介绍
void help()
{
	printf("系统支持命令功能介绍:\n\n");
	printf("(30+3条指令)\n\n");
	printf("命令操作符      命令格式               作用介绍\n");
	printf("cls            cls                   清除屏幕\n");
	printf("help           help                  版本信息与帮助界面\n");
	printf("login          login                 登录一个新的账户\n");
	printf("useradd        useradd               添加一个新的账户\n");
	printf("exit           exit                  结束本次服务\n");
	printf("format         format                格式化虚拟磁盘\n");
	printf("showsb         showsb                显示磁盘使用情况\n");
	printf("time           time                  显示当前系统时间\n");
	printf("ver            ver                   显示当前系统版本\n");
	printf("pwd            pwd                   显示当前路径\n");
	printf("mkdir          mkdir dirname         创建一个新目录\n");
	printf("dir\\ls         dir\\ls                显示当前目录下的信息与文件\n");
	printf("cd             cd path               切换路径\n");
	printf("mkfile         mkfile filename       创建一个新文件\n");
	printf("rmdir          rmdir dirname         删除当前路径下一个目录（递归）\n");
	printf("rm\\del         rmdir\\del filename    删除当前路径下一个文件\n");
	printf("cat\\more       cat\\more filename     显示文件内容\n");
	printf("rename         rename name           重命名一个文件或目录\n");
	printf("chmod          chmod name mode       修改文件或目录的访问权限\n");
	printf("attrib         attrib                显示当前目录下所有文件目录属性\n");
	printf("find           find txt              寻找包含指定文本的文件\n");
	printf("copy           copy filename path    将一个文件拷贝到另一目录\n");
	printf("xcopy          xcopy dirname path    将一个目录拷贝到另一目录（递归）\n");
	printf("export         export filename path  将虚拟磁盘内容导出到物理磁盘\n");
	printf("import         import path           将物理磁盘导入到虚拟磁盘内容\n");
	printf("backup         backup                备份虚拟磁盘内容\n");
	printf("recover        recover               恢复虚拟磁盘内容\n");
	printf("diskupdate     diskupdate            更新虚拟磁盘\n");
	printf("move           move filename path    移动文件至某个路径\n");
	printf("movedir        movedir dirname path  移动目录至某个路径\n");
}

//命令选择
void choice()
{
	//清屏
	if (!strcmp(cmd1, "cls"))
	{
		system("cls");
		head();
	}
	//帮助文档
	else if (!strcmp(cmd1, "help"))
	{
		help();
	}
	//重新登录
	else if (!strcmp(cmd1, "login"))
	{
		login();
	}
	//添加用户
	else if (!strcmp(cmd1, "useradd"))
	{
		useradd();
	}
	//更改权限
	else if (!strcmp(cmd1, "chmod"))
	{
		charstring(cmd2);
		intstring(cmd3);
		chmod(cmd11, cmd22);
	}
	//退出系统
	else if (!strcmp(cmd1, "exit"))
	{
		exit();
		Sleep(2000);
		exit(0);
	}
	//格式化磁盘
	else if (!strcmp(cmd1, "format"))
	{
		format();
	}
	//更新磁盘
	else if (!strcmp(cmd1, "diskupdate"))
	{
		diskupdating();
	}
	//磁盘使用情况
	else if (!strcmp(cmd1, "showsb"))
	{
		showsb();
	}
	//显示时间
	else if (!strcmp(cmd1, "time"))
	{
		time();
	}
	//显示版本
	else if (!strcmp(cmd1, "ver"))
	{
		ver();
	}
	//显示子目录/文件
	else if (!strcmp(cmd1, "dir"))
	{
		dir();
	}
	//显示子目录/文件(同dir)
	else if (!strcmp(cmd1, "ls"))
	{
		dir();
	}
	//显示当前路径
	else if (!strcmp(cmd1, "pwd"))
	{
		pwd();
	}
	//切换路径
	else if (!strcmp(cmd1, "cd"))
	{
		cd(cmd2);
	}
	//创建目录
	else if (!strcmp(cmd1, "mkdir"))
	{
		charstring(cmd2);
		mkdir(cmd11);
	}
	//创建文件
	else if (!strcmp(cmd1, "mkfile"))
	{
		charstring(cmd2);
		createfile(cmd11);
	}
	//删除目录（含递归）
	else if (!strcmp(cmd1, "rmdir"))
	{
		charstring(cmd2);
		rmdir(cmd11, curdir);
	}
	//删除文件
	else if (!strcmp(cmd1, "rm"))
	{
		charstring(cmd2);
		rm(cmd11, curdir);
	}
	//删除文件(同rm)
	else if (!strcmp(cmd1, "del"))
	{
		rm(cmd2, curdir);
	}
	//重命名文件
	else if (!strcmp(cmd1, "rename"))
	{
		charstring(cmd2);
		charstring1(cmd3);
		rename(cmd11, cmd111);
	}
	//显示文件内容
	else if (!strcmp(cmd1, "more"))
	{
		charstring(cmd2);
		more(cmd11);
	}
	//显示文件内容（同more）
	else if (!strcmp(cmd1, "cat"))
	{
		more(cmd2);
	}
	//复制文件
	else if (!strcmp(cmd1, "copy"))
	{
		charstring(cmd2);
		copy(cmd11, cmd3);
	}
	//复制目录（含递归）
	else if (!strcmp(cmd1, "xcopy"))
	{
		charstring(cmd2);
		xcopy(cmd11, cmd3);
	}
	//移动文件
	else if (!strcmp(cmd1, "move"))
	{
		charstring(cmd2);
		move(cmd11, cmd3);
	}
	//移动目录
	else if (!strcmp(cmd1, "movedir"))
	{
		charstring(cmd2);
		movedir(cmd11, cmd3);
	}
	//从本地磁盘复制内容到虚拟的磁盘驱动器
	else if (!strcmp(cmd1, "import"))
	{
		import(cmd2, curdir);
	}
	//从虚拟的磁盘驱动器复制内容到本地磁盘
	else if (!strcmp(cmd1, "export"))
	{
		pexport(cmd2, cmd3);
	}
	//寻找文件中的一段字符
	else if (!strcmp(cmd1, "find"))
	{
		find(cmd2);
	}
	//显示当前目录下文件权限
	else if (!strcmp(cmd1, "attrib"))
	{
		attrib(cmd2);
	}
	//备份磁盘
	else if (!strcmp(cmd1, "backup"))
	{
		Backup();
	}
	//恢复磁盘
	else if (!strcmp(cmd1, "recover"))
	{
		recovery();
	}
	else
	{
		printf("Invalid command........\n");
	}
}

//清空命令
void flush()
{
	strcpy(cmd, "\0");
	strcpy(cmd1, "\0");
	strcpy(cmd2, "\0");
	strcpy(cmd3, "\0");
	cmd11 = "";
	cmd22 = 0;
}

//将当前目录下的a.txt导出到本地C盘(不知是否需要删除本文件)
void pexport(char *curpath1, char *curpath2)
{
	//在本地c盘建立同名文件
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
	//获取本文件内容将其写入c盘文件
	int i;
	int flag = -1;
	//先找到目标文件
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
		cout << "文件不存在！" << endl;
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

	//写入文件
	fwrite(content, strlen(content), 1, file);
	fclose(file);

}

