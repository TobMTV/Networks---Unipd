//Attenzione al parametro len minimo per tcp 
unsigned short checksum( unsigned char * buffer, int len){
   int i;
   unsigned short *p;
   unsigned int tot=0;
   p = (unsigned short *) buffer;
   for(i=0;i<len/2;i++){
      tot = tot + htons(p[i]);
      if (tot&0x10000) tot = (tot&0xFFFF)+1;
   }
   return (unsigned short)0xFFFF-tot;
}
//ip->checksum=htons(checksum((unsigned char*) ip, 20)); per calcolare checksum ip


//Per checksum TCP

struct tcp_pseudo{
   unsigned int ip_src, ip_dst;
   unsigned char zeroes;
   unsigned char proto;        // ip datagram protocol field (tcp = 6, ip = 1)
   unsigned short entire_len;  // tcp length (header + data)
   unsigned char tcp_segment[20/*to set appropriatly */];  // entire tcp packet pointer
};

unsigned short ip_total_len = ntohs(ip->totlen);
unsigned short ip_header_dim = (ip->ver_ihl & 0x0F) * 4;
int ip_payload_len = ip_total_len-ip_header_dim;

int TCP_TOTAL_LEN = 20;
struct tcp_pseudo pseudo; // size of this: 12
memcpy(pseudo.tcp_segment,tcp,TCP_TOTAL_LEN); 
pseudo.zeroes = 0;
pseudo.ip_src = ip->src;
pseudo.ip_dst = ip->dst;
pseudo.proto = 6;
pseudo.entire_len = htons(TCP_TOTAL_LEN); // may vary
tcp->checksum = htons(checksum((unsigned char*)&pseudo,TCP_TOTAL_LEN+12));
