#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>
#include <unistd.h>


unsigned char miomac[6] = { 0xf2,0x3c,0x91,0xdb,0xc2,0x98 };
unsigned char broadcast[6] = { 0xff, 0xff, 0xff, 0xff,0xff,0xff};
unsigned char mioip[4] = {88,80,187,84};
unsigned char netmask[4] = { 255,255,255,0};
unsigned char gateway[4] = { 88,80,187,1};

unsigned char iptarget[4] = {146,241,215,214};



struct sockaddr_ll sll;

struct eth_frame{
unsigned char dst[6];
unsigned char src[6];
unsigned short int type;
unsigned char payload[1];
};

struct arp_packet{
unsigned short int htype;
unsigned short int ptype;
unsigned char hlen;
unsigned char plen;
unsigned short int op;
unsigned char hsrc[6];
unsigned char psrc[4];
unsigned char hdst[6];
unsigned char pdst[4];
};

struct icmp_packet{
unsigned char type;
unsigned char code;
unsigned short checksum;
unsigned short id;
unsigned short seq;
unsigned char payload[1];
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

void stampa_buffer( unsigned char* b, int quanti);

void crea_eth(struct eth_frame * e , unsigned char * dest, unsigned short type){
int i;
for(i=0;i<6;i++){
	e->dst[i]=dest[i];
	e->src[i]=miomac[i];
	}
e->type=htons(type);
}

void crea_arp( struct arp_packet * a, unsigned short op,  unsigned char * ptarget) {  
int i;
a->htype=htons(1);
a->ptype=htons(0x0800);
a->hlen=6;
a->plen=4;
a->op=htons(op);
for(i=0;i<6;i++){
	a->hsrc[i]=miomac[i];
	a->hdst[i]=0;
}
for(i=0;i<4;i++){
	a->psrc[i]=mioip[i];
	a->pdst[i]=ptarget[i];
	}
}


int risolvi(unsigned char* target, unsigned char * mac_incognito)
{
unsigned char buffer[1500];
struct eth_frame * eth;
struct arp_packet * arp;
int i,n,s;
int lungh;
s=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
if(s==-1) { perror("socket fallita"); return 1;}

eth = (struct eth_frame*) buffer;
arp = (struct arp_packet *) eth->payload;

crea_arp(arp,1,target);
crea_eth(eth,broadcast,0x0806);
//stampa_buffer(buffer, 14+sizeof(struct arp_packet));

sll.sll_family=AF_PACKET;
sll.sll_ifindex=3;
lungh=sizeof(sll);
n = sendto(s, buffer, 14+sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, lungh);
if(n==-1) { perror("sendto fallita"); return 1;}

while( 1 ) {
n = recvfrom(s, buffer, 1500, 0, (struct sockaddr *) &sll, &lungh);
if(n==-1) { perror("recvfrom fallita"); return 1;}
if (eth->type ==htons(0x0806)) 
		if(arp->op ==htons(2))
			if(!memcmp(arp->psrc,target,4)){
				for(i=0;i<6;i++)
					mac_incognito[i]=arp->hsrc[i];
		break;
	}
}
close(s);
return 0;
}


void stampa_buffer( unsigned char* b, int quanti){
int i;
for(i=0;i<quanti;i++){
	printf("%.3d(%.2x) ",b[i],b[i]);
	if((i%4)==3)
		printf("\n");
	}
}

unsigned short checksum(unsigned char * b, int n)
{
int i;
unsigned short prev, tot=0, * p = (unsigned short *) b;
for(i=0;i<n/2;i++){
	prev = tot;
	tot += htons(p[i]);
	if (tot < prev) tot++;
	}
	return (0xFFFF-tot);
}


void crea_icmp_echo(struct icmp_packet * icmp, unsigned short id, unsigned short seq, int codeIcmp, int typeIcmp ){
int i;
icmp->type=typeIcmp;
icmp->code=codeIcmp;
icmp->checksum=htons(0);
icmp-> id = htons(id);
icmp-> seq = htons(seq);
for(i=0;i<20;i++)
	icmp-> payload[i]=i;
icmp->checksum=htons(checksum((unsigned char *)icmp,28));
}

void crea_ip(struct ip_datagram *ip, int payloadsize,unsigned char  proto,unsigned char *ipdest)
{
ip-> ver_ihl = 0x45;
ip->tos=0;
ip->totlen=htons(payloadsize + 20);
ip->id=htons(0xABCD);
ip->flag_offs=htons(0);
ip->ttl=128;
ip->proto=proto;
ip->checksum=htons(0);
ip->saddr = *(unsigned int*) mioip ;
ip->daddr = *(unsigned int*) ipdest;
ip->checksum=htons(checksum((unsigned char*)ip,20));
};

int main(){
int t,s;
unsigned char mac[6];
unsigned char buffer[1500], bufferR[1500];
struct icmp_packet * icmp;
struct ip_datagram * ip, *ipR;
struct eth_frame * eth, *ethR;
struct tcp_segment * tcp;
int lungh,n;

if((*(unsigned int*)mioip & *(unsigned int*)netmask) == (*(unsigned int*)iptarget & *(unsigned int*)netmask)){
	t=risolvi(iptarget,mac);
}
else
	t=risolvi(gateway,mac);
		 
if  (t==0){
	printf("Mac incognito:\n");
	stampa_buffer(mac,6);
	}
s=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
if (s==-1) { perror("Socket fallita"); return 1;}
//-------------- Pacchetto di risposta
eth = (struct eth_frame *) buffer;
ip = (struct ip_datagram *) eth->payload;
icmp = (struct icmp_packet *) ip->payload;
//-------------- Pacchetto di buffer per richiesta
ethR = (struct eth_frame *) bufferR;
ipR = (struct ip_datagram *) ethR->payload;
tcp = (struct tcp_segment *) ipR -> payload;

//--------------- Creo pacchetto di buffer per accogliere la richiesta

printf("\nIn attesa di contatto...\n");
while( 1 ) {
n = recvfrom(s, bufferR, 1500, 0, (struct sockaddr *) &sll, &lungh);
if(n==-1) { perror("recvfrom fallita"); return 1;}
//Controllo se il pacchetto è quello corretto
if (ethR->type ==htons(0x0800))
	if(ipR->proto == 6) 
		/*if(tcp->d_port >= htons(20000) && tcp->d_port <= htons(30000))*/if(tcp->d_port == htons(21000)){
			stampa_buffer(bufferR, 14+20+8+20);
			mac[6] = ethR->src;
			//iptarget[4] = (htons(ipR->saddr));
			printf("\nIndirizzo MAC e IP\n");
			stampa_buffer(mac,6);
			stampa_buffer(iptarget,4);
			crea_icmp_echo(icmp, 0x1234, 1 ,3 ,3); //type=3 code=3 messaggio echo port unrechable
			crea_ip(ip, 28,1,iptarget);
			crea_eth(eth,mac,0x0800);
			printf("\nMessaggio ICMP\n");
			stampa_buffer(buffer, 14+20+8+20);
			lungh = sizeof(struct sockaddr_ll);
			bzero(&sll,lungh);
			sll.sll_ifindex=3;
			n = sendto(s, buffer, 14+20+8+20, 0, (struct sockaddr *) &sll, lungh);
			if(n==-1) { perror("sendto fallita"); return 1;}
			break;
		}
}
}

