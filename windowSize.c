#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/time.h> 
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */

#include <arpa/inet.h>
struct timeval tcptime, timezero;
struct sockaddr_ll sll;
unsigned int delta_sec, delta_usec;
int primo;
unsigned char dstip[4]={192,168,1,101 };
unsigned char miomac[6]={0xf2,0x3c,0x91,0xdb,0xc2,0x98};
unsigned char mioip[4]={192,168,1,104 };
unsigned char gateway[4]={192,168,1,1};
unsigned char netmask[4]={255 ,255 ,255 ,0 };
unsigned char broadcastmac[6]={0xff,0xff,0xff,0xff,0xff,0xff};

int progr=0;

struct tcp_segment{
unsigned short s_port;
unsigned short d_port;
unsigned int seq; //seq e ack sono utilizzati per il 3way handshake e per verificare se la trasmissione dell'informazione e' stata completata con successo
unsigned int ack; // 3way handshake ==> (Client)SYN + seq=x --> (Server)SYN-ACK ack=x+1 seq=y -->(Client)ACK ack=y+1 seq=x+1 --> DATA 
unsigned char d_offs_res;// indica dove i dati iniziano
unsigned char flags;
unsigned short win; // max windows size for the sender/receiver (?)
unsigned short checksum;
unsigned short urgp; // questo puntatore se settato punta ai dati urgenti
unsigned char kind;
unsigned char length;
unsigned char shiftCnt;
unsigned char payload[10];
};

struct eth_frame {
unsigned char dst[6];
unsigned char src[6];
unsigned short type;
char payload[1500];
};


struct ip_datagram{
unsigned char ver_ihl;
unsigned char tos;
unsigned short totlen;
unsigned short id;
unsigned short flag_offs;
unsigned char ttl;
unsigned char proto;
unsigned short checksum;
unsigned int saddr;
unsigned int daddr;
unsigned char payload[1];
};
unsigned short int checksum(char *b, int l){
unsigned short *p ;

int i;
unsigned short int tot=0;
unsigned short int prec;
/*Assunzione: l pari */
p=(unsigned short *)b;

	for(i=0; i < l/2 ;i++){
		prec = tot;
		tot += htons(p[i]);
		if (tot<prec) tot=tot+1;	
	}
	return (0xFFFF-tot);
}

struct tcp_status{
unsigned int forwzero, backzero;
struct timeval timezero;
}st[65536]; //[NUMERO MAX DI PORTE]

/*void decToBinary(int numb,int* bin[]){
	int i;
	for(i=0;numb>0;i++){
	bin[i]=numb%2;
	numb=numb/2;
	}
	for(i=0;i<7;i++){
	printf("%d",bin[i]);
	}
    }
*/
void stampa_buffer( unsigned char* b, int quanti){
int i;
for(i=0;i<quanti;i++){
	printf("%.3d(%.2x) ",b[i],b[i]);
	if((i%4)==3)
		printf("\n");
	}
}

void creaip(struct ip_datagram *ip, unsigned int dstip,int payload_len, unsigned char proto){
ip->ver_ihl= 0x45 ; //primi 4 bit: versione=4, ultimi 4 bit:IHL=5word(32bit*5=20byte)
ip->tos = 0x0;
ip->totlen = htons(payload_len+20);
ip->id=progr++;
ip->flag_offs=htons(0);
ip->ttl=64;
ip->proto=proto;
ip->checksum=0;
ip->saddr=*(unsigned int *)mioip;
ip->daddr=dstip;
ip->checksum = htons(checksum((unsigned char*)ip,20));
};

int main(int argc, char ** argv)
{
unsigned int ackzero, seqzero, forwzero,backzero;
unsigned char buffer[1600];
unsigned short other_port;
int i,l,s,lunghezza; 
struct eth_frame * eth;
struct ip_datagram *ip;
struct tcp_segment *tcp;
eth = (struct eth_frame *) buffer;
ip = (struct ip_datagram*) eth->payload;
tcp = (struct tcp_segment*) ip->payload;
s = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
lunghezza = sizeof(struct sockaddr_ll);
bzero(&sll,sizeof(struct sockaddr_ll)); //resetto la struct impostando tutti zero
sll.sll_ifindex=3; //imposto l'interfaccia da cui ricevero'
int  bin[8],g;
primo=1;
while(1){
	l=recvfrom(s,buffer,1600,0,(struct sockaddr *) &sll, &lunghezza); //setto l'indirizzo di ricezione


	gettimeofday(&tcptime,NULL);//time di inizio comunicazione
	if(eth->type==htons(0x0800)) //se IP
		if(ip->proto==6){ // se TCP
       			int offset = tcp ->d_offs_res; //01010000
        		offset = offset>>4;
			int scale=0, window;
			if(tcp->flags&0x2 && offset>=5){ // se il flag syn attivo (&000010) e ci sono delle opzioni attivate, imposto offset strettamente > 5 perche' 5 e' il valore minimo per l'header (32bit)
				printf("\nPacchetto con SYN attivo con opzione :%d\n", offset);
				window = tcp->win;
				//Posizione del fattore di scaling
	                        if(tcp->kind == 3 && tcp->length == 3){
                                	scale= tcp->shiftCnt;
					window = tcp->win + 2^scale;
				}
        	                printf("\nScalamento %d:",scale);
				printf("\nWindow size:%d",window);
				printf("\nInizio buffer\n");
        			stampa_buffer(buffer,14+20+20+10);
			}
	//stampa_buffer(buffer,14+20+20+10);
	//printf("\nIdentificativo pacchetto : %d",ip->id);		
        //printf("\nWindow size :%d\n",tcp->win);
		}
	}
}
