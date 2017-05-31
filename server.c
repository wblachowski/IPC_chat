#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
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
struct group{
    char name[MAX_NAME_LENGTH];
    char users[MAX_USERS][MAX_NAME_LENGTH];
};
struct user{
    char name[MAX_NAME_LENGTH];
    int queue;
};
void process_cmd();
void send_msg();
struct command CMD;
struct message MSG;
struct group groups[MAX_GROUPS];
struct user users[MAX_USERS];
char room_from[MAX_NAME_LENGTH];
int main(){
    int i=0;
    int sqid=msgget(IPC_PRIVATE,0622);
    if(sqid<0){
   perror("Blad tworzenia kolejki serwera");
      exit(0);
    }
    //wypisz id kolejki serwera
    printf("ID kolejki serwera: %d\n",sqid);
    //czytaj z kolejki serwera
   while(1){

       if(msgrcv(sqid,&CMD,2*MAX_MESSAGE_LENGTH+MAX_NAME_LENGTH,0,0)<0){
            perror("Blad przy odczycie");
        }else{
            process_cmd();

        }	

    }
    //sprzatanie
    if(msgctl(sqid,IPC_RMID,NULL)<0){
            perror("Blad przy usuwaniu kolejki klienta");
    }
    return 0;
}
void process_cmd(){
    int temp=0,pos=0;//zmienne pomocnicze
    char tospace[MAX_MESSAGE_LENGTH];
    char afterspace[MAX_MESSAGE_LENGTH];
    tospace[0]=0;
    afterspace[0]=0;
    int i=0;
    while(CMD.data[i]!=' ' && CMD.data[i]!='\0'){
        tospace[i]=CMD.data[i];
        i++;
    }
    tospace[i]='\0';
    if(CMD.data[i]==' '){
        i++;
        while(CMD.data[i]!='\0'){
            afterspace[temp]=CMD.data[i];
            i++;
            temp++;
        }
        afterspace[temp]='\0';
    }
    //logowanie
    if(CMD.mtype==2){
        pos=6;
        temp=0;
        char queue_string[32];
	queue_string[0]=0;
        int queue_nr;
        while(CMD.data[pos]!='\0'){
            queue_string[temp]=CMD.data[pos];
            temp++;
            pos++;
        }
	queue_string[temp]='\0';
        queue_nr=atoi(queue_string);
        //dodaj uzytkownika na pierwsze wolne miejsce w tablicy users
        temp=0;
        while(strcmp(users[temp].name,"") && temp<MAX_USERS)temp++;
        strcpy(users[temp].name,CMD.username);
        users[temp].queue=queue_nr;
	printf("dodalem uztkownika %s o kolejce %d\n",users[temp].name,users[temp].queue);
    }else
    if(!strcmp(tospace,"logout")){
        //konstruuj wiadomosc pozegnalna
        MSG.mtype=3;
        MSG.to_symbol='\0';
        strcpy(MSG.message,"zegnaj przyjacielu!\n");
        strcpy(MSG.to,CMD.username);
        strcpy(MSG.from,"server");
        send_msg();
        //usun uzytkownika z listy users
        temp=0;
        while(strcmp(users[temp].name,CMD.username) && temp<MAX_USERS)temp++;
        strcpy(users[temp].name,"");
        users[temp].queue=0;
        //usun uzytkownika z grup, do ktorych nalezy
        for(temp=0;temp<MAX_GROUPS;temp++){
            if(strcmp(groups[temp].name,"")){
                int i=0;
                for(i=0;i<MAX_USERS;i++){
                    if(!strcmp(CMD.username,groups[temp].users[i])){
                        strcpy(groups[temp].users[i],"");
                    }
                }
            }
        }
    }else
    if(!strcmp(tospace,"join")){
        //sprawdz czy juz istnieje taki pokoj
        temp=0;
        while(strcmp(groups[temp].name,afterspace) && temp<MAX_GROUPS)temp++;
        //znaleziono
        if(temp<MAX_GROUPS){
            //dodaj uzytkownika na pierwsze wolne miejsce
            pos=0;
            while(strcmp(groups[temp].users[pos],""))pos++;
            strcpy(groups[temp].users[pos],CMD.username);
        }
        //nie znaleziono, stworz taki pokoj, dodaj do niego uzytkownika
        else{
            temp=0;
            while(strcmp(groups[temp].name,""))temp++;
            strcpy(groups[temp].name,afterspace);
            strcpy(groups[temp].users[0],CMD.username);
        }
    }else
    if(!strcmp(tospace,"leave")){
        temp=0;
        //znajdz pokoj do opuszczenia, potem uzytkownika i go usun
        while(strcmp(groups[temp].name,afterspace) && temp<MAX_GROUPS)temp++;
        pos=0;
        while(strcmp(groups[temp].users[pos],CMD.username) && pos<MAX_USERS)pos++;
        strcpy(groups[temp].users[pos],"");
        //jesli pokoj jest pusty, usun go
        pos=0;
        while(!strcmp(groups[temp].users[pos],"") && pos<MAX_USERS)pos++;
        if(pos==MAX_USERS){strcpy(groups[temp].name,"");}
    }else
    if(!strcmp(tospace,"rooms")){
        char list[MAX_GROUPS*MAX_NAME_LENGTH];
	list[0]=0;
        pos=0;
        int i;
        //wypisz wszystkie istniejące pokoje
        for(temp=0;temp<MAX_GROUPS;temp++){
            if(strcmp(groups[temp].name,"")){
                i=0;
                while(groups[temp].name[i]!='\0'){
                    list[pos]=groups[temp].name[i];
                    pos++;
                    i++;
                }
            list[pos]='\n';
	    pos++;
            }
        }
        list[pos]='\0';
        //konstuuj wiadomosc
        MSG.mtype=1;
        MSG.to_symbol='\0';
        strcpy(MSG.message,list);
        strcpy(MSG.to,CMD.username);
        strcpy(MSG.from,"server");
        send_msg();
    }else
    if(!strcmp(tospace,"users")){
        char list[MAX_GROUPS*MAX_NAME_LENGTH];
	list[0]=0;
        pos=0;
        int i;
	for(i=0;i<MAX_GROUPS*MAX_NAME_LENGTH;i++)list[i]='\0';
        //wypisz wszystkich uzytkownikow ze struktury users
        for(temp=0;temp<MAX_USERS;temp++){
            i=0;
            if(strcmp(users[temp].name,"")){
                while(users[temp].name[i]!='\0'){
                    list[pos]=users[temp].name[i];
                    pos++;
                    i++;
                }
		list[pos]='\n';
            }
            pos++;
        }
        list[pos]='\0';
        //konstruuj wiadomosc
        MSG.mtype=1;
        MSG.to_symbol='\0';
        strcpy(MSG.message,list);
        strcpy(MSG.to,CMD.username);
        strcpy(MSG.from,"server");
        send_msg();
    }else
    if(!strcmp(tospace,"help")){
        char list[1024]="login [id_kolejki]\nlogout\n\njoin [nazwa_pokoju]\n\
leave [nazwa_pokoju]\nrooms\nusers\nhelp\n\n@[nick] [tresc]\n#[nazwa_pokoju] [tresc]\n*[tresc]\0";
        //konstruuj wiadomosc
        MSG.mtype=1;
        MSG.to_symbol='\0';
        strcpy(MSG.message,list);
        strcpy(MSG.to,CMD.username);
        strcpy(MSG.from,"server");
        send_msg();
    }else
    if(CMD.data[0]=='@'){
        temp=1;
        pos=0;
        char towhom[MAX_NAME_LENGTH];
        char message[MAX_MESSAGE_LENGTH];
        //zdobadz adresata
        while(CMD.data[temp]!=' '){
            towhom[temp-1]=CMD.data[temp];
            temp++;
        }
        //zdobadz tresc
        temp++;
        while(CMD.data[temp]!='\0'){
            message[pos]=CMD.data[temp];
            temp++;
            pos++;
        }
	message[pos]='\0';
        //zdobadz id kolejki adresata, trzeba mu wyslac wiadomosc
        temp=0;
        while(strcmp(users[temp].name,towhom) && temp<MAX_USERS)temp++;
        int towhom_id=users[temp].queue;
        //konstruuj wiadomosc do wyslania
        MSG.mtype=1;//od uztkownika
        MSG.to_symbol='@';
        strcpy(MSG.from,CMD.username);
        strcpy(MSG.to,towhom);
        strcpy(MSG.message,message);
        send_msg();
    }else
    if(CMD.data[0]=='#'){
        temp=1;
        pos=0;
        char room[MAX_NAME_LENGTH];
        char message[MAX_MESSAGE_LENGTH];
	room[0]=0;
	message[0]=0;
        //zdobadz nazwe pokoju
        while(CMD.data[temp]!=' '){
            room[temp-1]=CMD.data[temp];
            temp++;
        }
	room[temp-1]='\0';
        //zdobadz tresc
        temp++;
        while(CMD.data[temp]!='\0'){
            message[pos]=CMD.data[temp];
            pos++;
            temp++;
        }
	message[pos]='\0';
        //konstuuj wiadomosc
	strcpy(room_from,room);
        MSG.mtype=1; //od uzytkownika
        MSG.to_symbol='#';
        strcpy(MSG.from,CMD.username);
        strcpy(MSG.message,message);
        strcpy(MSG.to,"");  //adresatow wielu
        send_msg();
    }else
    if(CMD.data[0]=='*'){
        temp=1;
        //zdobadz tresc
        char message[MAX_MESSAGE_LENGTH];
        while(CMD.data[temp]!='\0'){
            message[temp-1]=CMD.data[temp];
            temp++;
        }
	message[temp-1]='\0';
        //konstruuj wiadomosc
        MSG.mtype=1;
        MSG.to_symbol='*';
        strcpy(MSG.from,CMD.username);
        strcpy(MSG.message,message);
        strcpy(MSG.to,""); //adresatow wielu (wszyscy)
        send_msg();
    }
}
void send_msg(){
    int temp,pos,i;
    //wiadomosc do jednej osoby, MSG.to nie jest puste
    if(strcmp(MSG.to,"")){
        //znajdz id kolejki do wyslania
        temp=0;
        while(strcmp(MSG.to,users[temp].name) && temp<MAX_USERS)temp++;
        int queue=users[temp].queue;
        //wysylanie wiadomosci

        msgsnd(queue,&MSG,2*MAX_NAME_LENGTH+MAX_MESSAGE_LENGTH+1,0);
    }
    //wiadomosc do pokoju lub wszystkich
    else{
        //wiadomosc do wszystkich 
        if(MSG.to_symbol=='*'){
            for(i=0;i<MAX_USERS;i++){
                if(strcmp(users[i].name,"")){
                    msgsnd(users[i].queue,&MSG,2*MAX_NAME_LENGTH+MAX_MESSAGE_LENGTH+1,0);
                }
            }
        }
        //wiadomosc do pokoju
        if(MSG.to_symbol=='#'){
            //znajdz pokoj (jest pod room_from)
            temp=0;
            while(strcmp(room_from,groups[temp].name) && temp<MAX_GROUPS)temp++;
            //dla kazdego z uzytkownikow w pokoju znajdz id kolejki i wyslij
            for(i=0;i<MAX_USERS;i++){
                if(strcmp(groups[temp].users[i],"")){
                    pos=0;
                    while(strcmp(groups[temp].users[i],users[pos].name) && pos<MAX_USERS)pos++;
                    msgsnd(users[pos].queue,&MSG,2*MAX_NAME_LENGTH+MAX_MESSAGE_LENGTH+1,0);
                }
            }
        }
    }
}
