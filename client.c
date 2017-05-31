#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#define MAX_USERS 32
#define MAX_GROUPS 32
#define MAX_NAME_LENGTH 32
#define MAX_MESSAGE_LENGTH 2048
struct command{
    long mtype;
    char data[2*MAX_MESSAGE_LENGTH];
    char username[MAX_NAME_LENGTH];
};
struct message{
    long mtype;
    char from[MAX_NAME_LENGTH];
    char to_symbol;
    char to[MAX_NAME_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
};
struct command CMD;
struct message MSG;
int main(){
	int koniec=0;
    int sqid;
    printf("Podaj id kolejki serwera:\n");
    scanf("%d",&sqid);
    char username[256];
    if(getlogin_r(username,256)<0){
        perror("Blad przy pobieraniu nazwy uzytkownika.");
        exit(0);
    }
    int cqid=msgget(IPC_PRIVATE,0622);
    if(cqid<0){
        perror("Blad tworzenia kolejki klienta.");
        exit(0);
    }
    printf("%d\n",cqid);
    char input[2*MAX_MESSAGE_LENGTH];
    char tospace[2*MAX_MESSAGE_LENGTH];
    int i;
    //pisz
    if(fork()){
        while(gets(input) && !koniec){
                if(strcmp(input,"")){
                    i=0;
                    while(input[i]!=' ' && input[i]!='\0'){
                        tospace[i]=input[i];
                        i++;
                    }
                    tospace[i]='\0';
                    if(!strcmp(tospace,"login")){
                        CMD.mtype=2;
                    }else{
                        CMD.mtype=1;
                    }
                    strcpy(CMD.data,input);
                    strcpy(CMD.username,username);
                    if(msgsnd(sqid,&CMD,2*MAX_MESSAGE_LENGTH+MAX_NAME_LENGTH,0)<0){
                        perror("Blad wysylania wiadomosci.");
                        exit(0);
                    }
                }
        }
    }
    //czytaj
    else{
        while(1)
	{
	   if(msgrcv(cqid,&MSG,2*MAX_NAME_LENGTH+MAX_MESSAGE_LENGTH+1,0,0)<0){
		    perror("Blad odczytu wiadomosci");
		    exit(0);
            }
           if(strcmp(MSG.from,"server")){printf("%c%s:%s\n",MSG.to_symbol,MSG.from,MSG.message);}else{
		printf("%s\n",MSG.message);
		}
           if(MSG.mtype==3){koniec=1;break;}


    	}
    }


    //sprzatanie
    if(msgctl(cqid,IPC_RMID,NULL)<0){
            perror("Blad przy usuwaniu kolejki klienta.");
            exit(0);
    }
    return 0;
}
