
#include <sys/socket.h>
#include <netinet/in.h>  // sockaddr_in is a structure containing an internet address. This structure is defined in netinet/in.h.
#include <stdio.h>
#include <strings.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//Primitives
#define PORT 7070
#define STARTPACKETID 0XFFFF
#define ENDPACKETID 0XFFFF
#define CLIENTID 0XFF
#define TIMEOUT 4

//Packet Types
#define DATATYPE 0XFFF1
#define ACKPACKETTYPE 0XFFF2
#define REJECTCODETYPE 0XFFF3

//Reject Sub codes
#define OUTOFSEQUENCE 0XFFF4
#define LENGTHMISMATCH 0XFFF5
#define ENDOFPACKETMISSING 0XFFF6
#define DUPLICATE 0XFFF7

//Data Packet Definition
struct DataPacket{
	uint16_t startofPacketID;
	uint8_t clientID;
	uint16_t data;
	uint8_t segmentNo;
	uint8_t length;
	char payload[255];
	uint16_t endofPacketID;
};

//Acknowledgement Packet Definition
struct ACKPacket {
	uint16_t startofPacketID;
	uint8_t clientID;
	uint16_t ACK;
	uint8_t segmentNo;
	uint16_t endofPacketID;
};

//Structure For Reject Definition
struct RejectPacket {
	uint16_t startofPacketID;
	uint8_t clientID;
	uint16_t reject;
	uint16_t subCode;
	uint8_t segmentNo;
	uint16_t endofPacketID;
};

// Function to print the data packet
/*void view (struct DataPacket data)
{
	printf(" \n *** SERVER RECIEVING *** \n");
	printf("PACKET ID : %hx\n", data.startofPacketID);
	printf("CLIENT ID : %hhx\n", data.clientID);
	printf("DATA : %x\n", data.data);
	printf("SEGMENT NUMBER -> %d\n", data.segmentNo);
	printf("LENGTH : %d\n", data.length);
	printf("PAYLOAD : %s\n", data.payload);
	printf("END PACKET ID : %x\n", data.endofPacketID);
}
*/
// Create Acknowledgement Packet
struct ACKPacket createACKPacket(struct DataPacket data) {
	struct ACKPacket ack;
	ack.startofPacketID = data.startofPacketID;
	ack.clientID = data.clientID;
	ack.segmentNo = data.segmentNo;
	ack.ACK = ACKPACKETTYPE ;
	ack.endofPacketID = data.endofPacketID;
	return ack;
}

// Creating Reject Packet
struct RejectPacket createReject(struct DataPacket data) {
	struct RejectPacket rp;
	rp.startofPacketID = data.startofPacketID;
	rp.clientID = data.clientID;
	rp.segmentNo = data.segmentNo;
	rp.reject = REJECTCODETYPE;
	rp.endofPacketID = data.endofPacketID;
	return rp;
}

int main(int argc, char**argv)
{
	int sck,n; //Socket creation variables
	struct sockaddr_in serverAddr; //is a structure containing an internet address
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	struct DataPacket dataPacket;
	struct ACKPacket  ackPacket;
	struct RejectPacket rejectPacket;
	int count=1;

	// Storing packet contents
	int visited[20];
	for (int j = 0; j < 20; j++) {
		visited[j]=0;
	}
	//Create and Verify Socket
	sck = socket(AF_INET, SOCK_DGRAM, 0); //Internet domain for any two hosts on the Internet
	//int packetexpected = 1;
	bzero(&serverAddr, sizeof(serverAddr));
	//Assign IP, PORT
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr=htonl(INADDR_ANY); //This field contains the IP address of the host
	serverAddr.sin_port=htons(PORT); //to convert this to network byte order
	//Binding created socket to IP
	bind(sck,(struct sockaddr *)&serverAddr,sizeof(serverAddr));
	addr_size = sizeof serverAddr;
	printf("**THE SERVER IS INITIALIZED**\n");

	for(int packetexpected=1;packetexpected<=count;packetexpected++)
	{
		//Receiving the Packet from client
		n = recvfrom(sck,&dataPacket,sizeof(struct DataPacket),0,(struct sockaddr *)&serverStorage, &addr_size);
		printf("******\n");
		//view(dataPacket);
		visited[dataPacket.segmentNo]++;

		if (dataPacket.segmentNo == 11 || dataPacket.segmentNo == 12) {
			visited[dataPacket.segmentNo]=1;
		}
		int length = strlen(dataPacket.payload);

		//Same segment number packeT recived
		if (visited[dataPacket.segmentNo]!=1) {
			rejectPacket = createReject(dataPacket);
			rejectPacket.subCode = DUPLICATE;
			sendto(sck,&rejectPacket,sizeof(struct RejectPacket), 0, (struct sockaddr *)&serverStorage,addr_size);
			printf("**ERROR: A DUPLICATE PACKET WAS RECEIVED**\n\n");
		}
		//packet does not have a end of the packet IDENTIFIER
		else if(dataPacket.endofPacketID != ENDPACKETID ) {
			rejectPacket = createReject(dataPacket);
			rejectPacket.subCode = ENDOFPACKETMISSING ;
			sendto(sck,&rejectPacket,sizeof(struct RejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			printf("**END OF PACKET IDENTIFIER IS MISSING**\n\n");
		}

		//length different then the payload field
		else if(length != dataPacket.length) {
			rejectPacket = createReject(dataPacket);
			rejectPacket.subCode = LENGTHMISMATCH ;
			sendto(sck,&rejectPacket,sizeof(struct RejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			printf("**ERROR : LENGTH MISMATCH**\n\n");
		}

		//if packet incoming is out of sequence
		else if(dataPacket.segmentNo != packetexpected && dataPacket.segmentNo != 11 && dataPacket.segmentNo != 12) {
			rejectPacket = createReject(dataPacket);
			rejectPacket.subCode = OUTOFSEQUENCE;
			sendto(sck,&rejectPacket,sizeof(struct RejectPacket),0,(struct sockaddr *)&serverStorage,addr_size);
			printf("**ERROR : PACKET IS OUT OF SEQUENCE**\n\n");
		}
		else {
			//if 11th then sleep no ack
			if(dataPacket.segmentNo == 11) {
				sleep(7);
			}
			//send ack
			ackPacket = createACKPacket(dataPacket);
			sendto(sck,&ackPacket,sizeof(struct ACKPacket),0,(struct sockaddr *)&serverStorage,addr_size);
		}
		count++; //next packet count
		printf("\n ---------------------------  NEXT PACKET STARTS HERE ---------------------------  \n");
	}
}


