
#include <stdio.h>
#include <strings.h>

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
unsigned char dstip[4]={88 ,80 ,187 ,75 };
unsigned char miomac[6]={0xf2,0x3c,0x91,0xdb,0xc2,0x98};
unsigned char mioip[4]={88 ,80 ,187 ,84 };
unsigned char gateway[4]={88 ,80 ,187 ,1};
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
unsigned char payload[1000];
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
primo=1;
while(1){
	l=recvfrom(s,buffer,1600,0,(struct sockaddr *) &sll, &lunghezza); //setto l'indirizzo di ricezione
	gettimeofday(&tcptime,NULL);//time di inizio comunicazione
	if(eth->type==htons(0x0800)) //se IP
		if(ip->proto==6){ // se ICMP
			// if (tcp->d_port == htons(80) && primo){
		 //		other_port=htons(tcp->s_port);
	   	//		timezero=tcptime;
				// primo=0;
				// }	
				
			if((tcp->s_port == htons(80)) || (tcp->d_port == htons(80))){ //se la porta di source o di destination e' uguale a 80
					other_port = (tcp->s_port==htons(80))? htons(tcp->d_port):htons(tcp->s_port); //capisco se e' impostata 80 sulla porta di destinazione o si source, e imposta l'altra
					if(tcp->flags&0x02){ // controllo se il SYN e' impostato a 1 ==> 000010&000010 = 000010 (ONLY SYN ABILITATO)
	   					st[other_port].timezero=tcptime; //Imposto il timer del primo pacchetto a quella porta precisamente (other_port)
						if(tcp->d_port==htons(80)) st[other_port].forwzero=htonl(tcp->seq);
						if(tcp->s_port==htons(80)) st[other_port].backzero=htonl(tcp->seq);
					}
					seqzero = (tcp->d_port==htons(80))?st[other_port].forwzero:st[other_port].backzero;
					ackzero = (tcp->s_port==htons(80))?st[other_port].forwzero:st[other_port].backzero;
					delta_sec = tcptime.tv_sec - st[other_port].timezero.tv_sec;
					if (tcptime.tv_usec < st[other_port].timezero.tv_usec) {
						delta_sec --;
						delta_usec = 1000000+tcptime.tv_usec - st[other_port].timezero.tv_usec;
						}
					else 
						delta_usec = tcptime.tv_usec - st[other_port].timezero.tv_usec;
					printf("%.4d.%.6d %.5d->%.5d %.2x %.10u %.10u %.5u %4.2f\n",delta_sec, delta_usec, htons(tcp->s_port),htons(tcp->d_port),tcp->flags, htonl(tcp->seq)-seqzero, htonl(tcp->ack)-ackzero, htons(tcp->win),(htonl(tcp->ack)-ackzero)/(double)(delta_sec*1000000+delta_usec));
				}
		}
	}
}
