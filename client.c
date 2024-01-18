/************************************************************************************************/
/*                                 Electronic Dictionary Client                                 */
/************************************************************************************************/

#include "dic.h"
void error_Handling(char* func,int retval);
void do_register(int sockfd,MSG_t* pbuf);
int  do_login(int sockfd,MSG_t* pbuf);
void do_query(int sockfd,MSG_t* pbuf);
void do_history(int sockfd,MSG_t* pbuf);
void do_exit(int sockfd,MSG_t* pbuf);
void dictionary(int sockfd,MSG_t* pbuf);
int transform(char* choose);

int main(int argc,char* argv[])
{
	if(argc != 3)
	{
		printf("%s--IP--Port\n",argv[0]);
		exit(1);
	}
	/*创建套接字*/
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	error_Handling("socket",sockfd);

	/*连接*/
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
	int ret_connect = connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
	error_Handling("connect",ret_connect);
	MSG_t message = {0,0,0};

	while(1)
	{
		system("clear");
		printf("**********************************************************************\n");
		printf("*                    NewFuture Electronic Dictionary                 *\n");
		printf("*                                              Version :  0.0.1      *\n");
		printf("*           1: register           2: login            3: exit        *\n");
		printf("**********************************************************************\n");
		printf("please choose a mode: ");
		char choose[100];
		scanf("%s", choose);
		
		switch (transform(choose))
		{
			case 1:
				do_register(sockfd,&message);
				break;
			case 2:
				if(do_login(sockfd,&message))
				{
					dictionary(sockfd,&message);
				}
				break;
			case 3:
				do_exit(sockfd,&message);
				exit(0);
			default:
				{
					printf("Message:无效的输入\n");				
					break;
				}
		}
		sleep(1);
	}
}
void do_register(int sockfd,MSG_t* pbuf)
{
	pbuf->type = R;
	printf("\nInput User Name     :");
	scanf("%s",pbuf->name);
	printf("Input  Password     :");
	scanf("%s",pbuf->data);
	ssize_t ret_write = write(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
	ssize_t ret_read = read(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("read",ret_read);
	puts(pbuf->data);
}
int do_login(int sockfd,MSG_t* pbuf)
{
	pbuf->type = L;
	printf("\nInput User Name     :");
	scanf("%s",pbuf->name);
	printf("Input  Password     :");
	scanf("%s",pbuf->data);
	ssize_t ret_write = write(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
	ssize_t ret_read = read(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("read",ret_read);
	puts(pbuf->data);
	sleep(1);
	if(pbuf->type == -1)
	{
		return 0;
	}
	return 1;
}
void dictionary(int sockfd,MSG_t* pbuf)
{
	while(1)
	{
		system("clear");
		printf("**********************************************************************\n");
		printf("*                    NewFuture Electronic Dictionary                 *\n");
		printf("*                                              Version :  0.0.1      *\n");
		printf("*       1: Words_Query         2: History_Reco          3: exit      *\n");
		printf("**********************************************************************\n");
		printf("please choose a mode: ");
		char choose[1000];
		scanf("%s", choose);
		switch (transform(choose))
		{
			case 1:
				do_query(sockfd,pbuf);	
				break;
			case 2:
				{
					do_history(sockfd,pbuf);
				}				
					break;
			case 3:
				do_exit(sockfd,pbuf);
				exit(0);
			default:
				{
					printf("Message:无效的输入\n");
					sleep(1);
				}
				break;
		}
	}
}

void do_query(int sockfd,MSG_t* pbuf)
{
	pbuf->type = Q;
	while(1)
	{
		printf("Input words (quit by '#') :");
		scanf("%s",pbuf->data);
		if(strcmp(pbuf->data,"#") == 0)
			break;
		ssize_t ret_write = write(sockfd,pbuf,sizeof(MSG_t));
		error_Handling("write",ret_write);
		ssize_t ret_read = read(sockfd,pbuf,sizeof(MSG_t));
		error_Handling("read",ret_read);
		putchar('\n');
		puts(pbuf->data);
	}
}
void do_history(int sockfd,MSG_t* pbuf)
{
	pbuf->type = H;
	ssize_t ret_write = write(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
	ssize_t ret_read;
	int i = 0;
	while( ret_read = read(sockfd,pbuf,sizeof(MSG_t)) )
	{
		error_Handling("read",ret_read);
		if( strcmp(pbuf->data,"end") == 0)
			break;
		printf("%s    ",pbuf->data);
		i++;
		if(i%3 == 0)
			printf("\n");
	}
	printf("Message:按回车返回上一层\n");
	getchar();
	getchar();
}

int transform(char* choose)
{
	if(!strcmp(choose,"1"))
		return 1;
	else if(!strcmp(choose,"2"))
		return 2;
	else if(!strcmp(choose,"3"))
		return 3;
	else 
		return 0;
}

void error_Handling(char* func,int retval)
{
	if(retval == -1)
	{
		perror(func);
		exit(1);
	}
}

void do_exit(int sockfd,MSG_t* pbuf)
{
	pbuf->type = E;
	ssize_t ret_write = write(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("write",ret_write);
	ssize_t ret_read = read(sockfd,pbuf,sizeof(MSG_t));
	error_Handling("read",ret_read);
	puts(pbuf->data);
}
