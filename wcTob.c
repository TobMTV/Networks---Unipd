#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct header{
char * n;
char * v;
}h[100];

int s,j=0,k=0,i,bodyLen,n;
struct sockaddr_in server;
unsigned char ip[4]={88,80,187,84};
char request[10000];
char response[20000];
char body[2000000];

void socketConf(int port){
    s=socket(AF_INET, SOCK_STREAM, 0);
        if(s==-1)
            printf("Failed socket..");

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = *(unsigned int *)ip;

    if (connect(s, (struct sockaddr *)&server,sizeof(struct sockaddr_in))==-1){
    perror("Connect fallita..");
    }
}

int writeRequest(){
    
    sprintf(request,"GET / HTTP/1.0\r\nConnection:keep-alive\r\n\r\n");
    if(write(s,request,strlen(request))==-1)
        return 0;

}

int main(){
    
    socketConf(80);
    if(writeRequest()==0)
        printf("Write error..");
    
    h[k].n = response;
    while(read(s,response+j,1)){
    //printf("%c",response[j]);
    if(response[j]=='\n' && response[j-1]=='\r'){
        //printf("LF\n");
        response[j-1]= 0; // metto un identificatore per capire quando c'e' un /r/n
        if(h[k].n[0]==0) break; //se la risposta e' finita -->esci
        h[++k].n = response+j+1; // Salvo la parte di response che si trova dopo j+1
    }
    if(response[j] == ':'){
        //printf("Due Punti\n");
        response[j]=0;
        h[k].v = response+j+1; //Salvo la parte di response che si trova dopo j+1
    }
    j++;
    }

    printf("Command-Line: %s\n", h[0].n);
    printf("===========HEADERS============\n");
    for (i=1;i<k;i++){
        printf("%s ==> %s\n", h[i].n, h[i].v);
        if(!strcmp(h[i].n,"Content-Length")) // cerco content length per salvarmerlo
            bodyLen = atoi(h[i].v);
       
    }
    printf("==========BODY================\n");
    printf("La lunghezza e' %d\n", bodyLen);
    for(j=0;n=read(s,body+j,bodyLen-j);j+=n){ //salvo il body nella apposita sezione per poi printarlo, lo faccio decrementando ogni carattere che leggo dal bodylen
	if (n==-1) {
        perror("read fallita");return 1;
        }

    }
    while (i<bodyLen){
        printf("%c",body[i]);
        i++;
    }

    close(s);
    return 1;
}