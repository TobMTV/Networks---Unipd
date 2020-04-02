#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h> 
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>


struct hostent *he; //Struct that describe an host struct  hostent {
                    //    char *  h_name;     
                    //    char ** h_aliases; 
                    //    int     h_addrtype;  
                    //    int     h_length;    
                    //    char ** h_addr_list;
                    //};
int yes=1;
int pid;
unsigned char car;
struct header{
	char * n;
	char * v; }h[100];
FILE * f;
struct sockaddr_in myaddr,remote_addr;
struct sockaddr_in server;
char  filename[100],command[1000];
char request[10000];
char response[10000];
char req_server[10000];
int main()
{
char *method,*scheme, *hostname, *path, *ver, *port;
int c;
int duepunti;
int i,k,j,n,t,s,s2,s3;
int length, lungh;
s = socket(AF_INET, SOCK_STREAM,0);
if ( s == -1) {
	perror("socket fallita");
	return 1;
	}
myaddr.sin_family=AF_INET;
myaddr.sin_port=htons(7453);
if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {perror("setsockopt fallita"); return 1; }

if (bind(s,(struct sockaddr *)&myaddr,sizeof(struct sockaddr_in))==-1){ perror("bind fallita"); return 1; }
if (listen(s,10)==-1){ perror("Listen Fallita");return 1; } //S e' il socket padre, ossia colui che resta in ascolto per le connessioni incoming
lungh=sizeof(struct sockaddr_in);
while(1){
s2 = accept(s,(struct sockaddr *) &remote_addr,&lungh); //Passo la connessione su s2 in modo tale che s rimanga in ascolto
if(fork()==0){ //Se il pid ==0 (fork()==0) allora si tratta del figlio
printf("%d\n",s2);
if (s2 == -1) {perror("accept fallita"); return 1;}
j=0;k=0;
h[0].n=request;
while(read(s2,request+j,1)){ //Splittaggio dell'header con caratteri adatti al parsing successivo
	if((request[j]=='\n') && (request[j-1]=='\r')){
		//printf("LF\n");
		request[j-1]=0;
		duepunti=0;
		if(h[k].n[0]==0) break;
		h[++k].n=request+j+1;
		}
	if((request[j]==':') && (!duepunti)&&  (k!=0)){
		duepunti=1;
		request[j]=0;
		h[k].v=request+j+1;
		}	
	j++;
}
if (k==0) {close(s2);continue;} //Se non ho nulla da parsare allora chiudo s2
//perror("Read\n");
printf("Command-Line: %s\n",h[0].n);
printf("===== HEADERS ==============\n"); //Parsing dell'header
/*for(i=1;i<k;i++){
	printf("%s --> %s\n", h[i].n, h[i].v);	
}
*/
method = h[0].n;
for(i=0;h[0].n[i]!=' ';i++); h[0].n[i]=0;i++;
printf("method=%s\n",method);

if (!strcmp(method,"GET")){ //CASO SE L'HOST FA UNA RICHIESTA GET
scheme =&h[0].n[i];
for(;h[0].n[i]!=':';i++); h[0].n[i]=0; i+=3;
printf("schema:%s\n",scheme);
hostname =&h[0].n[i];
for(;h[0].n[i]!='/';i++); h[0].n[i]=0; i++;
printf("hostname:%s\n",hostname); //IP dell'host
path = &h[0].n[i];
for(;h[0].n[i]!=' ';i++); h[0].n[i]=0; i++;
ver = &h[0].n[i];
printf("REPORT:%s %s %s %s %s\n",method,scheme,hostname,path,ver);
he = gethostbyname(hostname); // compila la struct di he con i dati relativi all'host in questione(ip preso da hostname della richiesta GET)
if (he == NULL) {
	printf("gethostbyname fallita\n");
	return 1;
	}
printf("Indirizzo server: %u.%u.%u.%u\n", (unsigned char)he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2], (unsigned char)he->h_addr[3]);
s3 = socket(AF_INET, SOCK_STREAM, 0); //  Socket aperto per collegarsi al server richiesto dall'host
if (s3 == -1) { perror("socket verso server fallita"); return 1;}
server.sin_family = AF_INET;
server.sin_port = htons(80); // Mi collego al servizio sulla porta 80
server.sin_addr.s_addr=(*(unsigned int *)((*he).h_addr));
t = connect(s3,(struct sockaddr *)&server,sizeof(server)); //Cerco di connettermi al server che l'host mi ha richiesto
if (t == -1) { perror("connect fallita"); return 1;}
sprintf(req_server,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",path,hostname); //Scrivo in req_server la richiesta dell'host al server
printf("richiesta:%s\n",req_server);
write(s3,req_server,strlen(req_server)); //Richiedo al server
while(read(s3,&car,1)){ //Ricevo la risposta del server in s3
	write(s2,&car,1); //Forward della risposta su s2 
	printf("%c",car);
	}
close(s3);
} else if (!strcmp(method,"CONNECT")){ //CASO IN CUI L'HOST VUOLE UNA CONNECT
hostname =&h[0].n[i];
for(;h[0].n[i]!=':';i++); h[0].n[i]=0; i++;
printf("hostname:%s\n",hostname);
port =&h[0].n[i];
for(;h[0].n[i]!=' ';i++); h[0].n[i]=0; i++;
printf("port:%s\n",port);
printf("REPORT:%s %s %s\n",method,hostname,port);
he = gethostbyname(hostname);
if (he == NULL) {
	printf("gethostbyname fallita\n");
	return 1;
	}
printf("Indirizzo server: %u.%u.%u.%u\n", (unsigned char)he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2], (unsigned char)he->h_addr[3]);
s3 = socket(AF_INET, SOCK_STREAM, 0);
if (s3 == -1) { perror("socket verso server fallita"); return 1;}
server.sin_family = AF_INET;
server.sin_port = htons((unsigned short)atoi(port));
server.sin_addr.s_addr=(*(unsigned int *)((*he).h_addr));
t = connect(s3,(struct sockaddr *)&server,sizeof(server));
if (t == -1) { perror("connect fallita"); return 1;}
sprintf(response,"HTTP/1.1 200 Established\r\n\r\n"); //Se la connect al server richiesto e' andata a buon fine invio all'host su s2 la risposta di connessione stabilita
write(s2,response,strlen(response));
if( pid = fork()){//Genitore
	while(read(s2,&car,1)){ //Leggo dal socket collegato all'host e scrivo sul socket del server per fargli la richiesta
		write(s3,&car,1);
		//printf("PAR:%x\n",car);
		}
	kill(pid,15);// 15 e' il codice per la sigterm --> kill -l
	}
else { //Figlio
	while(read(s3,&car,1)){ //Leggo dal socket collegato al server e scrivo sul socket dell'host per rispondergli
		write(s2,&car,1);
		//printf("CHD:%x\n",car);
		}
	exit(0);
	}
close(s3);
}
else{
sprintf(response,"HTTP/1.1 501 Not implemented\r\n\r\n");
write(s2,response,strlen(response));
}
close(s2);
exit(0);
}
}
close(s);
}