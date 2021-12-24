/* 服务端server.c 源码
	作者：Maxx
	学号：181802xxxxx
	学院：网络与信息安全学院
	专业：网络工程
	时间：2020/10/25 ~ 2020/12/?
*/
#include <stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <signal.h>
#include <ncurses.h>
#include <netdb.h>
#include <math.h>
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
#define server_port 9999
#define MAXFILENAME 500
#define BUFFERSIZE 1000
#define MAXFILENAME 500

struct sockaddr_in server_addr;
int sockfd;
int userList_fd;

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

struct user
{
	char username[20];
	char password[20];
	int userid;
	struct sockaddr user_addr;	
};

int start_on(){
	//一系列初始化
	//int fd_temp;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))==-1){
		perror("can't create socket\n");
		exit(1);
	}
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(server_port);
	struct user user;
	memset((struct user*)&user, '\0', sizeof(struct user));
	//创建用户列表文件
	//printf("1");
	userList_fd=open("./userList.txt",O_RDWR|O_CREAT);
	if(userList_fd==-1){
	//	printf("2");
		error("error!\n");
		exit(1);
	}
	if(lseek(userList_fd,0,SEEK_SET)==-1){
		perror("error!\n");
		exit(1);
	}
	if(write(userList_fd, (char*)&user, sizeof(struct user))==-1){
		//printf("4");
		perror("error!\n");
		exit(0);
	}
	//绑定端口：
	if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))==-1){
		perror("bind error!");
		exit(1);
	}
	return 0;
}

struct message
{	
	int message_type; //message_type=1：普通的消息；message_type=2：表示注册信息的消息；message_type=3：表示登录信息的消息；message_type=4：表示文件传送的消息;
	char message_content[MAXDATASIZE]; //消息的具体内容
	int userid_src; //消息的发送者id
	int userid_dst; //消息的接收者id
	char message_username[20];//消息的发送者姓名（！!待议！!）
	char message_filename[20];
};

int handle_request(){
	struct message handle_message;
	struct sockaddr handle_addr;
	printf("Server is already , and listening now ...\n");
	int temp = sizeof(struct sockaddr);
	while(1){
		memset(&handle_message,0,sizeof(struct message));
		if(recvfrom(sockfd,&handle_message, sizeof(struct message),0,&handle_addr,&temp)==-1){
			perror("error!\n");
			exit(1);
		}
		printf("A request is coming!\n");
		//更改time 2020/12/1  主要思想同客户端（该部分非原创）

		//printf("%s",handle_message.message_content);
		//printf("%d",handle_message.message_type);

		switch (handle_message.message_type)
		{
			case 1 :
				delivery_message(&handle_message,(struct sockaddr*)&handle_addr);
				break;
			case 2 :
				register_user(&handle_message, (struct sockaddr*)&handle_addr);
				break;			
			case 3 :
				user_login_check(&handle_message, (struct sockaddr*)&handle_addr);
				break;
			case 4 :
				transfer_file_to_user(&handle_message, (struct sockaddr*)&handle_addr);
			default :
				break;
		}
	}
	return(0);
	}

//校验用户登陆功能
int user_login_check(struct message *login_message, struct sockaddr *addr){
	printf("someone is logining now!\n");
	struct user user;
	//在服务端校验登陆成功的同时记录该条登录信息，为客户端查询在线用户功能提供文件信息输入。
	FILE * user_online_fd;
	user_online_fd=fopen("online_user_list.txt","a+");
	//搜索文件 查找这个用户id所在的文件位置
	int temp=sizeof(struct user);
	if(lseek(userList_fd,(login_message->userid_src)*temp,SEEK_SET)==-1){
		perror("error!\n");
		exit(1);
	}
	//读取文件，将该行信息读入到user的结构体中
	if(read(userList_fd,&user,sizeof(struct user))==-1){
		perror("error!\n");
		exit(0);
	}
	printf("Username:%s ",user.username);
	if(strcmp(login_message->message_content,user.password)==0){ //验证成功，并且服务端给该客户端发送成功的消息。
		if(lseek(userList_fd,-(sizeof(struct user)),SEEK_CUR)==-1){
			perror("error!\n");
			exit(1);
		}	
		//保存该用户的地址信息
		user.user_addr= *addr;
		if(write(userList_fd, &user,sizeof(struct user))==-1){
			perror("error!\n");
			exit(0);
		}
		//给该客户端用户发送验证成功的信息：
		if(sendto(sockfd, (struct message*)login_message, sizeof(struct message),0,&(user.user_addr),sizeof(struct sockaddr))==-1){
			perror("serror!\n");
			exit(0);
		}
  }
  else{ //验证失败,需要通知客户端
			printf("UserID %d login failed.\n",login_message->userid_src);
			login_message->message_type = 0; //客户端接收需要查验的参数设置为0
			memset(&login_message->message_content,0,MAXDATASIZE);		
			if(sendto(sockfd, (struct message*)login_message, sizeof(struct message), 0,&(user.user_addr), sizeof(struct sockaddr))==-1){
				perror("error!\n");
				exit(0);
			}
			return(1);
	}
	printf("UesId:%d login sucess!\n",login_message->userid_src);
	//将登陆成功的这个用户信息写入到user_online_fd文件中
	fputs(user.username,user_online_fd);
	fputc('\n',user_online_fd);
	fclose(user_online_fd);
	return(0);
}

//注册用户功能
int register_user(struct message *register_message, struct sockaddr *addr){
	struct user user;
	printf("someone is registing now!\n");
	//printf("%s\n",register_message->message_content);
	char *username;
	username=&(register_message->message_content[0]);
	//printf("1");
	char *password;
	password=&(register_message->message_content[20]);
	//在
	int struct_len=sizeof(struct user);
	//printf("0");

	if(lseek(userList_fd,-struct_len,SEEK_END)==-1){
		perror("lsek error!\n");
		exit(1);
	}
	//printf("1");
	//读取文件写入，并且将message字段信息读入到user的结构体中
	if(read(userList_fd,&user,sizeof(struct user))==-1){
		perror("error!\n");
		exit(0);
	}
	int temp3=sizeof(struct sockaddr);
	memcpy(&(user.user_addr),addr,temp3);
	strcpy(user.username,username);
	//printf("%s\n",user.username);
	strcpy(user.password, password);
//	printf("%s",);
	//printf("%s\n",user.username);
	//printf("%s\n",user.password);
	user.userid+=1;
	//printf("1");
	if(lseek(userList_fd,0,SEEK_END)==-1){
		perror("error!\n");
		exit(1);
	}
	//将user信息写入userlist.txt文件
	if(write(userList_fd,&user,sizeof(struct user))==-1){
		perror("error!\n");
		exit(0);
	}
	//printf("2");
	register_message->userid_dst=user.userid;
	//发送注册成功的消息返回给客户端
	if(sendto(sockfd, (struct message*)register_message, sizeof(struct message), 0,&(user.user_addr), sizeof(struct sockaddr))==-1){
		perror("error!\n");
		exit(0);
	}
	//printf("3");
	printf("UserID：%d regist success！\n",register_message->userid_dst);
	printf("用户ID为：%d\n",user.userid);
	printf("用户名为：%s\n",user.username);
	printf("密码为：%s\n",user.password);
	return 0;
}

int show_server_function(){
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
	printf("|~~~~~~~~~~~~~Welcome to the XDU Chatroom Server~~~~~~~~~~~~~~~~|\n");
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
	printf("|~~~~~~~function_No.1: send system message to user(select 1)~~~~|\n"); //输入字符1：发送系统信息给指定用户功能
	printf("|~~~~~~~function_No.2: show online users(select 2)~~~~~~~~~~~~~~|\n"); //输入字符2：查看目前在线的用户
	printf("|~~~~~~~function_No.3: exit the system(select 3)~~~~~~~~~~~~~~~~|\n");//输入字符3：退出服务器系统
	printf("|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|\n");
	char selection_character;
	while(1){
	printf("Your selection:");
	selection_character=getchar();
	switch(selection_character){
		case '1':
			send_system_message();
		//show_server_function();
			break;
		case '2':
			show_online_users();
		//	show_server_function();
			break;
		case '3':
			exit_system();
	}
	}
}
//发送系统信息给指定用户
int send_system_message(){
	//unfinished
	return 0;
}

//服务端转发来自客户端的消息信息->给dst客户端id转发
int delivery_message(struct message *chat_message, struct sockaddr *addr){
	struct user user1;
	struct user user2;
	int dst_client_id=chat_message->userid_dst;
	int src_client_id=chat_message->userid_src;
	int temp1=sizeof(struct user);
	int temp2=sizeof(struct message);
	int temp3=sizeof(struct sockaddr);
	printf("UserID:%d is chatting to the UserID:%d\n",chat_message->userid_src,chat_message->userid_dst);
	//printf("1");
	if(lseek(userList_fd,src_client_id*temp1,SEEK_SET)==-1){
		perror("error!\n");
		//printf("1");
		exit(1);
	}
	//printf("1");
	if(read(userList_fd,&user1,temp1)==-1){
		perror("error!\n");
		//printf("1");
		exit(1);
	}
	strncpy(chat_message->message_username,user1.username,20);
	//printf("2");
	if(lseek(userList_fd,dst_client_id*temp1,SEEK_SET)==-1){
		perror("error!\n");
		//printf("1");
		exit(1);
	}
	//printf("1");
	if(read(userList_fd,&user2,temp1)==-1){
		perror("error!\n");
		//printf("1");
		exit(1);
	}
	if(sendto(sockfd,chat_message,temp2,0,&(user2.user_addr),temp3)==-1){
		perror("error!\n");
		//printf("1");
		exit(1);
	}

	printf("UserID:%d send message:%s to the UserID:%d Successfully!\n",chat_message->userid_src,chat_message->message_content,chat_message->userid_dst);
	return 0;
}

//文件传输功能
int transfer_file_to_user(struct message *filename_message, struct sockaddr *addr){
	FILE * fp =NULL;
	char buffer[BUFFERSIZE];//用来缓存文件内容
	char filename[MAXFILENAME]; //用来存储文件名
	strncpy(filename,filename_message->message_content,sizeof(filename_message->message_content));//
	//bzero(filename_message->message_content,sizeof(filename_message->message_content));
	int temp1=sizeof(struct user);
	int temp2=sizeof(struct message);
	int temp3=sizeof(struct sockaddr);
	int dst_client_id=filename_message->userid_src; //返回给发送该条消息的客户端 id_src
	struct user user;
	if(lseek(userList_fd,dst_client_id*temp1,SEEK_SET)==-1){
		perror("error!\n");
		exit(1);
	}
	//比较笨拙的方法->获取客户端的socket地址。类似上面的一些子函数的做法。
	if(read(userList_fd,&user,temp1)==-1){
		perror("error!\n");
		exit(1);
	}
	//读取该文件的内容并赋值给buffer缓存中
	fp=fopen(filename_message->message_content,"r");
	fgets(buffer,sizeof(filename_message->message_content),(FILE*)fp);
	strncpy(filename_message->message_content,buffer,sizeof(filename_message->message_content));
	strncpy(filename_message->message_filename,filename,sizeof(filename_message->message_filename));
	printf("file:%s  content:%s\n",filename_message->message_filename,filename_message->message_content);
	filename_message->message_type=4; 	//ACK
	
	if(sendto(sockfd,(struct message*)filename_message,temp2,0,&(user.user_addr),temp3)==-1){
		perror("error!\n");
		exit(1);
	}
	printf("Transfer file:%s successfully!",filename);
	return 0;
}

//显示在线用户功能
int show_online_users(){
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

int exit_system(){
	close(sockfd);
	close(userList_fd);
	printf("exit the system,bye!\n");
	kill(0, SIGABRT);
	exit(0);
}

int main()
{	
	//printf("1");
	start_on();
	//printf("2");
	pid_t pid;
	pid=fork();
	/*
	switch (pid)
	{
		case -1 :
			perror("fork error\n");
			exit(1);
			break;
		case 0 : //pid=0 ------->子进程
			handle_request();
			break;
		default : // pid>0 ------>进程
			show_server_function();
			break;
	}
*/
	if(pid==-1){
	    perror("error!\n");
		exit(1);
	}
	else if(pid==0){      //pid=0 ------->子进程
		handle_request(); //在完成输入命令选项的同时，一直循环监听本socket-->接收来自其他用户的信息。
	}
	else{ 				 //pid>0 ------>父进程
		//show_functions();
		show_server_function();
	}
	return(0);
}
