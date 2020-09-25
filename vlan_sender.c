#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include "ethernetheader.h" //for vlan
#include <inttypes.h> // to print uint16 values
#include <math.h>

#define MY_DEST_MAC0	0xb8
#define MY_DEST_MAC1	0x27
#define MY_DEST_MAC2	0xeb
#define MY_DEST_MAC3	0x6e
#define MY_DEST_MAC4	0xcd
#define MY_DEST_MAC5	0xe8

#define WDEST_MAC0	0x08
#define WDEST_MAC1	0x00
#define WDEST_MAC2	0x27
#define WDEST_MAC3	0x73
#define WDEST_MAC4	0x99
#define WDEST_MAC5	0x7c

#define DEFAULT_IF	"enp0s31f6"
#define BUF_SIZ		2024
#define DEFAULT_PCP     4
#define DEFAULT_ID      102
#define DEFAULT_IDENTIFICATION 10 //mbps 
#define DEFAULT_PAYLOAD_LEN  4 //maximum 1440 byte

int main(int argc, char *argv[])
{
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	struct ifreq if_ip;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct vlan_ethhdr *vh=(struct vlan_ethhdr *) sendbuf;
	//struct ether_header *vh = (struct ether_header *) sendbuf;

	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
	int identification;
	uint16_t PCP;
	uint16_t CFI=0;
	uint16_t ID;
	int payload_limit_for_cycle;	
	int payload_len;
	int tx_len_bit; //size of ethernet frame in  bit
	double tx_len_Mb; // size of ethernet frame in Mbit
	double sending_period; //trasmission period of the packet
	/* Get interface name */
	if (argc > 3){

		printf("ooooo");
		strcpy(ifName, argv[1]);
		identification=atoi(argv[2]);//bit ps
                PCP=atoi(argv[3]);// priorirty
                ID=atoi(argv[4]);//vlan id
		payload_len=atoi(argv[5]);//payload len byte
		}
	else{
		strcpy(ifName, DEFAULT_IF);
		PCP=(int)DEFAULT_PCP;
                ID=(int)DEFAULT_ID;
                identification=(int)DEFAULT_IDENTIFICATION;
		payload_len=(int)DEFAULT_PAYLOAD_LEN;
		
	   }
	printf("identification,PCP,ID,payload len: %d %d %d %d",identification,PCP,ID,payload_len);
	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");
	/* Get the IP address of the interface to send on */
	memset(&if_ip, 0, sizeof(struct ifreq));
	strncpy(if_ip.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFADDR, &if_idx) < 0)
	    perror("SIOCGIFADDR");

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	vh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	vh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	vh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	vh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	vh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	vh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	vh->ether_dhost[0] = MY_DEST_MAC0;
	vh->ether_dhost[1] = MY_DEST_MAC1;
	vh->ether_dhost[2] = MY_DEST_MAC2;
	vh->ether_dhost[3] = MY_DEST_MAC3;
	vh->ether_dhost[4] = MY_DEST_MAC4;
	vh->ether_dhost[5] = MY_DEST_MAC5;
	/* Ethertype field */
	vh->ether_type = htons(0x0800);
      vh->h_vlan_proto = htons(0x8100);
	PCP=PCP<<13;
	CFI=CFI<<12;
         vh->h_vlan_TCI = htons(PCP|CFI|ID);
	
	tx_len += sizeof(struct vlan_ethhdr);

        /*Costruction of IP header*/
      struct iphdr *iph = (struct iphdr*)(sendbuf + sizeof(struct vlan_ethhdr));

        iph->ihl=5;
         iph->tos =htons( 128);
	// iph->tot_len=5;
	iph->version=4;
        iph->id = htons(identification);
        iph->frag_off=0;
        iph->ttl = htons(255);
	
      

        iph->protocol =17;//htons(IPPROTO_UDP);
        iph->check=0;
//
 //	iph->tot_len=htons(50);
	
        iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(if_idx.ifr_addr))->sin_addr)));
      //iph->saddr = inet_addr("10.0.0.1");
        iph->daddr = inet_addr("10.0.0.10"); // put destination IP address
	//	iph->ihl = sizeof(struct iphdr);
//        printf("Dest IP address: %" PRIu16 "\n",iph->daddr); 
//        printf("Src IP address: %" PRIu16 "\n",iph->saddr);

 
        tx_len += sizeof(struct iphdr);
        struct udphdr * udph= (struct udphdr *) (sendbuf +sizeof(struct iphdr)+sizeof(struct vlan_ethhdr));
//	udph->source=htons(6666);
//	udph->dest=htons(8622);
	udph->check=0;
	tx_len +=sizeof(struct udphdr);
//
	int z=0;

	payload_limit_for_cycle=payload_len/4;//the cycle is done with a for cycle with 4 istructions so
	printf("How many times cycle :%d",payload_limit_for_cycle);

	if(payload_limit_for_cycle==1){
		/*Packet data*/	
		sendbuf[tx_len++] = 0xab;
       	        sendbuf[tx_len++] = 0xbc;
        	sendbuf[tx_len++] = 0xcd;
        	sendbuf[tx_len++] = 0xef;
		}
	else{
		/* Packet data */
		for(z=0;z<payload_limit_for_cycle;z++){
			sendbuf[tx_len++] = 0xab;
			sendbuf[tx_len++] = 0xbc;
			sendbuf[tx_len++] = 0xcd;
			sendbuf[tx_len++] = 0xef;
		}
	   }
	/*Calculation for sleep period in order to send packet to the given IDENTIFICATION(Bit rate)*/
	tx_len_bit=tx_len*8;
	printf("lenght of packet in bit %d",tx_len_bit);
//	tx_len_Mb=(double)tx_len_bit/(double)(pow(2,20));
//	printf("tx_len Mb %f",tx_len_Mb);
	sending_period=(double)tx_len_bit/(double)identification;
	printf("seding period %f",sending_period);
	udph->len=htons(tx_len - sizeof(struct vlan_ethhdr) - sizeof(struct iphdr));
	iph->tot_len=htons(tx_len - sizeof(struct vlan_ethhdr));

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;
	int k;
	/* Send packet */
//	int var=20;
//	int var2=3;
//	double f=(double)var/(double)var2;
//	printf("time %f",f);
	for(k=0;;k++){
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0){
	    printf("Send failed\n");}
	printf("\n Sent");	
	sleep(sending_period);
	}
//	return 0;
}
