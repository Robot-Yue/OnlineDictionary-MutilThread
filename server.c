/************************************************************************************************/
/*                                 Electronic Dictionary Server                                 */
/************************************************************************************************/

#include "dic.h"
void error_Handling(char* func,int retval);
void break_handling(int retval,char* IP,int fd,MSG_t* pbuf);
void do_register(int newconfd,MSG_t* pbuf);
void do_login(int newconfd,MSG_t* pbuf);
void do_query(int newconfd,MSG_t* pbuf);
void do_history(int newconfd,MSG_t* pbuf);
void do_exit(int newconfd,MSG_t* pbuf);
void recordInQueryLog(MSG_t* pbuf);
void recordInloginLog(MSG_t* pbuf,char* operation);
void createDb_table();
int checkLogout(MSG_t* pbuf,int flag);

int count,row,col,ret;
sqlite3* db = NULL;
char sql[100] = {};
char** result;

void* dealClient(void* p)
{
	pthread_detach(pthread_self());
	int newconfd = (*(ThreadParam_t*)p).fd;
	
	time_t t = time(NULL);
	printf("            -------------New connection information-------------\n\n");
	printf("      System time                        :  %s\n",ctime(&t));
	printf("      Establish a connection with the IP :   %s\n",(*(ThreadParam_t*)p).IP);
	printf("      New connected sockfd               :   %d\n",newconfd);
	printf("      Processing ThreadID                :   %lu\n",pthread_self());
	printf("      The number of currently connected  :   %d\n\n\n",count);

	MSG_t message = {0,0,0};
	while(1)
	{
		ssize_t ret_read = read(newconfd,&message,sizeof(MSG_t));
		break_handling(ret_read,(*(ThreadParam_t*)p).IP,newconfd,&message);

		switch(message.type)
		{
			case R:
				do_register(newconfd,&message);
				break;
			case L:
				do_login(newconfd,&message);
				break;
			case Q:
				do_query(newconfd,&message);
				break;
			case H:
				do_history(newconfd,&message);
				break;
			case E:
				do_exit(newconfd,&message);
				break;
		}
	}
}

int main(int argc,char* argv[])
{
	if(argc != 2)
	{
		printf("%s--Port\n",argv[0]);
		exit(1);
	}
	/*创建数据库，用户信息表，历史记录表*/
	createDb_table();


	/*创建套接字——监听套接字*/
	int listenfd = socket(AF_INET,SOCK_STREAM,0);
	error_Handling("socket",listenfd);

	/*绑定监听套接字与主机网络地址信息*/
	int on = 1;
	int ret_set = setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	error_Handling("setsockopt",ret_set);
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1]));
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret_bind = bind(listenfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
	error_Handling("bind",ret_bind);

	/*设置监听队列的大小*/
	int ret_listen = listen(listenfd,10);
	error_Handling("listen",ret_listen);

	/*监听等待连接*/
	struct sockaddr_in buf_addr;
	socklen_t buf_addrlen = sizeof(buf_addr);
	pthread_t tid;
	printf("            -------------The server is running-------------\n\n");
	while(1)
	{
		int newconfd = accept(listenfd,(struct sockaddr*)&buf_addr,&buf_addrlen);
		error_Handling("accept",newconfd);
	
		ThreadParam_t param;
		param.fd = newconfd;
		strcpy(param.IP,inet_ntoa(buf_addr.sin_addr));
		pthread_create(&tid,NULL,dealClient,&param);

		count++;
	}

}
void createDb_table()
{
	/*创建库*/
	ret = sqlite3_open("usrInfo.db",&db);
	if(ret != 0)
	{
		perror("sqlite3_open");
		exit(1);
	}
	/*创建表*/
	char* sql = "create table accountInfo(usr_name varchar(20),password varchar(20))";
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
	if(ret != 0)
	{
		printf("Server_Infomation: %s\n",sqlite3_errmsg(db));
	}
	sql = "create table queryLog(usr_name varchar(20),words varchar(20),time varchar(40))";
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
	if(ret != 0)
	{
		printf("Server_Infomation: %s\n",sqlite3_errmsg(db));
	}
	sql = "create table loginLog(usr_name varchar(20),operation varchar(10),time varchar(40))";
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
	if(ret != 0)
	{
		printf("Server_Infomation: %s\n",sqlite3_errmsg(db));
	}
	
	printf("Server_Infomation: The database and tables are ready\n");
}
void do_register(int newconfd,MSG_t* pbuf)
{
	sprintf(sql,"select * from accountInfo where usr_name = '%s'",pbuf->name);
	ret = sqlite3_get_table(db,sql,&result,&row,&col,NULL);
	if(row > 0)
	{
		strcpy(pbuf->data,"Message:账号已存在，重新注册！");
		ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
	}
	else
	{
		bzero(sql,sizeof(sql));
		sprintf(sql,"insert into accountInfo values('%s','%s')",pbuf->name,pbuf->data);
		ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
		strcpy(pbuf->data,"Message:注册成功！");
		ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
	}
}
void do_login(int newconfd,MSG_t* pbuf)
{
	sprintf(sql,"select * from accountInfo where usr_name = '%s' and password = '%s'",pbuf->name,pbuf->data);
	sqlite3_get_table(db,sql,&result,&row,&col,NULL);
	if(row == 0)
	{
		strcpy(pbuf->data,"Message:账号不存在或密码错误！");
		pbuf->type = -1;
		ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
	}
	else if(row == 1)
	{
		if( checkLogout(pbuf,1) )
		{
			strcpy(pbuf->data,"Message:该账号已在另外一台设备上登录！");
			pbuf->type = -1;
		}
		else
		{
			recordInloginLog(pbuf,"login");	
			strcpy(pbuf->data,"Message:登录成功！");
		}	
		ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
	}
}
void do_query(int newconfd,MSG_t* pbuf)
{
	FILE *fp = fopen("dict.txt","r");
	if(fp == NULL)
	{
		perror("open");
		exit(-1);
	}
	char buf[100] = "\0";
	char word[50] = "\0";
	int flag = 0;
	while(fgets(buf,sizeof(buf),fp)!=NULL)
	{
		bzero(word,sizeof(word));
		int i = 0;
		while(buf[i]>='a'&&buf[i]<='z')
		{
			word[i] = buf[i++];
		}
		if(strcmp(word,pbuf->data)==0)
		{
			flag = 1;
			break;
		}
	}
	if(!flag)
	{
		strcpy(pbuf->data,"Message:Sorry,Word Not found");
	}
	else
	{
		recordInQueryLog(pbuf);
		strcpy(pbuf->data,buf);
	}	
	fclose(fp);
	ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
}
void do_history(int newconfd,MSG_t* pbuf)
{
	sprintf(sql,"select * from queryLog where usr_name = '%s'",pbuf->name);
	ret = sqlite3_get_table(db,sql,&result,&row,&col,NULL);
	int i;
	for(i=0;i<(row+1)*col;i++)
	{
		strcpy(pbuf->data,result[i]);
		ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
	}
	strcpy(pbuf->data,"end");
	ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
}

void recordInloginLog(MSG_t* pbuf,char* operation)
{
	char time_q[40];
	time_t t = time(NULL);
	sprintf(time_q,"%s",ctime(&t));
	sprintf(sql,"insert into loginLog values('%s','%s','%s')",pbuf->name,operation,time_q);
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
}

void recordInQueryLog(MSG_t* pbuf)
{
	char time_q[40];
	time_t t = time(NULL);
	sprintf(time_q,"%s",ctime(&t));
	sprintf(sql,"insert into queryLog values('%s','%s','%s')",pbuf->name,pbuf->data,time_q);
	ret = sqlite3_exec(db,sql,NULL,NULL,NULL);
}

int checkLogout(MSG_t* pbuf,int flag)
{
	if(strlen(pbuf->name) == 0)  
		return 0;
	sprintf(sql,"select * from loginLog where usr_name = '%s'",pbuf->name);
	ret = sqlite3_get_table(db,sql,&result,&row,&col,NULL);

	if(row == 0)	             
		return 0;

	if(strcmp(result[(row+1)*col-2],"login") == 0)
	{	
		if(flag)
			return 1;
		recordInloginLog(pbuf,"logout");
	}
	return 0;
}
void break_handling(int retval,char* IP,int fd,MSG_t* pbuf)
{
	if(retval<0)
	{
		perror("read");
	}
	if(retval == 0)
	{
		checkLogout(pbuf,0);
		count--;
		time_t t = time(NULL);
		printf("            -------------New Disconnect information-------------\n\n");
		printf("      System time                        :  %s\n",ctime(&t));
		printf("      Disconnected to IP                 :  %s\n",IP);
		printf("      The socket is closed               :  %d\n",fd);
		printf("      ThreadID is terminated             :  %lu\n",pthread_self());
		printf("      The number of currently connected  :  %d\n\n",count);

		pthread_exit(NULL);
	}
}
void do_exit(int newconfd,MSG_t* pbuf)
{
	strcpy(pbuf->data,"Message:退出成功!");
	ssize_t ret_write = write(newconfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
}
void error_Handling(char* func,int retval)
{
	if(retval == -1)
	{
		perror(func);
		exit(1);
	}
}
