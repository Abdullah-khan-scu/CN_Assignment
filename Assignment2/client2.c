#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define PORT 30000

//Primitives
#define PACKETID 0XFFFF
#define CLIENTID 0XFF
#define ENDPACKETID 0XFFFF

//server responses
#define PAID 0XFFFB
#define NOTPAID 0XFFF9
#define NOTEXIST 0XFFFA

//Access permission request packet format
struct requestPacket {
	
	uint16_t packet_ID;
	uint8_t client_ID;
	uint16_t Acc_Per;
	uint8_t segNum;
	uint8_t length;
	uint8_t technology;
	unsigned int SourceSubscriberNo;
	uint16_t endpacket_ID;
};

//response packet
struct responsePacket {
	
	uint16_t packet_ID;
	uint8_t client_ID;
	uint16_t type;
	uint8_t segNum;
	uint8_t length;
	uint8_t technology;
	unsigned int SourceSubscriberNo;
	uint16_t endpacket_ID;
};

//print all the packet details
void printPacketDetails(struct requestPacket request) {
	
	printf(" packet_ID: %x\n",request.packet_ID);
	printf("Client id : %hhx\n",request.client_ID);
	printf("Access permission: %x\n",request.Acc_Per);
	printf("Segment no : %d \n",request.segNum);
	printf("length %d\n",request.length);
	printf("Technology %d \n", request.technology);
	printf("Subscriber no: %u \n",request.SourceSubscriberNo);
	printf("end of datapacket id : %x \n",request.endpacket_ID);
}

//create requestPacket with data
struct requestPacket createRequestPacket() {
	
	struct requestPacket request;
	request.packet_ID = PACKETID;
	request.client_ID = CLIENTID;
	request.Acc_Per = 0XFFF8;
	request.endpacket_ID = ENDPACKETID;
	
	return request;
}


int main(int argc, char**argv){
	
	struct requestPacket request;
	struct responsePacket response;
	char line[30];
	FILE *fp;
	int sck,n = 0;
	struct sockaddr_in cliaddr;
	socklen_t addr_size;
	sck = socket(AF_INET,SOCK_DGRAM,0);
	int i = 1;
	
	//socket timeout
	struct timeval timer;
	timer.tv_sec = 3;
	timer.tv_usec = 0;	
	
	setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timer,sizeof(struct timeval));
	int retryCounter = 0;
	if(sck < 0) {
		printf("\n ERROR: Socket Failure \n");
	}
	
	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliaddr.sin_port=htons(PORT);
	addr_size = sizeof cliaddr;
	request = createRequestPacket();

	//get input data from txt file
	fp = fopen("sample_input_pa2.txt", "rt");
	if(fp == NULL)
	{
		printf("\n ERROR: File not found \n");
	}
	
	while(fgets(line, sizeof(line), fp) != NULL) {
		
		n = 0;
		retryCounter = 0;
		printf("\n");
		
		char * words;
		words = strtok(line," ");
		request.length = strlen(words);
		request.SourceSubscriberNo = (unsigned) atoi(words);
        
		words = strtok(NULL," ");
		request.length += strlen(words);
		request.technology = atoi(words);
		words = strtok(NULL," ");
		request.segNum = i;
		
        printf("Subscriber no: %u \n",request.SourceSubscriberNo);
		printPacketDetails(request);
		while(n <= 0 && retryCounter < 3) {
			
			//send and recieve packets
			sendto(sck,&request,sizeof(struct requestPacket),0,(struct sockaddr *)&cliaddr,addr_size);
			n = recvfrom(sck,&response,sizeof(struct responsePacket),0,NULL,NULL);
			
			if(n <= 0 ) {
				printf("\n ERROR: No response from server\n");
				printf("Sending packet again \n");
				retryCounter++;
			}
			
			else if(n > 0) {
				printf("]n INFO: \n");
				if(response.type == NOTPAID) {
					printf("Subscriber has not paid \n");
				}
				else if(response.type == NOTEXIST ) {
					printf("Subscriber not found  \n");
				}
				else if(response.type == PAID) {
					printf("Paid Subscriber \n");

				}
			}
		}
		
		//no ACK recieved after sending packet 3 times
		if(retryCounter>= 3 ) {
			printf("\n ERROR: Server does not respond \n");
			exit(0);
		}
		printf("\n ---------------------------------------------------------------------- \n");
		i++;
	}
	fclose(fp);
}

