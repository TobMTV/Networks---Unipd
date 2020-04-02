/* Tob ricorda:
char 1 byte
short 2 byte
int 4 byte 
*/
#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>


unsigned char miomac[6] = { 0xf2,0x3c,0x91,0xdb,0xc2,0x98 };
unsigned char broadcast[6] = { 0xff, 0xff, 0xff, 0xff,0xff,0xff};
unsigned char mioip[4] = {88,80,187,84};

unsigned char target[4] = {88,80,187,80};
struct sockaddr_ll sll;
/*    struct sockaddr_ll {
               unsigned short sll_family;   /* Always AF_PACKET 
               unsigned short sll_protocol; /* Physical-layer protocol 
               int            sll_ifindex;  /* Interface number 
               unsigned short sll_hatype;   /* ARP hardware type 
               unsigned char  sll_pkttype;  /* Packet type 
               unsigned char  sll_halen;    /* Length of address 
               unsigned char  sll_addr[8];  /* Physical-layer address 
           };
*/
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

unsigned char buffer[1500];

void stampa_buffer( unsigned char* b, int quanti);

void crea_eth(struct eth_frame * e , unsigned char * dest, unsigned short type){
int i;
for(i=0;i<6;i++){ //Lo inserisco byte per byte
	e->dst[i]=dest[i];
	e->src[i]=miomac[i];
	}
e->type=htons(type);
}

void crea_arp( struct arp_packet * a, unsigned short op,  unsigned char * ptarget) {  
int i;
a->htype=htons(1); // Ovviamente questi li passo con htons perche' altrimenti la rete non capisce se li trova con l' endianess sbagliato
a->ptype=htons(0x0800);
a->hlen=6; //No htons perche' si tratta di un char a 1 byte
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


int main()
{
struct eth_frame * eth;
struct arp_packet * arp;
int i,n,s;
int lungh;
unsigned char mac_incognito[6];
s=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //ETH_P_ALL tutti i pacchetti eth vengono passati attraverso questo socket
if(s==-1) { perror("socket fallita"); return 1;}

eth = (struct eth_frame*) buffer;
arp = (struct arp_packet *) eth->payload; //Formatto il payload di eth per accogliere dati arp, dunque eth tiene in pancia arp 

crea_arp(arp,1,target);
crea_eth(eth,broadcast,0x0806);
stampa_buffer(buffer, 14+sizeof(struct arp_packet));//+14 perche' si tratta dell'header dell'eth 

sll.sll_family=AF_PACKET;
sll.sll_ifindex=3; //Interface number 3
lungh=sizeof(sll);
n = sendto(s, buffer, 14+sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, lungh); //+14 perche' si tratta dell'header dell'eth 
																						
while( 1 ) {
n = recvfrom(s, buffer, 1500, 0, (struct sockaddr *) &sll, &lungh);
if(n==-1) { perror("recvfrom fallita"); return 1;}
if (eth->type ==htons(0x0806)) //Se si tratta di un ipv4
		if(arp->op ==htons(2)) //Op =2 significa che mi sta arrivando una risposta , se fosse op=1 una richiesta
			if(!memcmp(arp->psrc,target,4)){ //  se psrc contenuto nell'arp packet e' quello target e deve per forza avere lunghezza 4
				for(i=0;i<6;i++)
					mac_incognito[i]=arp->hsrc[i]; //Mi piglio il mac incognito portato dal pacchetto arp
				printf("Mac incognito:\n");
				stampa_buffer(mac_incognito,6);
		break;
	}
}
}


void stampa_buffer( unsigned char* b, int quanti){
int i;
for(i=0;i<quanti;i++){
	printf("%.3d(%.2x) ",b[i],b[i]);
	if((i%4)==3)
		printf("\n");
	}
}
