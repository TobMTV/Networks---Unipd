#include <stdio.h>
#include <sys/types.h> /* See NOTES */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int s,sc,j=0;
socklen_t lungh;
struct sockaddr_in myaddr,remoteAddr; //struct sockaddr_in {
int yes = 1;
char request[1000];
char response[1000];
//unsigned char ip[4]={127,0,0,1};  //Me stesso


struct header
{
    char *n; // name Creo dei puntatori in modo tale da salvare quanta roba voglio
    char *v; // value
} h[100];

void socketInit()
{

    s = socket(AF_INET, SOCK_STREAM, 0);                           //short            sin_family;   // e.g. AF_INET
    myaddr.sin_family = AF_INET;                                  //unsigned short   sin_port;     // e.g. htons(3490)
    myaddr.sin_port = htons(9589);                               //addr.sin_addr.s_addr = *(unsigned int *)ip;
                                                                //struct in_addr   sin_addr;     // see struct in_addr, below
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1) //char  sin_zero[8];  // zero this if you want to};
        perror("Failed Setsockopt..");

    if(bind(s, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in))==-1)
        perror("Failed Binding..");
     //assegna un nome a un socket senza nome, bindandolo con la struct addr
    if(listen(s,10)==-1) //Crea una coda per memorizzare le richieste in ingresso, 10 in questo caso
        perror("Failed Listen.."); 
    lungh=sizeof(struct sockaddr_in);
    sc = accept(s,(struct sockaddr *) &remoteAddr,&lungh); //Rimuove una richiesta dalla coda e ne aspetta una nuova
    if (sc==-1)
        perror("Failed Accept..");
}
void responseMkr(){
    sprintf(response,"HTTP/1.1 404 Not Found\r\nConncection:close\r\n\r\n");
    write(sc,response,strlen(response));
}

void requestParser(){
    
    h[0].n = request; //collego la request alla struct che ho creato, in modo da parsarla in un momento successivo
    while(read(sc,request+j,1)){ //cerca di leggere da sc(file descriptor) e scrivere in request(ovviamente devo farlo incrementare altrimenti sovrascrive sempre il primo), con l'ultimo parametro settato a 1 ritorna anche il numero di byte letti
        printf("%c", request[j]); //printf legge carattere per carattere e lo stampa a standard output
    if(request[j] == '\n' && request[j-1] == '\r' ){//Request-Line = Method SP Request-URI SP HTTP-Version CRLF
        printf("LF\n");
        request[j]= '0';
    }
    if(request[j] == '\n' && request[j-1] == '\r' && request[j-2] == '\n' && request[j-3] == '\r' )
        printf("EOR\n"); //End of request
        responseMkr();
    j++;
    }
}



int main()
{
    socketInit();
    requestParser();


    close(s);
    close(sc);
}
