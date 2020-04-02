#include <stdio.h>
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

unsigned char iptarget[4] = {88,80,187,80};
struct sockaddr_ll sll;

struct eth_frame{ //1+1+2+1
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

struct icmp_packet{ //1+1+2+2+2
unsigned char type;
unsigned char code;
unsigned short checksum;
unsigned short id;
unsigned short seq;
unsigned char payload[1];
};
struct ip_datagram{ //1+1+2+2+2+1+1+2+4+4+1 =21bytes
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


int risolvi(unsigned char* target, unsigned char * mac_incognito) //Utilizza i pacchetti arp per assegnare i mac agli ip
{
unsigned char buffer[1500];
struct eth_frame * eth;
struct arp_packet * arp;
int i,n,s;
int lungh;
s=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //Accetta tutti i pacchetti eth
if(s==-1) { perror("socket fallita"); return 1;}

eth = (struct eth_frame*) buffer;
arp = (struct arp_packet *) eth->payload; //Formatto il payload in modo tale da poterci in

crea_arp(arp,1,target);
crea_eth(eth,broadcast,0x0806); // type =0x0806 ip
//stampa_buffer(buffer, 14+sizeof(struct arp_packet));

sll.sll_family=AF_PACKET;
sll.sll_ifindex=3; //interface number 3
lungh=sizeof(sll);
n = sendto(s, buffer, 14+sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, lungh); //14 e' la lunghezza dell'header, richiedo la risoluzione arp
if(n==-1) { perror("sendto fallita"); return 1;}

while( 1 ) { //Aspetto una risposta
n = recvfrom(s, buffer, 1500, 0, (struct sockaddr *) &sll, &lungh);
if(n==-1) { perror("recvfrom fallita"); return 1;}
if (eth->type ==htons(0x0806)) 
		if(arp->op ==htons(2))//Op =2 significa che mi sta arrivando una risposta , se fosse op=1 una richiesta
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


void crea_icmp_echoFrag(struct icmp_packet * icmp, unsigned short id, unsigned short seq, int nFrag ){
int i;
icmp->type=8; //8 e' la echo request
icmp->code=0; //con type=8 il code deve essere a 0
icmp->checksum=htons(0);
icmp-> id = htons(id);
icmp-> seq = htons(seq);
if(nFrag == 1){
	for(i=0;i<8;i++) //Primo pacchetto da 16bytes totali
		icmp-> payload[i]=i;
}
if(nFrag == 2){
	for(i=8;i<12;i++) //Secondo pacchetto restanti bytes
	icmp-> payload[i]=i;

}

icmp->checksum=htons(checksum((unsigned char *)icmp,28));
}

void crea_ip(struct ip_datagram *ip, int payloadsize,unsigned char  proto,unsigned char *ipdest, int frag)
{
ip-> ver_ihl = 0x45; //boh davedere
ip->tos=0;
ip->totlen=htons(payloadsize + 20);
ip->id=htons(0xABCD);
if(frag == 1){
ip->flag_offs=htons(0x124);
}
else
{
	ip->flag_offs=htons(0);
}

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
unsigned char buffer1[1500], buffer2[1500];
struct icmp_packet * icmp1;
struct ip_datagram * ip1;
struct eth_frame * eth1;
struct icmp_packet * icmp2;
struct ip_datagram * ip2;
struct eth_frame * eth2;
int lungh,n,n1;

t=risolvi(iptarget,mac);
if  (t==0){
	printf("Mac incognito:\n");
	stampa_buffer(mac,6);
	}
s=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
if (s==-1) { perror("Socket fallita"); return 1;}
eth1 = (struct eth_frame *) buffer1; //creo il pacchetto eth
ip1 = (struct ip_datagram *) eth1->payload;
icmp1 = (struct icmp_packet *) ip1->payload;

eth2 = (struct eth_frame *) buffer2;
ip2 = (struct ip_datagram *) eth2->payload;
icmp2 = (struct icmp_packet *) ip2->payload;


//Creo i due frammenti icmp e li inserisco in ip
crea_icmp_echoFrag(icmp1, 0x1234, 1, 1  );
crea_icmp_echoFrag(icmp2, 0x5678, 2, 2 );
crea_ip(ip1, 16,1,iptarget,1);
crea_ip(ip2, 12,1,iptarget,0);
crea_eth(eth1,mac,0x0800); 
crea_eth(eth2,mac,0x0800); 
printf("ICMP1/IP1/ETH1\n");
stampa_buffer(buffer1,14+20+8+100); // dimensioni dei vari buffer
printf("\nICMP2/IP2/ETH2\n");
stampa_buffer(buffer2,14+20+8+4); // dimensioni dei vari buffer
lungh = sizeof(struct sockaddr_ll);
bzero(&sll,lungh); //scrive tutti zero sulla stringa
printf("%d",sll.sll_ifindex);
sll.sll_ifindex=3;
n = sendto(s, buffer1, 14+20+8+20, 0, (struct sockaddr *) &sll, lungh);
if(n==-1) { perror("sendto fallita"); return 1;}
n1 = sendto(s, buffer2, 14+20+8+20, 0, (struct sockaddr *) &sll, lungh);
if(n1== -1){perror("sendto second buffer fallita"); return 1;}

}

