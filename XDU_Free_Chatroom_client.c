/* 客户端client.c 源码
	作者：Maxx
	学号：181802xxxxx
	学院：网络与信息安全学院
	专业：网络工程
	时间：2020/10/25 ~ 2020/12/?
*/
#include <stdio.h>
#include<stdlib.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <signal.h>
#include <ncurses.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h> 

#define MAXDATASIZE 150
#define port 3000
#define server_port 9999
#define MAXFILENAME 500
#define BUFFERSIZE 1000

struct sockaddr_in client_addr;
struct sockaddr_in server_addr;
int sockfd;
char name[20];//记录该客户端的用户名
char global_filename[MAXFILENAME];//记录该客户端的传输文件名
//memset(name,0,sizeof(name));//初始化
int client_id;
//定义拷贝字符串的函数，用于将用户输入内容拷贝到需要传输的消息内容的字段中
void *String_copy(char *dest,const char *src,int n)  
{  
    char *strDest=dest;  
    if((dest!=NULL)&&(src!=NULL)){
        while(n &&(*dest++=*src++)!='\0')  
        {  
            n--;  
        }  
        if (n)  
        {  
            while(--n)  
            {  
                *dest++='\0';  
            }  
        }  
    }
}

//注册成功后给本客户端的client_id赋值
void change_client_id(int register_id){
	client_id=register_id;
	//return 0;
}
void change_global_filename(char * newfilename){
	strcpy(global_filename,newfilename);
}
//更改全局变量name->log out函数会引用这个变量的值
void * change_username(char * login_username){
	strcpy(name,login_username);
}

//定义展示聊天室功能界面 （该功能界面不包含注册与登陆部分，注册与登陆部分独立划分出来，准确来说是在完成注册登陆之后才可看到该功能界面）
int show_functions(){
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
	printf("|~~~~~~~~~~~Welcome to the XDU Free Chatroom~~~~~~~~~~~~|\n");
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
	printf("|~~~~~function_No.1: Chat to others(select 1)~~~~~~~~~~~|\n"); //输入字符1：发送信息功能
	printf("|~~~~~function_No.2: Show online users(select 2)~~~~~~~~|\n"); //输入字符3：在线用户查询功能
	printf("|~~~~~function_No.3: Download file from server(select 3)|\n"); //输入字符4：文件传送功能
	printf("|~~~~~function_No.4: Log out(select 4)~~~~~~~~~~~~~~~~~~|\n"); //输入字符5：用户登出聊天室
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
}
//提示符：用来接收用户的selection_character，单独作为一个函数出来。
int show_prompt(int * selection_character){
	//getchar()
	//sleep(5)
	printf("Your Selection is:");
	scanf("%d",selection_character);
	//return 0;
	return 1;

}
//通过用户输入的字符，选择相应的功能函数
int select_function(){
	show_functions();
	int selection_character;
	while(1==show_prompt(&selection_character)){	
		switch(selection_character)
		{
			case 1:
				function_chat_to_others();
				break;
			case 2:
				function_show_online_users();
				break;
			case 3:
				function_download_file_from_server();
				break;
			case 4:
				function_log_out();
				break;
			default:
				printf("Your Selection is:");
				break;
		}
	}
	return 0;
}
//定义用户结构体
struct user
{
	char username[20];
	char password[20];
	int userid;
	struct sockaddr user_addr;	
};

//定义消息结构体
struct message
{	
	int message_type; //message_type=1：普通的消息；message_type=2：表示注册信息的消息；message_type=3：表示登录信息的消息；message_type=4：表示文件传送的消息;
	char message_content[MAXDATASIZE]; //消息的具体内容
	int userid_src; //消息的发送者id
	int userid_dst; //消息的接收者id
	char message_username[20];//消息的发送者姓名（！待议！）
	char message_filename[20]; //针对bonus功能的文件名字段
};

//发送聊天信息功能
int function_chat_to_others(){
	//printf("1\n");
	struct message chat_message;
	memset(&chat_message,0,MAXDATASIZE);
	chat_message.message_type=1;
	chat_message.userid_src=client_id;
	int chat_to_user_id;
	char end_char='\0';
	printf("Input the userid you want to chat to:");
	scanf("%d",&chat_to_user_id);
	getchar();
	chat_message.userid_dst=chat_to_user_id; //赋值
	printf("Input the content of the message:");
	int i;
	for(i=0;i<MAXDATASIZE;i++){
		chat_message.message_content[i]=getchar();
		if(chat_message.message_content[i]=='\n'){
			break;
		}
	}
	printf("Confirm!Your message content is:%s\n",chat_message.message_content);
	int temp1,temp2;
	temp1=sizeof(struct message);
	temp2=sizeof(struct sockaddr);
	//给服务端发送聊天的消息，由服务端进行消息的转发。
	if(sendto(sockfd,&chat_message,temp1,0,(struct sockaddr*)&server_addr,temp2)==-1){
		perror("error!\n");
		exit(1);
	}
	return 0;
}

//显示在线用户功能
int function_show_online_users(){
	int Max_size = 10; //目前最多支持10个人
	printf("******* online users *******\n");
	FILE * r = fopen("online_user_list.txt","r+");
	//assert(r!=NULL);
	struct user usrs[10];
	int i =0;
	while(fscanf(r,"%s",usrs[i].username)!=EOF)
	{
		//printf("%d",i);
		printf("%s\n",usrs[i].username);
		i++;
	}
	printf("目前一共有%d个用户在线\n",i);
	fclose(r);
	//system("pause");
	return 0;		
}

//传送文件功能(bonus+2)  改动time:2020/12/6 采用UDP的通信方式-> 思路方法类似chat(message_type->回送消息文件内容->写入本地目录)
int function_download_file_from_server(){ 
	//printf("unfinished\n");
	struct message filename_message;
	struct message filename_message_reply;
	memset(&filename_message,0,MAXDATASIZE);
	memset(&filename_message_reply,0,MAXDATASIZE);
	filename_message.message_type=4;
	int temp1,temp2;
	temp1=sizeof(struct message);
	temp2=sizeof(struct sockaddr);
	struct sockaddr addr_recv;
	filename_message.userid_src=client_id;//标记客户端的id->服务端根据该id查找用户文件->查找地址
	char filename[MAXFILENAME];
	printf("Input the filename you want to download from server:");
	scanf("%s",filename);
	strncpy(filename_message.message_content,filename,MAXDATASIZE);
	change_global_filename(filename);//修改需要下载的文件名——全局变量
//	printf("global_filename is %s\n",global_filename);
	//发送文件名给服务端
	if(sendto(sockfd,(struct message*)&filename_message,temp1,0,(struct sockaddr*)&server_addr,sizeof(struct sockaddr)) == -1){
		perror("send error!");
		exit(1);
	}
	//接收来自服务端返回响应的的文件内容信息
	/*  改动time：2020/12/10：该部分接收文件的内容放置在主进程的listen里进行处理 避免冲突
	if(recvfrom(sockfd,(struct message*)&filename_message_reply,temp1,0,&addr_recv,&temp2)==-1){
		perror("receive eorror!");
		exit(1);
	}
	if(filename_message_reply.message_type != 4){
		printf("error! Download file %s failed!\n",filename);
		exit(1);
	}
	else{
		FILE * fp=fopen(filename,"w");
		if(fp == NULL){
			printf("Open file failed!\n");
			exit(1);
		}
		fwrite(filename_message_reply.message_content,sizeof(filename_message_reply.message_content),1,fp);
		printf("Download file:%s successfully!\n",filename);
		fclose(fp);
	}
	*/
	return 0;
}

//退出聊天室功能
int function_log_out(){
	FILE *in,*out;
	char temp[20];
	in=fopen("online_user_list.txt","r");//打开需要读的原文件online_user_list.tx
	out=fopen("temp.txt","w");//打开需要写入的原文件temp.txt
	if(in==NULL || out==NULL){
		printf("Open the file failed\n");
		exit(0);
	}
	//printf("%s\n",name);
	while(fgets(temp,20,in)){
		if((strncmp(temp,name,3))!=0){ //校验是否为该将要退出登陆的用户名
			//printf("%s",temp);
			//printf("1");
			fputs(temp,out);//不是则将这条用户信息复制写入到temp.txt中
		}
	}
	fclose(in);
	fclose(out);
	remove("online_user_list.txt");//删除原来的online_user_list.txt
	rename("temp.txt","online_user_list.txt");//将临时文件改名为新的online_user_list.txt
	printf("You logged out,Bye！\n");
	close(sockfd);
		/*		explaintion：
			SIGABRT是中止一个程序，它可以被捕捉，但不能被阻塞。处理函数返回后，所有打开的文件描述符将会被关闭，
			流也会被flush。程序会结束，有可能的话还会core dump。 当程序调用abort(3)时，该进程会向自己发送SIGABRT信号。
			所以，SIGABRT一般用于信号中一些关键的处理，assert失败时也会使用它。
			不应该去捕捉SIGSEGV和SIGABRT信号，如果收到这种信号，说明进程处于一个不确定的状态，很可能会直接挂起。
		*/
		//这里使用kill系统调用的SIGABRT类型
	kill(0, SIGABRT);
	exit(0);
}

//选择注册或者登陆操作       (!!!!！！！！!该功能需要放置在主程序开始部分！！!!!!!! 逻辑上：注册成功->登陆成功->选择其他具体功能)
int choose_register_or_login(){
	char selection_character;
	printf("*******Register(select 1) Or Login(select 2)*******\n");
	scanf("%c",&selection_character);
	getchar();
	switch(selection_character){
		case '1':
			//printf("You are registing now!\n");
			function_register();
			break;
		case '2':
			//printf("You are logining now!\n");
			function_login();
			break;
		default:
		   //错误输入字符非1非2，再次返回进行该函数
			choose_register_or_login();
			break;
	}
	return 0;
}

//改动time：2020/12/1 重新定义消息结构体-> 该message结构体增加消息类型变量（该想法来源于一个博主的示例代码）并且在其的想法上，加入了用户id字段，解决了文件的读取查询困难问题。

int userid_src;
//改动time：2020/12/1 根据重新定义后的消息结构体更改register函数，通过判断消息类型字段区分消息内容比之前纯判断消息内容的方法确实要美妙得多。
int function_register(){
	//一系列的初始化
	printf("You are registing now \n");
	int temp = sizeof(struct sockaddr);
	struct sockaddr addr_recv; //定义回送注册消息的sockaddr结构体
	memset(&addr_recv,0,sizeof(struct sockaddr));
	char username[20];
	memset(username,0,sizeof(username));
	char password[20];
	memset(password,0,sizeof(password));
	//提示输入用户名及密码
	printf("Regist username is:");
	scanf("%s",&username);
	printf("Regist password is:");
	scanf("%s",&password);
	// 输完注册用户名和密码后，客户端与服务端进行通信，客户端发送注册消息给服务端，服务端接收该消息，并且记录该条注册信息，写进userlist.txt中
	struct message register_message; //定义一个注册消息的结构体
	memset(&register_message,0,sizeof(struct message));
	struct message register_message_reply; //定义一个服务端回复注册消息的结构体
	memset(&register_message_reply,0,MAXDATASIZE);
	register_message.message_type=2; //这表示发送给服务端的该消息为注册信息
	register_message.userid_src=0;
	register_message.userid_dst=0;
	memset(register_message.message_content,0,MAXDATASIZE);
	//将用户名和密码的字符串拷贝到该消息结构体的message_content字符数组里中
	strncpy(register_message.message_content,username,20);
	strncpy(&(register_message.message_content[20]),password,20);
	register_message.message_content[40]='\n';

	printf("%s",register_message);
	int temp2 = sizeof(struct message);
	//发送注册信息给服务端：
	if(sendto(sockfd,(struct message*)&register_message,temp2,0,(struct sockaddr*)&server_addr,sizeof(struct sockaddr)) == -1){
		perror("send error!");
		exit(1);
	}

	//接收来自服务端的注册确认消息：

	if(recvfrom(sockfd,(struct message*)&register_message_reply,temp2,0,&addr_recv,&temp)==-1){
		perror("receive eorror!");
		exit(1);
	}
	//printf("register_message_reply.message_type = %d\n",register_message_reply.message_type);
	//printf("register_message_reply.message_content = %s\n",register_message_reply.message_content);
	//printf("register_message_reply.userid_dst = %d\n",register_message_reply.userid_dst);
	//printf("register_message_reply.userid_src = %d\n",register_message_reply.userid_src);

	//改动time：2020/12/1 根据消息结构体中新加的消息类型字段判断是否注册成功（非原创）
	if(register_message_reply.message_type != 2){
		printf("error! Regist failed!: %s \n", register_message_reply.message_content);
		exit(1);
	}
	else{
		printf("%s You have registed success! And Your register ID is:%d\n",register_message_reply.message_content,register_message_reply.userid_dst);
	}
	change_client_id(register_message_reply.userid_dst);
	printf("Do you want to login right now?(1 or 0)\n");
	int choice;
	scanf("%d",&choice);
	getchar();
	//printf("%c",&choice);
	switch(choice){
		case 1:
			function_login();
			break;
		case 0:
			printf("Bye!\n");
			return 0;
			break;
		default:
			printf("error choice!\n");
			//return 0;
			choose_register_or_login();
			break;
	}
	return 0;
}

//改动time 2020/12/1 改动部分，idea等同上。
int function_login(){
	int temp = sizeof(struct sockaddr);
	struct message login_message;
	struct message login_message_reply;
	memset(&login_message,0,sizeof(&login_message));
	login_message.message_type=3;
	login_message.userid_dst=0;
	struct sockaddr addr_recv;///用来接收服务端响应的临时sockaddr
	int temp_id; //临时的用户id输入
	char username[20];
	char password[20];
	memset(username,0,sizeof(username));
	printf("userid:");
	scanf("%d",&temp_id);
	printf("username:");
	scanf("%s",username);
	printf("password:");
	scanf("%s",password);
	getchar();
	login_message.userid_src=temp_id;
	//String_copy(login_message.message_content,username,20);
	//String_copy(&login_message.message_content[20],password,20);
	strncpy(login_message.message_content,password,20);

	//发送登陆信息给服务端：
	if(sendto(sockfd,(struct message*)&login_message,sizeof(struct message),0,(struct sockaddr*)&server_addr,temp) == -1){
		perror("send error!");
		exit(1);
	}

	if(recvfrom(sockfd,(struct message*)&login_message_reply,sizeof(struct message),0,&addr_recv,&temp)==-1){
		perror("receive eorror!");
		exit(1);
	}
	if(login_message_reply.message_type == 3){
		printf("login success!\n");
		change_username(username);
		return 0;
	}
	else{
		printf("login failed!\n");
		printf("please try again！\n");
		choose_register_or_login(); //返回选择注册还是登陆的界面
	}
	
	return 0;
}

//更改time：2020/12/4 解决端口冲突问题 ---------利用ifreq结构体
//来自网上查找到的相关资料，对这里的问题可以很好的解决（非原创）：
int init_ip_and_port(){
	int new_client_port;
	struct sockaddr_in *new_client_addr;

/*
	在Linux系统中，ifconfig命令是通过ioctl接口与内核通信，例如，当系统管理员输入如下命令来改变接口eth0的MTU大小：

    ifconfig eth0 mtu 1250

	ifconfig命令首先打开一个socket，然后通过系统管理员输入的参数初始化一个数据结构，并通过ioctl调用将数据传送到内核。SIOCSIFMTU是命令标识符。

    struct ifreq data;
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    < ... initialize "data" ...>
   err = ioctl(fd, SIOCSIFMTU, &data);
*/

	//该部分借鉴网络上的一种socket分配方案，可以很好的解决本作品的问题。
	struct ifreq req;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))== -1)
	{
		perror("error!");
		exit(1);
	}
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(server_port);

	memset(&client_addr,0,sizeof(client_addr));
	client_addr.sin_family = AF_INET;

	strcpy(req.ifr_name, "lo");

	if (ioctl(sockfd, SIOCGIFADDR, &req) < 0 ) 
	{
		perror("error！");
		exit(1);
        } 
	new_client_addr = (struct sockaddr_in*)&(req.ifr_addr);
	memcpy(&client_addr, (struct sockaddr_in*)&(req.ifr_addr),sizeof(struct sockaddr_in));
	new_client_port = port;
	do {
		new_client_port++;
		client_addr.sin_port = htons(new_client_port);
		bind(sockfd, (struct sockaddr*)&client_addr,sizeof(struct sockaddr)); 		
	} while (errno == EADDRINUSE);
}

//在客户端选择功能的同时，采用循环模型->一直监听本地socket->接收来自其他socket的消息
int waiting_for_chat(){
	int temp_userid;
	int temp_userid_2;
	int temp;
	temp = sizeof(struct sockaddr);
	int count; //计数消息字符
	struct sockaddr client_addr;
	struct message chat_message;
	printf("(You can be chatted by others now !)\n");
	while(1){
		if(recvfrom(sockfd,&chat_message,sizeof(struct message),0,&client_addr,&temp)==-1){
			perror("error!");
			exit(1);
		}
		else{
			if(chat_message.message_type==1){
			printf("User:%s(userid:%d)send a message to you!\nThe message is %s:",chat_message.message_username,chat_message.userid_src,chat_message.message_content);
			}
			else if(chat_message.message_type==4){ //传输文件
				//printf("%s\n",chat_message.message_filename);
				//printf("1\n");
				FILE * fp=fopen(chat_message.message_filename,"w+");
				if(fp == NULL){
					printf("Open file failed!\n");
					exit(1);
				}
				fwrite(chat_message.message_content,sizeof(chat_message.message_content),1,fp);
				printf("Download file:%s successfully!\n",chat_message.message_filename);
				fclose(fp);
				select_function();
			}
		}
	}
}

int main(){
	init_ip_and_port();
	choose_register_or_login();
	pid_t pid;
	pid=fork();
	if(pid==-1){
		perror("error!\n");
		exit(1);
	}
	else if(pid==0){      //pid=0 ------->子进程
		waiting_for_chat(); //在完成输入命令选项的同时，一直循环监听本socket-->接收来自其他用户的信息。
	}
	else{ 				 //pid>0 ------>父进程
		//show_functions();
		select_function();
	}
}
