#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define WINDOWSIZE 10
#define MAX_WAIT_TIME 1

int main(int argc, char *argv[]){
	int sd;
	struct sockaddr_in server_address;
	struct sockaddr_in from_address;
	socklen_t fromLength = sizeof(struct sockaddr_in);
	int portNumber;
	char serverIP[29];
	int rc = 0;
	char string[100];
	char bufferRead[100];
	time_t timeSent[100];
	time_t currentTime;
	char packet[17];
	int seq_num = 0;
	int ackNumber = -1;
	int packetSize;

	if(argc < 3){
		printf("usage is client <ipaddr> <port>\n");
		exit(1);
	}

	sd = socket(AF_INET, SOCK_DGRAM, 0);

	portNumber = strtol(argv[2], NULL, 10);
	strcpy(serverIP, argv[1]);

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portNumber);
	server_address.sin_addr.s_addr = inet_addr(serverIP);

	/* ask the user for a string */
	printf("Please enter a string that you want to transfer: ");
	memset(string, '\0', 100);
	fgets(string, sizeof(string), stdin);

	/* determine length of the string */
	int stringSize = strlen(string);
	if(stringSize > 0 && string[stringSize-1] == '\n'){
		string[stringSize-1] = '\0';
		stringSize = strlen(string);
	}
	printf("The string size is %d\n", stringSize);


	/* convert to network order */
	int convertedStringSize = htonl(stringSize);
	printf("The converted string size is %d\n", convertedStringSize);

	/* send the length to the server */
	rc = sendto(sd, &convertedStringSize, sizeof(int), 0, (struct sockaddr *)&server_address, sizeof(server_address));
	
	/* setup top and bottom window */
	int bottomWindow = 0;
	int topWindow = 0;
	int i = 0;
	while(topWindow < stringSize && i < WINDOWSIZE){
		topWindow++;
		i++;
	}
	
	/* loop until all data is sent */ 
	while(bottomWindow < stringSize)
	{	
		/* send data in the window */
		while(seq_num < topWindow){
			memset(packet, '\0', 17);
			timeSent[seq_num] = time(NULL);
			if(seq_num + 1 < stringSize){
				sprintf(packet, "%11d%4d", seq_num, 2);
				packet[15] = string[seq_num];
				packet[16] = string[seq_num + 1];
				seq_num += 2;
				packetSize = 17;
			}
			else{
				sprintf(packet, "%11d%4d", seq_num, 1);
				packet[15] = string[seq_num];
				seq_num += 1;
				packetSize = 16;
			}
		
			rc = sendto(sd, packet, packetSize, 0, (struct sockaddr *)&server_address, sizeof(server_address));
			if(rc < 0){
				perror("send");
				exit(1);
			}
		}
		
		/* check for ACK or time out for first packet in window */
		while(1){
			/* resend everything in window if time out */
			currentTime = time(NULL);
			if(currentTime - timeSent[bottomWindow] > MAX_WAIT_TIME) {
				printf("***Timeout occurred***\n");
				printf("Resending everything in the window of: %d\n", bottomWindow); 
				seq_num = bottomWindow;
				while(seq_num < topWindow){	
					timeSent[seq_num] = time(NULL);
					if(seq_num + 1 < stringSize){
						sprintf(packet, "%11d%4d", seq_num, 2);
						packet[15] = string[seq_num];
						packet[16] = string[seq_num + 1];
						seq_num += 2;
						packetSize = 17;
					}
					else{
						sprintf(packet, "%11d%4d", seq_num, 1);
						packet[15] = string[seq_num];
						seq_num += 1;
						packetSize = 16;
					}
				
					rc = sendto(sd, packet, packetSize, 0, (struct sockaddr *)&server_address, sizeof(server_address));
					if(rc < 0){
						perror("send");
						exit(1);
					}
				}
			}

			/* if ACK then change top and bottom window */
			memset(bufferRead, 0, 100);
			if(recvfrom(sd, &bufferRead, 100, MSG_DONTWAIT, (struct sockaddr *)&from_address, &fromLength) > 0){ 
				sscanf(bufferRead, "%11d", &ackNumber);
			
				if(ackNumber == bottomWindow){
					if(bottomWindow + 2 <= stringSize){
						bottomWindow += 2;
					}
					else{
						bottomWindow++;
					}
					int j = 0;
					while(topWindow < stringSize && j < 2){
						topWindow++;
						j++;
					}
					printf("Recieved ack num %d\n", ackNumber);
					break;
				}
			}	
		
		}

	
	}
	
}


