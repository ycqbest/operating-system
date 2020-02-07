#define _CRT_SECURE_NO_WARNINGS

//每一层索引的最大节点数
#define Node_Per_Sector 0xF

//more指令一次显示文件开头的多少内容
#define HOW_MUCH_MORE 0xFF

//读写信号量宽度
#define SIG_SIZE 4U

#include <iostream>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <Windows.h>


//int *pt_SIGNAL;

//初始化读写信号量
char szName[] = "SignalVariable";
HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL, PAGE_READWRITE,0,SIG_SIZE,(LPCWSTR)szName);
int *pt_SIGNAL = (int *)MapViewOfFile(hMapFile,FILE_MAP_ALL_ACCESS,0,0,SIG_SIZE);


using std::cin;
using std::cout;
using std::endl;

char cmd[0xFF];
char key[0xF];
char address[0xFF];
char mode;
char Secondary_oppend[0xFF];
int cur;//当前文件夹地址
char Last[0xF];//最后一段地址名
int L_add;//最后一段地址
char curadd[0xFF];//维护一当前地址字符串

int blocksize = 4096;
int blockcount = 2048;
int inodesize = 128;
int inodecount = 2048;

struct inode {
	char name[0xF];
	int mode;
	int owner;
	int size;

	//mtime, 就是modify time。
	//mtime和ctime的区别在于，只有修改了文件的内容，才会更新文件的mtime，
	//		而对文件更名，修改文件的属主等操作，只会更新ctime。
	//举例说明 : 对文件进行mv操作，mtime不变，ctime更新；编辑文件内容，mt
	//		ime和ctime同时修改。其他操作的影响，可以自己试验。但是我发现对
	//		文件执行touch操作，会同时修改mtime和ctime，所以具体修改哪个时间
	//		，还取决于不同命令自己的实现；
	//atime, 这个就是每次查看文件内容的时候会更新。比如cat操作，而ls操作是不
	//		会更新的。


	time_t ctime;
	time_t atime;
	time_t mtime;
	int flag;
	int pt[Node_Per_Sector];
	int pt_parent;

};

typedef struct {
	short blockcount;
	short inodecount;
	short blockused;
	short inodeused;
	short blocksize;
	short inodesize;
	time_t mtime;
	time_t wtime;
	time_t atime;

	int inode_bitmap;
	int block_bitmap; 
}superblock;

typedef struct {
	char name[0x3F];
	int psw_hash;
	int UID;
	int Administrative_authority;
	//0 for guest
	//1 for admin
	//2 for root
}userinfo;

superblock * p_superblock;
inode * root_node;
userinfo *Cur_Usr;
userinfo *Usr_list;

int curr_address;
int dest_add;
bool *inode_bitmap;
bool *block_bitmap;

void Input();
void pr();
void self_test();
void Resolve();
void format();
void diskinfo();
int process();
int yesorno();
int free_inode();
int free_block();

void get_node(inode*, int address);
void write_node(inode *, int address);
int write_block(void *, int size, int address);
int read_block(void *, int address, int size);
int dir2add(char *);
int changedir();
void pr_time(time_t &time);
void m_mode(int n);
unsigned int BKDRHash(char* str, unsigned int len);
void pwd_in(char *pwd);


int mkdir();
int dir(int m_add);
int cd();
int import(char *dir,int add);
int more(int fcb_address);
int rm(int fcb_address);
int m_rename(int inode_address);
void help();
int rmdir(int dir_add);
void pr_mode(inode *);
int chmod(int mode, int address);
int copy(int m_from, int m_to);
void login();
void pr_signiture();
void useradd();
void flush();
void attrib(int m_add);
void find(char *,int m_node);
int xcopy(int m_from, int m_to);
int r_xcopy(int m_from, int m_to);
int move(int m_from,int m_to);
int auth(int m_node, int UID, char rwx);
int get_auth(int mode, int grop, char rwx);
int backup();
int recover();

void pr() {
	printf("\n[%s@localhost:%s]# ",Usr_list[Cur_Usr-Usr_list].name,curadd);
}

int main() {
	pr_signiture();
	self_test();
	login();
	//Cur_Usr = Usr_list;
	while (true)
	{
		Input();
		Resolve();
		process();
		flush();
	}
}

//格式化磁盘
void format() {
	FILE *fp;

	printf("您确定要格式化该磁盘吗？\n格式化后文件将不可恢复（Yes、No）\n");
	if (!yesorno())
		return;
	char buff[100];
	fp = fopen("disk.dat", "wb");
	if (fp == NULL) //打开文件失败，返回错误信息
	{
		printf("open file for write error\n");
		return;
	}

	p_superblock = (superblock *)malloc(sizeof(superblock));

	//超级块地址为1
	superblock sb;
	p_superblock = &sb;
	sb.blocksize = blocksize;
	sb.blockcount = blockcount;
	sb.inodecount = inodecount;
	sb.inodesize = inodesize;
	time(&sb.atime);
	time(&sb.wtime);
	time(&sb.mtime);
	sb.inode_bitmap = 1;//address
	sb.block_bitmap = 2;//address
	sb.blockused = 2;
	sb.inodeused = 2;

	fseek(fp, 0L, SEEK_SET);
	fwrite(&sb, sizeof(superblock), 1, fp);
	fclose(fp);

	fp = fopen("disk.dat", "rb+");
	p_superblock->blockused = 200;
	fseek(fp, 0L, SEEK_SET);
	fread(&sb, sizeof(superblock), 1, fp);

	// fread(&sb, sizeof(superblock), 1, fp);
	// printf("%d\n",sb.blocksize);

	fclose(fp);

	//fp = fopen("disk.dat", "rb");
	//p_superblock->blockused = 200;
	//fseek(fp, 0L, SEEK_SET);
	//fread(&sb, sizeof(superblock), 1, fp);

	//// fread(&sb, sizeof(superblock), 1, fp);
	//// printf("%d\n",sb.blocksize);

	//fclose(fp);

	fp = fopen("disk.dat", "rb+");
	inode in;
	char buffer[0xF];
	strcpy(buffer, "\0");
	strcpy(in.name, buffer);
	in.flag = 0;
	in.mode = 0;
	for (int i = 0; i<Node_Per_Sector; i++)
		in.pt[i] = 0;
	in.pt_parent = 0;
	in.size = 0;
	in.owner = 0;
	for (int i = 2; i < inodecount - 1; i++) {
		fseek(fp, (i - 1)*p_superblock->inodesize, SEEK_SET);
		fwrite(&in, sizeof(inode), 1, fp);
	}
	fclose(fp);


	//创建根目录
	//根目录inode地址为2
	inode root;
	strcpy(buffer, "\0");
	strcpy(root.name, buffer);
	root.flag = 1;
	root.mode = 0;
	root.owner = 0;
	for (int i = 0; i<Node_Per_Sector; i++)
		root.pt[i] = 0;
	root.pt_parent = -1;//root路径的专有标记
	root.size = 0;
	time(&root.ctime);
	time(&root.atime);
	time(&root.mtime);
	write_node(&root, 2);

	root.mode = 321;
	get_node(&root, 2);

	printf("Formatting Block ");
	//创建bitmap
	//block 1 为inode bitmap
	inode_bitmap = (bool *)malloc(sizeof(bool)*inodecount);
	memset(inode_bitmap, 0, sizeof(bool)*inodecount);
	inode_bitmap[0] = 1;
	inode_bitmap[1] = 1;
	write_block(inode_bitmap, sizeof(bool)*inodecount, 1);
	//read_block(inode_bitmap, 1, sizeof(bool)*inodecount);


	//block 2 为block bitmap
	block_bitmap = (bool *)malloc(sizeof(bool)*blockcount);
	memset(block_bitmap, 0, sizeof(bool)*blockcount);
	block_bitmap[0] = 1;
	block_bitmap[1] = 1;
	block_bitmap[2] = 1;
	write_block(block_bitmap, sizeof(bool)*blockcount, 2);
	//read_block(block_bitmap, 2, sizeof(bool)*blockcount);

	//block 3 为user info
	userinfo *u_buffer = (userinfo *)malloc(0xF * sizeof(userinfo));
	//root
	u_buffer[0].Administrative_authority = 2;
	strcpy(u_buffer[0].name,"root");
	u_buffer[0].psw_hash = BKDRHash("root", strlen("root"));
	u_buffer[0].UID = 0;
	//blank
	for (int i = 1; i < 0xF; i++) {
		u_buffer[i].Administrative_authority = -1;
		strcpy(u_buffer[i].name, "\0");
		u_buffer[i].psw_hash = 0;
		u_buffer[i].UID = -1;
	}
	write_block(u_buffer, 0xF * sizeof(userinfo),3);

	//格式化剩余磁盘
	char index[6] = "-\\|/\0";
	void *B_buffer = malloc(1);
	for (int i = 4; i < blockcount; i++) {
		if (!i % 200)
		{
			putchar('\b');
			putchar(index[i%4]);
		}
		write_block(B_buffer, 1, i);
	}
	putchar('\n');
	//fscanf(fp, "%s", buff);
	//cout<<buff<<endl;
	//fputc(0,fp);

	self_test();
	login();
};

void get_node(inode * buffer, int address) {
	FILE *fp;
	while (*pt_SIGNAL==1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;
	fp = fopen("disk.dat", "rb+");
	if (address<=0 || address>p_superblock->inodecount - 1)
	{
		printf("地址越界\n");
		return;
	}
	fseek(fp, (address - 1)*p_superblock->inodesize, SEEK_SET);
	fread(buffer, sizeof(inode), 1, fp);
	fclose(fp);
	*pt_SIGNAL = 0;
	return;
}

void write_node(inode *buffer, int address) {
	FILE *fp;
	while (*pt_SIGNAL == 1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;
	fp = fopen("disk.dat", "rb+");
	if (address<=0 || address>p_superblock->inodecount - 1)
	{
		printf("地址越界\n");
		return;
	}
	fseek(fp, (address - 1)*p_superblock->inodesize, SEEK_SET);
	fwrite(buffer, sizeof(inode), 1, fp);
	fclose(fp);
	*pt_SIGNAL = 0;
	return;
}

int write_block(void *data, int size, int address) 
{
	FILE *fp;
	while (*pt_SIGNAL == 1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;
	fp = fopen("disk.dat", "rb+");
	if (fp == NULL) {
		printf("打开文件失败\n");
		return 1;
	}
	if (size>p_superblock->blocksize) {
		printf("块区太大，写入失败");
		return 1;
	}
	fseek(fp, p_superblock->inodesize*p_superblock->inodecount + (address - 1)*p_superblock->blocksize + 1, SEEK_SET);
	fwrite(data, size, 1, fp);
	fclose(fp);
	*pt_SIGNAL = 0;
	return 0;
}

int read_block(void *dest, int address, int size = blocksize) {
	FILE *fp;
	while (*pt_SIGNAL == 1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;
	fp = fopen("disk.dat", "rb+");
	fseek(fp, p_superblock->inodesize*p_superblock->inodecount + (address - 1)*p_superblock->blocksize + 1, SEEK_SET);
	fread(dest, size, 1, fp);
	fclose(fp);
	*pt_SIGNAL = 0;
	return 0;
}

int dir2add(char * add = address) {
	//绝对路径
	if (!strcmp(add, ".")|| (*add == '\0')) {
		return curr_address;
	}
	if ((address[0] == '\\') && (address[1] == '\\'))
	{
		cur = 2;
		strcpy(address, address + 2);
	}
	else
		cur = curr_address;
	
	inode* i_cur = (inode*)malloc(sizeof(inode));
	inode *peak = (inode *)malloc(sizeof(inode));
	get_node(i_cur, cur);

	char dir[10];
	char *start = add;
	char *end = add + 1;
	char *pt;

	while (*end != '\0') {
		F2:end++;
		if (*end != '\\')
			continue;
		dir[0] = '\0';
		pt = dir;
		while (start<end) {
			*pt++ = *start++;
		}
		start++;
		end++;
		*pt = '\0';

		if (!strcmp(dir, "..")) {
			//退回上级目录
			if (i_cur->pt_parent != -1)
			{
				cur = i_cur->pt_parent;
				get_node(i_cur, cur);
			}
			else {
				printf("目录不存在\n");
				return -1;
			}
		}
		else {
			//进入下级目录
			for (int i = 0; i<Node_Per_Sector; i++) {
				if (i_cur->pt[i]>0) {
					get_node(peak, i_cur->pt[i]);
					if (!strcmp(peak->name, dir)) {
						cur = i_cur->pt[i];
						get_node(i_cur, cur);
						goto F2;
					}
				}
			}
			printf("目录不存在\n");
			return -1;
		}

	}

	//将最后的目录写入Last中
	pt = Last;
	while (end>start) {
		*pt++ = *start++;
	}
	*pt = '\0';

	//试图寻找是否存在Last
	L_add = 0;
	for (int i = 0; i<Node_Per_Sector; i++) {
		if (i_cur->pt[i]>0) {
			get_node(peak, i_cur->pt[i]);
			if (!strcmp(peak->name, Last)) {
				L_add = i_cur->pt[i];
				return i_cur->pt[i];
			}
		}
	}
	return -1;

	//可以加一个全局字符串专门用于存放最后一段操作数

}

int changedir()
{
	if (!strcmp(address, "\0"))
	{
		printf("当前路径: %s\n", curadd);
		return 0;
	}
	//解析并更改当前目录值
	//TODO:访问权限管理
	int B_cur = curr_address;
	int B_Ladd = L_add;
	char B_curadd[256];
	char B_Last[10];
	strcpy(B_curadd, curadd);
	strcpy(B_Last, Last);

	//当前的dir2add会随着处理路径改变cur值，这样引发了一个问题
	//如果返回-1无法回溯
	//干脆把判断路径的工作交给changedir（dir2add）
	//在一开始的时候存储当前目录位置，如果寻址失败，恢复目录位置。

	if ((address[0] == '\\') && (address[1] == '\\'))
	{
		cur = 2;
		strcpy(address, address + 2);
	}
	else
		cur = curr_address;
	inode* i_cur = (inode*)malloc(sizeof(inode));
	inode *peak = (inode *)malloc(sizeof(inode));
	get_node(i_cur, cur);

	char dir[10];
	dir[0] = '\0';
	char *start = address;
	char *end = address;
	char *pt;

	while (*end != '\0') {
		F:end++;
		if (*end != '\\')
			continue;
		dir[0] = '\0';
		pt = dir;
		while (start<end) {
			*pt++ = *start++;
		}
		start++;
		end++;
		*pt = '\0';

		if (!strcmp(dir, "..")) {
			//退回上级目录
			if (i_cur->pt_parent != -1)
			{
				cur = i_cur->pt_parent;
				get_node(i_cur, cur);

				char *i, *j;
				i = curadd;
				j = curadd;
				while (*j != '\0') {
					j++;
					if (*j == '\\') {
						i = j;
					}
				}
				*i = '\0';
			}
			else {
				printf("目录不存在\n");
				curr_address = B_cur;
				L_add = B_Ladd;
				strcpy(curadd, B_curadd);
				strcpy(Last, B_Last);
				return -1;
			}
		}
		else {
			//进入下级目录
			for (int i = 0; i<Node_Per_Sector; i++) {
				//TODO:利用flag标记是否为目录
				if (i_cur->pt[i]>0) {
					get_node(peak, i_cur->pt[i]);
					if (!strcmp(peak->name, dir)) {
						if (peak->flag != 1)
							goto R;
						cur = i_cur->pt[i];
						get_node(i_cur, cur);
						char *i = curadd;
						while (*i++ != '\0');
						*--i = '\\';
						strcpy(++i, dir);
						goto F;
					}
				}
			}
			printf("目录不存在\n");
			curr_address = B_cur;
			L_add = B_Ladd;
			strcpy(curadd, B_curadd);
			strcpy(Last, B_Last);
			return -1;
		}

	}

	//将最后的目录写入Last中
	pt = Last;
	while (end>start) {
		*pt++ = *start++;
	}
	*pt = '\0';

	//试图寻找是否存在Last
	L_add = 0;

	//返回目录
	if (!strcmp(Last, "..")) {
		//退回上级目录
		if (i_cur->pt_parent != -1)
		{
			cur = i_cur->pt_parent;
			get_node(i_cur, cur);

			char *i, *j;
			i = curadd;
			j = curadd;
			while (*j != '\0') {
				j++;
				if (*j == '\\') {
					i = j;
				}
			}
			*i = '\0';
			curr_address = cur;
			return 0;
		}
		else {
			printf("目录不存在\n");
			curr_address = B_cur;
			L_add = B_Ladd;
			strcpy(curadd, B_curadd);
			strcpy(Last, B_Last);
			return -1;
		}
	}
	//进入目录
	for (int i = 0; i<Node_Per_Sector; i++) {
		//TODO:利用flag标记是否为目录
		if (i_cur->pt[i]>0) {
			get_node(peak, i_cur->pt[i]);
			if (!strcmp(peak->name, Last)) {
				if (peak->flag != 1) {
					goto R;
				}
				L_add = i_cur->pt[i];
				curr_address = i_cur->pt[i];
				cur = i_cur->pt[i];
				char *pt_c = curadd;
				while (*pt_c++ != '\0');
				*--pt_c = '\\';
				strcpy(++pt_c, Last);
				printf("当前路径: %s\n", curadd);
				return cur;
			}
		}
	}

	R:printf("目录不存在\n");
	curr_address = B_cur;
	L_add = B_Ladd;
	strcpy(curadd, B_curadd);
	strcpy(Last, B_Last);
	return -1;
}

void Input() {
	pr();
	char s[256];
	gets_s(s, 256);
	strcpy(cmd, s);
	int i = 0;
	while (cmd[i])
	{                     //找到指令串结束位置i
		i++;
	}
	cmd[i] = '\0';                       //将‘\0’赋给指令串尾，以保证打印的信息为所输入的指令
}

void self_test() {
	printf("正在检查磁盘完整性...\n");
	//打开磁盘
	FILE *fp;
	// 不知道为什么，ab模式无法读取
	fp = fopen("disk.dat", "rb+");

	// delete(p_superblock);
	printf("正在读取系统数据...\n");
	p_superblock = (superblock *)malloc(sizeof(superblock));
	p_superblock->blockused = 200;
	fseek(fp, 0, SEEK_SET);
	fread(p_superblock, sizeof(superblock), 1, fp);

	//读取超级块
	p_superblock = (superblock *)malloc(sizeof(superblock));
	fseek(fp, 0, SEEK_SET);
	fread(p_superblock, sizeof(superblock), 1, fp);
	//TODO:检查超级块完整性


	//还原root
	root_node = (inode *)malloc(sizeof(inode));
	get_node(root_node, 2);//同标准EXT2文件系统相同，根目录的inode存储在2号单元中
	curr_address = 2;
	curadd[0] = '\\';
	curadd[1] = '\0';

	//尝试读取inode,检查inode格式是否正确
	//inode in;
	//for(int i = 1;i<inodecount-1;i++){

	//	fseek(fp,i*inodesize,SEEK_SET);
	//	fread(&in,sizeof(inode),1,fp);
	//	printf("%s\n",in.name);
	//	printf("%d\n",in.pt[1]);
	//}

	//还原bitmap
	block_bitmap = (bool*)malloc(sizeof(bool)*blockcount);
	read_block(block_bitmap, 2, sizeof(bool)*blockcount);
	// printf("block 1 used?: %d,block 2 used?: %d, block3 used?:%d",block_bitmap[0],block_bitmap[1],block_bitmap[2]);
	inode_bitmap = (bool*)malloc(sizeof(bool)*inodecount);
	read_block(inode_bitmap, 1, sizeof(bool)*inodecount);

	//还原userinfo
	Usr_list = (userinfo *)malloc(0xF * sizeof(userinfo));
	read_block(Usr_list, 3, 0xF * sizeof(userinfo));
	printf("启动成功\n");
	fclose(fp);
}

void Resolve() {
	//cout<<"Resolving"<<endl;
	memset(address, 0, sizeof(address));
	memset(key, 0, sizeof(key));
	int i = 0;
	while ((cmd[i] != '\0') && (cmd[i] != ' '))
	{
		key[i] = cmd[i];
		i++;
	}
	key[i] = '\0';
	if (cmd[i] != '\0')
	{
		//若有地址
		i++;
		if ((cmd[i] == '\/')||((cmd[i] == '-'))) {
			//若有参数
			i++;
			mode = cmd[i];
			while ((cmd[i] != '\0') && (cmd[i] != ' '))
				i++;
			i++;
		}
		else {
			//若无参数
			mode = 'd';//D for default
		}
		int j = 0;
		while ((cmd[i] != '\0') && (cmd[i] != ' '))
			address[j++] = cmd[i++];
		address[j] = '\0';

		if (cmd[i] != '\0')
		{
			i++;
			//第二操作数
			int j = 0;
			while ((cmd[i] != '\0') && (cmd[i] != ' '))
				Secondary_oppend[j++] = cmd[i++];
			Secondary_oppend[j] = '\0';
		}
	}
	else
	{
		//若无操作数
		address[0] = '\0';
	}
}

int free_inode() {
	for (int i = 0; i<p_superblock->inodecount; i++) {
		if (inode_bitmap[i] == 0)
			return(i + 1);
	}
	printf("达到文件数目最大值，无空闲磁盘位置");
}

int free_block() {
	for (int i = 0; i<p_superblock->blockcount; i++) {
		if (block_bitmap[i] == 0)
			return(i + 1);
	}
	printf("达到文件数目最大值，无空闲磁盘位置");
}

void pr_time(time_t &m_time) {
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

int dir(int m_add = curr_address) {
	inode *cur = (inode*)malloc(sizeof(inode));
	inode *peek = (inode*)malloc(sizeof(inode));
	get_node(cur, m_add);

	printf("给定路径下的文件：\n\n");
	pr_mode(cur);
	pr_time(cur->ctime);
	printf("  %6s  <DIR>           .\n",Usr_list[cur->owner].name);
	if (cur->pt_parent != -1)
	{
		pr_mode(peek);
		get_node(peek, cur->pt_parent);
		pr_time(peek->ctime);
		printf("  %6s  <DIR>           ..\n", Usr_list[peek->owner].name);
	}
	for (int i = 0; i<Node_Per_Sector; i++) {
		if (cur->pt[i] > 0) {
			get_node(peek, cur->pt[i]);
			switch (peek->flag) {
			case (0) :
				continue;

			case (1) :
				pr_mode(peek);
				pr_time(peek->ctime);
				printf("  %6s  <DIR>           %s\n", Usr_list[peek->owner].name, peek->name);
				break;

			case (2) :
				pr_mode(peek);
				pr_time(peek->ctime);
				printf("  %6s          %6d  %s\n", Usr_list[peek->owner].name, peek->size, peek->name);
				break;
			}
		}
	}
	return 0;
}

int mkdir() {
	inode * i_cur = (inode *)malloc(sizeof(inode));
	inode * n_node = (inode *)malloc(sizeof(inode));
	inode *peek = (inode *)malloc(sizeof(inode));
	get_node(i_cur, curr_address);
	int add = free_inode();

	//构建目录树
	int i;
	for (i = 0; i < Node_Per_Sector; i++) {
		if (i_cur->pt[i] == 0)
		{
			i_cur->pt[i] = add;
			break;
		}
		get_node(peek, i_cur->pt[i]);
		if (!strcmp(peek->name, Last)) {
			printf("该目录已存在。\n");
			return 0;
		}

	}

	//先分配inode的结点
	get_node(n_node, add);
	strcpy(n_node->name, Last);
	time(&n_node->ctime);
	time(&n_node->atime);
	time(&n_node->mtime);
	n_node->flag = 1;
	n_node->mode = 664;
	n_node->owner = Cur_Usr - Usr_list;

	n_node->pt_parent = curr_address;

	//写入磁盘
	write_node(n_node, add);
	write_node(i_cur, curr_address);

	get_node(i_cur, curr_address);

	inode_bitmap[add - 1] = 1;

	write_block(inode_bitmap, sizeof(bool)*inodecount, 1);

	return 0;

}

int cd() {
	changedir();
	return 0;
}

int import(char *dir, int add)
{
	//TODO:时间
	//TODO:文件大小
	//BUG:曾经发现过在多次同一目录下导入文件时，FCB被覆盖的情况（会在第七次试图import的时候报错？）
	int FCB;
	char *pt = dir;
	while (*pt++!= '\0');
	while (*pt-- != '\\');
	pt += 2;
	FCB = free_inode();

	inode_bitmap[FCB - 1] = 1;

	FILE*fp = fopen(dir, "r");
	//TODO:文件打开失败返回
	//获取文件控制块
	inode * in = (inode *)malloc(sizeof(inode));
	get_node(in, FCB);
	//in->ctime = now;
	time(&in->ctime);
	time(&in->atime);
	time(&in->mtime);
	in->flag = 2;
	in->mode = 640;
	strcpy(in->name, pt);
	in->pt_parent = add;
	in->owner = Cur_Usr - Usr_list;

	inode *current_node = (inode *)malloc(sizeof(inode));
	get_node(current_node, add);
	inode *peek = (inode *)malloc(sizeof(inode));

	//检查是否可装入
	for (int i = 0; i < Node_Per_Sector; i++) {
		if (current_node->pt[i] > 0) {
			get_node(peek, current_node->pt[i]);
			if (!strcmp(peek->name, in->name)) {
				printf("该文件名已存在\n");
				return 1;
			}
		}
	}

	int i;
	for (i = 0; i < Node_Per_Sector; i++) {
		if (current_node->pt[i] <= 0) {
			current_node->pt[i] = FCB;
			break;
		}
	}
	if (i == Node_Per_Sector) {
		printf("该目录已满\n");
		return 1;
	}
	

	char buffer[0xFFF+1 / sizeof(char)];
	char *pt_c = buffer;
	i = 0;
	int sizecount = 1;
	while (!feof(fp)) {
		if (pt_c >= buffer + 0xFFF / sizeof(char)) {
			++sizecount;
			pt_c = buffer;
			int Block_add = free_block();
			write_block(buffer, 0xFFF / sizeof(char), Block_add);
			in->pt[i++] = Block_add;
			block_bitmap[Block_add - 1] = 1;
		}
		fread(pt_c++, sizeof(char), 1, fp);
	}
	*--pt_c = '\0';
	in->size = sizecount * 0x1000;
	pt_c = buffer;
	int Block_add = free_block();
	write_block(buffer, 0xFFF / sizeof(char), Block_add);
	in->pt[i] = Block_add;
	block_bitmap[Block_add - 1] = 1;

	//更新inode bitmap
	write_block(inode_bitmap, sizeof(inode_bitmap), 1);
	read_block(inode_bitmap, 1, sizeof(inode_bitmap));

	//更新block bitmap
	write_block(block_bitmap, sizeof(block_bitmap), 2);

	//将文件控制块写回磁盘
	write_node(in, FCB);

	//将文件所在目录写回磁盘
	write_node(current_node, add);

	free(in);
	free(current_node);
	fclose(fp);
	return 0;
}

int more(int fcb_address) {
	//TODO:检查内存泄漏
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in, fcb_address);
	if (in->flag == 1) {
		printf("This is a directory\n");
		return 1;
	}
	else if (in->flag == 2) {
		//more指令不会修改时间戳
		char buffer[HOW_MUCH_MORE];
		read_block(buffer, in->pt[0], HOW_MUCH_MORE*sizeof(char));
		printf("%s", buffer);
	}
	free(in);
	return 0;
}

int rm(int fcb_address) {
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in, fcb_address);
	//如果不是文件则返回
	if (in->flag <= 1) {
		printf("文件名称无效\n");
		return 1;
	}

	inode *parent = (inode *)malloc(sizeof(inode));
	get_node(parent, in->pt_parent);

	read_block(block_bitmap, 2, sizeof(bool)*blockcount);
	
	for (int i = 0; i < Node_Per_Sector; i++) {
		if(in->pt[i]>0)
			block_bitmap[in->pt[i] - 1] = 0;
		if (parent->pt[i] == fcb_address)
			parent->pt[i] = 0;
	}

	int parent_add = in->pt_parent;
	in->name[0] = '\0';
	in->flag = 0;
	in->mode = 0;
	for (int i = 0; i<Node_Per_Sector; i++)
		in->pt[i] = 0;
	in->pt_parent = 0;
	in->size = 0;
	

	read_block(inode_bitmap, 1, sizeof(bool)*inodecount);
	inode_bitmap[fcb_address - 1] = 0;


	//写回block bitmap
	write_block(block_bitmap, sizeof(bool)*blockcount,2);

	//写回inode bitmap
	write_block(inode_bitmap, sizeof(bool)*inodecount, 2);

	//写回fcb
	write_node(in, fcb_address);

	//写回parent inode
	write_node(parent, parent_add);

	free(parent);
	free(in);
}

int m_rename(int inode_address) {
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in,inode_address);
	time(&in->ctime);
	time(&in->mtime);
	strcpy(in->name, Secondary_oppend);
	write_node(in, inode_address);
	return 0;
}

int m_export(int inode_address) {
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in, inode_address);
	//如果不是文件则返回
	if (in->flag != 2) {
		printf("文件名称无效\n");
		return 1;
	}
	//判断是直接导出到文件还是在目录中新建文件
	FILE *fp;
	printf("%s是路径吗？（Y/N）\n",Secondary_oppend);
	if (yesorno()) {
		char Phy_dir[0xFF];
		strcpy(Phy_dir, Secondary_oppend);
		char *ptc = Phy_dir;
		while (*ptc++ != '\0');
		if (*--ptc != '\\'){
			*ptc++ = '\\';
			*ptc = '\0';
		}
		strcat(Phy_dir, in->name);
		fp = fopen(Phy_dir, "w+");
	}
	else {
		fp = fopen(Secondary_oppend, "w+");
	}
	if (fp == NULL) {
		printf("导出目录非有效目录\n");
		return 1;
	}
	//export Desktop\3.dat C:\Users\天禹\Desktop\OS_Data\out.dat
	time(&in->atime);
	char Buffer[0xFFF];
	for (int i = 0; i < Node_Per_Sector; i++) {
		if (in->pt[i] == 0)
			break;
		read_block(Buffer, in->pt[i], 0xFFF/sizeof(char));
		char *pt_c = Buffer;
		while (pt_c-Buffer<0xFFF / sizeof(char)) {
			if (*pt_c == '\0') {
				fputc('\0', fp);
				fclose(fp);
				return 0;
			}
			//fwrite(pt_c++, sizeof(char), 1, fp);
			fputc(*pt_c++, fp);
		}
	}



}

void help() {
	printf("-----------------------------------------------------------------------\n");
	printf("                       ______________________________________\n");
	printf("                      \/                   \n");
	printf("                     \/              Virtual Disk System  \n");
	printf("                    \/_______  _____     _____   _____         \n");
	printf("                           \/ \/    \/ \/  \/       \/    \/   \n");
	printf("                          \/ \/    \/ \/  \/       \/____\/  \n");
	printf("         ________________\/ \/____\/ \/  \/_____  \/    \/￣￣\n");
	printf("                          \/                 \/    \/       \n");
	printf("                         \/      Ver.     2.1.3     \n");
	printf("                        \/      Build    2017.06    \n");
	printf("----------------------------------------------------------------------\n\n");
	printf("部分命令与使用帮助:\n\n");

	printf("命令操作符      命令格式               作用介绍\n");
	printf("cat    more    cat filename          显示文件前头部内容\n");
	printf("cd             cd path               显示当前的目录名称或将其修改\n");
	printf("chmod          chmod [-x] mode path  修改目录的访问权限\n");
	printf("cls            cls                   清除屏幕\n");
	printf("copy           copy file dest_path   将一个文件拷贝到另一目录\n");
	printf("dir    ls      dir path              显示当前目录下的信息与文件\n");
	printf("export         export file path      将虚拟磁盘内容导出到物理磁盘\n");
	printf("fdisk  format  format                格式化虚拟磁盘\n");
	printf("help           help                  版本信息与帮助界面\n");
	printf("import         import path filepath  将物理磁盘中内容导入虚拟磁盘\n");
	printf("login          login                 登录一个新的账户\n");
	printf("mkdir          mkdir dir             创建一个新目录\n");
	printf("move           move path1 path2      将一个文件或目录剪切到另一位置\n");
	printf("rename         rename file           重命名一个文件或目录\n");
	printf("rm      del    rm path\file          删除一个文件\n");
	printf("rmdir          rmdir path            删除一个目录\n");
	printf("time           time                  显示当前系统时间\n");
	printf("useradd        useradd [a][r] name   添加一个新的账户\n");
	printf("xcopy          xcopy from to         递归拷贝目录下所有子文件\n");
	system("pause");
};

int chmod(int mode, int address) {
	//TODO:在更改权限之前检查权限
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in, address);
	in->mode = mode;
	write_node(in, address);
	printf("权限更改完成\n");
	return 0;
}

int rmdir(int dir_add) {
	inode *in = (inode *)malloc(sizeof(inode));
	get_node(in, dir_add);
	if (mode == 'S' || mode == 's') {
		for (int i = 0; i < Node_Per_Sector; i++) {
			if(in->pt[i]>0)
				rmdir(in->pt[i]);
		}
	}
	else {
		for (int i = 0; i < Node_Per_Sector; i++) {
			if (in->pt[i] != 0) {
				printf("目录非空\n");
				return 1;
			}
		}
	}

	inode *parent = (inode *)malloc(sizeof(inode));
	get_node(parent, in->pt_parent);

	read_block(block_bitmap, 2, sizeof(bool)*blockcount);

	for (int i = 0; i < Node_Per_Sector; i++) {
		if(in->flag == 2)
			if (in->pt[i]>0)
				block_bitmap[in->pt[i] - 1] = 0;
		if (parent->pt[i] == dir_add)
			parent->pt[i] = 0;
	}

	int parent_add = in->pt_parent;
	in->name[0] = '\0';
	in->flag = 0;
	in->mode = 0;
	for (int i = 0; i<Node_Per_Sector; i++)
		in->pt[i] = 0;
	in->pt_parent = 0;
	in->size = 0;


	read_block(inode_bitmap, 1, sizeof(bool)*inodecount);
	inode_bitmap[dir_add - 1] = 0;


	//写回block bitmap
	write_block(block_bitmap, sizeof(bool)*blockcount, 2);

	//写回inode bitmap
	write_block(inode_bitmap, sizeof(bool)*inodecount, 2);

	//写回fcb
	write_node(in, parent_add);

	//写回parent inode
	write_node(parent, parent_add);

	printf("目录删除完成\n");
	free(parent);
	free(in);
	return 0;
}

int copy(int m_from, int m_to) {
	inode *i_from = (inode *)malloc(sizeof(inode));
	inode *i_to = (inode *)malloc(sizeof(inode));

	get_node(i_from, m_from);
	get_node(i_to, m_to);

	if (i_from->flag != 2) {
		printf("所需拷贝文件为目录，如需拷贝目录请使用xcopy\n");
		return 1;
	}
	else {
		if (i_to->flag == 2) {
			//给定的第二个操作数是一个文件
			printf("是否覆盖所选目标文件（Y/N）");
			if (yesorno()) {
				//父节点还没连接
				i_from->pt_parent = i_to->pt_parent;
				inode *p = (inode *)malloc(sizeof(inode));
				get_node(p, i_to->pt_parent);
				int i;
				for (i = 0; i < Node_Per_Sector; i++) {
					if (p->pt[i] == 0) {
						p->pt[i] = m_to;
						break;
					}
				}
				if (i == Node_Per_Sector)
				{
					printf("目录已满");
					return 1;
				}


				for (int i = 0; i < Node_Per_Sector; i++) {
					if (i_from->pt[i] != 0) {
						int m_block = free_block();
						//Blockbitmap,把from的block写到新block中
						block_bitmap[m_block - 1] = 1;
						write_block(block_bitmap, sizeof(block_bitmap), 2);
						void *buffer = malloc(p_superblock->blocksize);
						//write_block(block_bitmap, sizeof(block_bitmap), 2);
						read_block(buffer, i_from->pt[i]);
						i_from->pt[i] = m_block;
						write_block(buffer, p_superblock->blocksize, m_block);
					}
				}				
				write_node(p, i_to->pt_parent);
				write_node(i_from, m_to);
				free(i_from);
				free(i_to);
				free(p);
				return 0;
			}
			else
				return 1;
		}
		else {
			//给定的第二个地址为一个目录
			//新建一个文件控制块
			int FCB = free_inode();
			i_from->pt_parent = m_to;
			inode_bitmap[FCB - 1] = 1;
			int i;
			inode peek;
			for (i = 0; i < Node_Per_Sector; i++) {
				if(i_to->pt[i]>0)
					get_node(&peek, i_to->pt[i]);
					if (!strcmp(i_from->name, peek.name))
					{
						printf("该文件名称已存在\n");
						return 1;
					}
			}

			for (i = 0; i < Node_Per_Sector; i++) {
				if (i_to->pt[i] == 0) {
					i_to->pt[i] = FCB;
					break;
				}
			}
			if (i == Node_Per_Sector)
			{
				printf("目录已满");
				return 1;
			}

			write_node(i_to, m_to);
			write_block(inode_bitmap, sizeof(inode_bitmap), 1);
			for (int i = 0; i < Node_Per_Sector; i++) {
				if (i_from->pt[i] != 0) {
					int m_block = free_block();
					//Blockbitmap,把from的block写到新block中
					block_bitmap[m_block - 1] = 1;
					write_block(block_bitmap, sizeof(block_bitmap), 2);
					void *buffer = malloc(p_superblock->blocksize);
					//write_block(block_bitmap, sizeof(block_bitmap), 2);
					read_block(buffer, i_from->pt[i], p_superblock->blocksize);
					i_from->pt[i] = m_block;
					write_block(buffer, p_superblock->blocksize, m_block);
				}
				time(&i_from->atime);
				time(&i_from->mtime);
				time(&i_from->ctime);
				write_node(i_from, FCB);
				free(i_from);
				free(i_to);
				return FCB;
			}
		}

	}
}

//
int process() {
	//cout<<"Processing"<<endl<<" key:"<<key<<" add:"<<address<<endl;
	if (!strcmp(key, "cd"))
	{
		changedir();
		return 1;//显示或改变工作目录
	}
	else if (!strcmp(key, "ls"))
	{
		if (address[0] != '\0') {
			//显示一个目录中的子目录
			int m_address = dir2add();
			if (m_address == -1) {
				printf("目录不存在\n");
				return 0;
			}
			else {
				dir(m_address);
				return 0;
			}
		}
		dir();
		return 0;
	}
	else if (!strcmp(key, "dir"))
	{
		if (address[0] != '\0') {
			//显示一个目录中的子目录
			int m_address = dir2add();
			if (m_address == -1) {
				printf("目录不存在\n");
				return 1;
			}
			else {
				dir(m_address);
				return 0;
			}
		}
		dir();
		return 0;
	}
	else if (!strcmp(key, "mkdir"))
	{
		dir2add();
		if (auth(curr_address, Cur_Usr - Usr_list, 'w'))
		{
			mkdir();
			printf("目录创建完成\n");
		}
		else
			printf("权限不足,请联系管理员\n");
		return 0;//新建文件夹
	}
	else if (!strcmp(key, "rm"))
	{
		//删除文件
		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'w'))
			{
				rm(m_add);
				printf("文件删除成功\n");
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("不存在该路径\n");
		return 1;
	}
	else if (!strcmp(key, "rmdir"))
	{
		//删除文件夹
		//实现/S

		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'w'))
			{
				rmdir(m_add);
				printf("文件删除成功\n");
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("查无此目录\n");
		return 1;
	}
	else if (!strcmp(key, "cls"))
	{
		system("cls");
		return 0;//清屏幕
	}
	else if (!strcmp(key, "rename"))
	{
		//重命名
		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'w'))
			{
				m_rename(m_add);
				printf("文件重命名成功\n");
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("查无此目录\n");
		return 1;
	}
	else if (!strcmp(key, "help"))
	{
		//帮助文档
		help();
		return 0;
	}
	else if (!strcmp(key, "import"))
	{
		int m_add = dir2add(Secondary_oppend);
		if (m_add == -1) {
			printf("目录不存在\n");
			return 1;
		}
		import(address, m_add);
		return 0;//从物理磁盘中读入文件
	}
	else if (!strcmp(key, "cat"))
	{
		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'r'))
			{
				more(m_add);
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("查无此目录\n");
		return 1;
	}
	else if (!strcmp(key, "more"))
	{
		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'r'))
			{
				more(m_add);
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("查无此目录\n");
		return 1;
	}
	else if (!strcmp(key, "export"))
	{
		//导出文件到指定目录
		int m_add = dir2add();
		if (m_add > 0) {
			m_export(m_add);
			return 0;
		}
		return 1;
	}
	else if (!strcmp(key, "format"))
	{
		format();//格式化磁盘
		return 0;
	}
	else if (!strcmp(key, "ver"))
	{
		pr_signiture();//显示版本信息
		return 0;
	}
	else if (!strcmp(key, "fdisk"))
	{
		format();//格式化磁盘
		return 0;
	}
	else if (!strcmp(key, "time"))
	{
		//显示时间
		char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		time_t timep;
		struct tm *p;
		time(&timep);
		p = localtime(&timep);
		printf("%d\/%d\/%d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
		printf("%s %d:%d:%d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
		return 0;
	}
	else if (!strcmp(key, "del"))
	{
		//删除文件
		int m_add = dir2add();
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'w'))
			{
				rm(m_add);
				printf("文件删除成功\n");
			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("不存在该路径\n");
		return 1;
	}
	else if (!strcmp(key, "chmod"))
	{
		//更改权限，只支持数字更改
		int m_mode = atoi(address);
		strcpy(address, Secondary_oppend);
		int m_add = dir2add();

		//root权限可以直接更改

		if (m_add == -1) {
			printf("文件不存在\n");
			return 1;
		}
		chmod(m_mode,m_add);
		return 0;
	}
	else if (!strcmp(key, "copy"))
	{
		int from = dir2add();
		int to = dir2add(Secondary_oppend);
		if ((from == -1) || (to == -1)) {
			printf("请输入有效地址");
			return 1;
		}
		copy(from, to);
		return 0;
	}
	else if (!strcmp(key, "exit"))
	{
		//退出系统
		exit(0);
	}
	else if (!strcmp(key, "check"))
	{
		//检查磁盘有效性
		self_test();
		return 0;
	}
	else if (!strcmp(key, "useradd"))
	{
		//添加用户
		useradd();
		return 0;
	}
	else if (!strcmp(key, "login"))
	{
		//重新登陆
		login();
		return 0;
	}
	else if (!strcmp(key, "attrib"))
	{
		//显示目录下文件权限
		int m_add = dir2add();
		if (m_add == -1)
			return 1;
		attrib(m_add);
		return 0;
	}
	else if (!strcmp(key, "diskinfo"))
	{
		//显示虚拟磁盘信息
		diskinfo();
		return 0;
	}
	else if (!strcmp(key, "backup"))
	{
		//备份虚拟磁盘
		backup();
		return 0;
	}
	else if (!strcmp(key, "recover"))
	{
		//恢复虚拟磁盘
		recover();
		return 0;
	}
	else if (!strcmp(key, "find"))
	{
		//寻找文件中的一段字符
		int m_add = dir2add(Secondary_oppend);
		if (m_add > 0) {
			if (auth(m_add, Cur_Usr - Usr_list, 'r'))
			{
				find(address, m_add);

			}
			else
				printf("权限不足,请联系管理员\n");
			return 1;
		}
		printf("查无此目录\n");
		return 1;
	}
	else if (!strcmp(key, "xcopy"))
	{
		//拷贝整个目录书
		int from = dir2add();
		int to = dir2add(Secondary_oppend);
		if ((from == -1) || (to == -1)) {
			printf("请输入有效地址");
			return 1;
		}
		xcopy(from, to);
		return 0;
	}
	else if (!strcmp(key, "copy"))
	{
		int from = dir2add();
		int to = dir2add(Secondary_oppend);
		if ((from == -1) || (to == -1)) {
			printf("请输入有效地址");
			return 1;
		}
		move(from, to);
		return 0;
	}
	else
	{
		printf("请输入有效命令\n");
		return 0;
	}
}

int yesorno() {
	char c[30];
	scanf("%s", c);
	getchar();
	if (!strcmp(c, "Y") || !strcmp(c, "y") || !strcmp(c, "yes") || !strcmp(c, "Yes") || !strcmp("YES", c))
		return 1;
	else
		return 0;
}

void diskinfo() {
	//TODO:这玩意咋维护啊2333333，一言不合就关窗口的
	printf("最多可存文件数目（indode）:%d\n磁盘块区数目:%d\n磁盘块区大小:%d\ninode大小:%d\n已使用文件数目:%d\n已使用块区数目:%d\n",
		p_superblock->inodecount,
		p_superblock->blockcount,
		p_superblock->blocksize,
		p_superblock->inodesize,
		p_superblock->inodeused,
		p_superblock->blockused);
}

void pr_mode(inode *n) {
	switch (n->flag)
	{
	case(0) :
		putchar('x');
		break;
	case(1) :
		putchar('d');
		break;
	case(2) :
		putchar('-');
		break;
	case(3) :
		putchar('l');
		break;
	default:
		putchar('x');
		break;
	}

	m_mode(n->mode / 100);
	m_mode((n->mode % 100) / 10);
	m_mode(n->mode % 10);
	putchar(' ');
	return;
};

void m_mode(int n) {
	char c[4] = "rwx";
	for (int i = 0; i < 3; i++) {
		if ((n & 4) >> 2)
			putchar(c[i]);
		else
			putchar('-');
		n = n << 1;
	}
};

unsigned int BKDRHash(char* str, unsigned int len)
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	unsigned int i = 0;
	for (i = 0; i < len; str++, i++)
	{
		hash = (hash * seed) + (*str);
	}
	return hash;
}

void login() {
	int count = 0;
	printf("-----------------------------------------------\n");
	printf("--              请输入登录信息               --\n");
	printf("-----------------------------------------------\n");
	T:printf("Username:");
	char usr_name[0x3F];
	scanf("%s", usr_name);
	int UID,i;
	for (i = 0; i < 0xF; i++)
		if (!strcmp(usr_name, Usr_list[i].name))
		{
			UID = Usr_list[i].UID;
			break;
		}
	if (i == 15) {
		printf("用户名错误\n");
		goto T;
	}
	printf("Password:");
	char pwd[0xFF];
	pwd_in(pwd);
	int HashValue = BKDRHash(pwd, strlen(pwd));
	if (HashValue == Usr_list[i].psw_hash) {
		printf("\nLogin Success\n");
		Cur_Usr = &Usr_list[i];
		getchar();
		//显示时间
		char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		time_t timep;
		struct tm *p;
		time(&timep);
		p = localtime(&timep);
		printf("-----------------------------------------------\n");
		printf("Welcome to S.P.I.C.A Virtual Disk System, for \nmore information,please type \"help\", or visit \n\"www.liutianyu.cn\" for further support. Thank \nyou for using.\n\n");
		printf("Login time: %d\/%d\/%d ", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
		printf("%s %d:%d:%d\n", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
		return;
	}
	else if(count++<3){
		printf("\n密码错误，请重新输入\n");
		goto T;
	}
	else
	{
		printf("\n请稍后重新尝试\n");
		exit(0);
	}
}

void find(char *to_find,int add) {
	inode peek;
	get_node(&peek, add);
	if (peek.flag == 1) {
		printf("无法访问文件夹\n");
		return;
	}
	else {
		printf("------------%s\n", Secondary_oppend);
		char buffer[0x1000];
		for (int i = 0; i < Node_Per_Sector; i++) {
			if (peek.pt[i] > 0) {
				read_block(buffer, peek.pt[i], p_superblock->blocksize);
				char * s = strstr(buffer, to_find);
				while (s != NULL) {
					printf("...");
					char *pr = s;
					if (pr - buffer > 8)
						pr -= 8;
					else
						pr = buffer;
					for (int i = 0; i < 20; i++)
						if (pr++ != '\0') {
							if (*pr != '\n')
								putchar(*pr);
						}
						else
							break;
					printf("...\n");
					s = strstr(++s, to_find);

				}
			}
		}
	}
}

void attrib(int m_add) {
	inode cur;
	inode peek;
	get_node(&cur, m_add);
	for (int i = 0; i < Node_Per_Sector; i++) {
		if (cur.pt[i]>0) {
			get_node(&peek, cur.pt[i]);
			pr_mode(&peek);
			if(address[0] == '\0')
				printf("    %s\\%s\n", curadd, peek.name);
			else
				printf("    %s\\%s\\%s\n",curadd,address,peek.name);
		}
	}
}

void pr_signiture() {
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
	printf("----------------------------------------------\n\n");
}

void useradd() {
	userinfo* pt_u= Usr_list;
	if (!strcmp("\0", address)) {
		printf("请输入用户名\n");
		return;
	}
	//判断是否已经存在该用户
	for (int i = 0; i < 0xF; i++) {
		if (!strcmp(pt_u->name, address))
		{
			printf("该用户已存在\n");
			return;
		}
		pt_u++;
	}
	pt_u = Usr_list;
	while ((pt_u->Administrative_authority != -1)&&(pt_u-Usr_list<0xF))
		pt_u++;
	if ((pt_u - Usr_list) == 0xF)
	{
		printf("此文件系统只支持15位用户，当前用户已满。\n详细信息请联系管理员\n");
		return;
	}
	switch (mode)
	{
	case('d') :
		pt_u->Administrative_authority = 0;
		break;
	case('a') :
		pt_u->Administrative_authority = 1;
		break;
	case('r') :
		pt_u->Administrative_authority = 2;
		break;
	case('h') :
		printf("该指令用于创建新用户：\n");
		printf("默认创建Guest用户\n");
		printf("使用‘/a’创建管理员用户\n");
		printf("使用‘/r’创超级建管理员用户\n");
		break;
	default:
		break;
	}
	strcpy(pt_u->name, address);
	pt_u->UID = pt_u - Usr_list;
	
	char pwd1[0x3F];
	char pwd2[0x3F];
	while (true) {
		printf("请输入密码:");
		pwd_in(pwd1);
		printf("\n再输入一次密码:");
		pwd_in(pwd2);
		if (!strcmp(pwd1, pwd2))
			break;
		printf("\n密码输入不一致，请从新输入\n");
	}
	pt_u->psw_hash = BKDRHash(pwd1, strlen(pwd1));
	write_block(Usr_list, 0xF * sizeof(userinfo), 3);
	printf("\n用户创立成功\n");
}

void pwd_in(char *pwd) {
	int p = 0;
	char ch;
	while ((ch = _getch()) != '\r')
	{
		if (ch == 8)
		{
			putchar('\b');
			putchar(' ');
			putchar('\b');
			if (p > 0)
				p--;
		}
		if (!isdigit(ch) && !isalpha(ch))
			continue;
		putchar('*');
		pwd[p++] = ch;
	}
	pwd[p] = '\0';
	return;
}

void flush() {
	*cmd = '\0';
	*key = '\0';
	mode = 'd';
	*address = '\0';
	*Secondary_oppend = '\0';
	*Last = '\0';//最后一段地址名
	L_add = -1;//最后一段地址
}

int xcopy(int m_from, int m_to) {
	inode i_from;
	inode i_to;
	inode peek;

	get_node(&i_from, m_from);
	get_node(&i_to, m_to);

	if (i_from.flag == 2)
	{
		copy(m_from, m_to);
		return 1;
	}

	if (i_to.flag == 2) {
		printf("无法将目录拷贝到文件\n");
		return 1;
	}

	for (int i = 0; i < Node_Per_Sector; i++) {
		if (i_to.pt[i] == 0) {
			i_to.pt[i] = r_xcopy(m_from, m_to);
			break;
		}
	}
	write_node(&i_to, m_to);

}

int r_xcopy(int m_from, int m_to) {
	inode i_from;
	inode peek;
	inode i_to;
	inode *n_node = (inode*)malloc(sizeof(inode));

	get_node(&i_to, m_to);
	get_node(&i_from, m_from);

	int new_add = free_inode();
	int r_value = new_add;
	//判断是否已经存在
	for (int i = 0; i < Node_Per_Sector; i++) {
		if (i_to.pt[i] > 0) {
			get_node(&peek, i_to.pt[i]);
			if (!strcmp(peek.name, i_from.name)) {
				if (peek.flag == 2)
				{
					printf("文件已存在\n");
					return 0;
				}
				r_value = 0;
				//Combine
				for (int j = 0; j < Node_Per_Sector; j++) {
					if (i_from.pt[j] != 0) {
						int k;
						for (k = 0; k < Node_Per_Sector; k++)
						{
							if (peek.pt[k] == 0) {
								peek.pt[k] = r_xcopy(i_from.pt[j], i_to.pt[i]);
							}
						}
					}
				}
				return 0;
			}
			break;
		}
	}

	//if (i_from.flag == 2)
	//	copy(m_from, m_to);

	//不重复
	int n_add = free_inode();
	get_node(n_node, n_add);
	strcpy(n_node->name, i_from.name);
	time(&n_node->ctime);
	time(&n_node->atime);
	time(&n_node->mtime);
	n_node->flag = 1;
	n_node->mode = 664;
	n_node->owner = Cur_Usr - Usr_list;
	n_node->pt_parent = m_to;
	write_node(n_node, n_add);

	inode_bitmap[n_add - 1] = 1;
	write_block(inode_bitmap, sizeof(bool)*inodecount, 1);

	for (int i = 0; i < Node_Per_Sector; i++) {
		inode pp;
		if (i_from.pt[i] > 0) {
			get_node(&pp, i_from.pt[i]);
			switch (pp.flag)
			{
			case(1) :
				n_node->pt[i] = r_xcopy(i_from.pt[i], n_add);
				write_node(n_node, n_add);
				break;
			case(2) :
				n_node->pt[i] = copy(i_from.pt[i], n_add);
				write_node(n_node, n_add);
				break;
			default:
				return 0;
				break;
			}
		}
	}

	write_node(n_node, n_add);
	return n_add;
}

int move(int m_from, int m_to) {
	inode peek;
	inode i_to;
	inode i_from;
	get_node(&i_to, m_to);
	get_node(&i_from, m_from);

	for (int i = 0; i < Node_Per_Sector; i++) {
		if (i_to.pt[i] > 0) {
			get_node(&peek, i_to.pt[i]);
			if (!strcmp(peek.name, i_from.name)) {
				printf("文件已存在\n");
				return 1;
			}
		}
	}

	for (int i = 0; i < Node_Per_Sector; i++) {
		if (i_to.pt[i] == 0) {
			i_from.pt_parent = m_to;
			i_to.pt[i] = m_from;
			get_node(&peek, i_from.pt_parent);
			for (int i = 0; i < Node_Per_Sector; i++) {
				if (i_to.pt[i] == m_from) {
					i_to.pt[i] = 0;
					break;
				}
				write_node(&i_from, i_to.pt[i]);
				break;
			}
		}

	}
}

int get_auth(int mode, int grop,char rwx) {
	switch (grop)
	{
	case(1) :
		mode /= 100;

		break;
	case(2) :
		mode = (mode % 100) / 10;
		break;
	case(3) :
		mode = mode % 10;
		break;
	default:
		mode = 0;
		break;
	}
	switch (rwx)
	{
	case('r') :
		return mode & 4;
	case('w') :
		return mode & 2;
	case('x') :
		return mode & 1;
	default:
		return 0;
	}
}

int backup()
{
	while (*pt_SIGNAL == 1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;

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

	*pt_SIGNAL = 0;
	return 0;
}

int recover() {
	while (*pt_SIGNAL == 1) {
		Sleep(5);
	}
	*pt_SIGNAL = 1;
	FILE *SourceFile, *BackupFile;


	BackupFile = fopen("backup.dat", "rb");

	//No backup found
	if (BackupFile = NULL) {
		printf("未找到备份文件\n");
		return 1;
	}
	SourceFile = fopen("disk.dat", "wb");

	void *buf = malloc(1);
	while (!feof(SourceFile))
	{
		fread(&buf, 1, 1, BackupFile);
		fwrite(&buf, 1, 1, SourceFile);
	}

	fclose(SourceFile);
	fclose(BackupFile);
	printf("恢复完成\n");
	*pt_SIGNAL = 0;
	return 0;
}

int auth(int m_node, int UID, char rwx) {
	inode buffer;
	get_node(&buffer, m_node);

	//文件所有者
	if (buffer.owner == UID) 
		return get_auth(buffer.mode, 1, rwx);
	//组用户
	if(Usr_list[buffer.owner].Administrative_authority == Usr_list[UID].Administrative_authority)
		return get_auth(buffer.mode, 2, rwx);
	//其他用户
	return get_auth(buffer.mode, 3, rwx);
}
