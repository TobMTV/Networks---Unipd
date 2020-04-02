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

int s,j=0,k=0,i,bodyLen,n,size,t,q,letti;
struct sockaddr_in server;
unsigned char ip[4]={216,58,212,99};
char request[10000];
char response[20000];
char body[2000000];
int chunked = 0;

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
    
    sprintf(request,"GET / HTTP/1.1\r\nHost:www.google.it\r\n\r\n");
    if(write(s,request,strlen(request))==-1)
        return 0;

}
void stampaBody(int i){
  while (i<bodyLen){
        printf("%c",body[i]);
        i++;
    }
}

int main(){
    
    socketConf(80);
    if(writeRequest()==0)
        printf("Write error..");
    
    h[k].n = response;
    while(read(s,response+j,1)){ //Tutta questa parte la esegue sempre per l'HEADER dei messaggi
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
        if(!strcmp(h[i].n,"Transfer-Encoding") && !strcmp(h[i].v," chunked")){ // capisco se la response e' divisa in chunk
            chunked = 1;
        }
       
    }
    printf("==========BODY================\n");
    
    if(chunked == 0){ //Se si tratta di una richiesta non chunked la gestisco con una read classica
    printf("La lunghezza e' %d\n", bodyLen);
    for(j=0;n=read(s,body+j,bodyLen-j);j+=n){ //salvo il body nella apposita sezione per poi printarlo, lo faccio decrementando ogni carattere che leggo dal bodylen
	if (n==-1) {
        perror("read fallita");return 1;
        }

    }
    stampaBody(i);
    }
    else{ // se mi arrivano messaggi chunked
    j++;// J sta mantenendo il valore che aveva quando ha scannato l'header, con questo ++ lo porto sotto per partire con il body del chunk
    t=j;
       while(read(s, response + j, 1)){
           if(response[j]=='\n' && response[j-1] == '\r'){
            response[j-1]= 0;
            
            size = (int) strtoul(&response[t], NULL, 16); //Cast in (int) di string(size del chunk in hexadecimal)
            printf("Size: %d\n", size);
            if (!size) break;
            letti = 0;
            while(letti = read(s, body + q, size)) { //leggo i chunk e rimuovo la parte letta dal totale, legge finche' il size !=0 (ultimo chunk)
                printf("Letti: %u\n", letti);   
                size = size - letti;
                printf("New size: %u\n", size);
                q += letti;
                printf("Totale letto %u\n", q);
                
            }
            read(s, response, 2);
            if((response[1]!='\n') || (response[0]!='\r')){ printf("Errore di formato chunk!\n"); return 1;} //CHECK CRLF
        t=j=0;
           }
        j++;
       }
       printf("FULL BODY:\n\n %s", body);

    }

    close(s);
    return 1;
}