

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <byteswap.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>


#define MAX_PENDING_CONNECTIONS		5
#define ETH_TRANCIEVE_LENGTH		2048

#define ifr_name 	"eth0"

uint32_t 			NetIF_isReady;

uint8_t  NetIF_IP_Address[4] = { 192U, 168U, 1U, 199U };
uint8_t  NetIF_Subnet_Address[4] = { 255U, 255U, 255U, 0U };
uint8_t  NetIF_Gateway_Address[4] = { 192U, 168U, 1U, 1U };
uint8_t  NetIF_PriDNS_Server_Address[4] = { 8U, 8U, 8U, 8U };
uint8_t  NetIF_SecDNS_Server_Address[4] = { 8U, 8U, 4U, 4U };
uint8_t  NetIF_MAC_Address[6] = { 0x1E, 0x30, 0x6C, 0xA2, 0x45, 0x6F };
uint16_t NetIF_Port_Number 						= 1001U;
uint64_t Default_SysIF_Serial							= (uint64_t)1981000001ULL;
uint8_t	 NetIF_Host_Name[15] = { "AMR_RELAY_V3" };
uint8_t	 NetIF_DHCP_STATUS						= 0U;


uint8_t				Connected;
uint32_t 			NumberOfByteRecieve;
uint8_t 			RecieveLan[ETH_TRANCIEVE_LENGTH];
uint32_t 			NumberOfByteSend;
uint8_t 			SendLan[ETH_TRANCIEVE_LENGTH];
uint8_t 			StartForDefault;
uint32_t 			EmacDataLenSend;
uint32_t 			EmacDataSend[1000];
uint32_t 			EmacDataLenReciev;
uint8_t 			EmacDataReciev[1000];
int32_t 			Network_Speed;

void *client_handler(void *arg)
{
	int client_sock = *(int *)arg;

	while (1) {
		bzero(RecieveLan, ETH_TRANCIEVE_LENGTH);
		ssize_t num_bytes = recv(client_sock, RecieveLan, ETH_TRANCIEVE_LENGTH, 0);

		if (num_bytes < 0) {
			perror("recv");
			close(client_sock);
			return NULL;
		}
		else if (num_bytes == 0) {
			printf("Client Socket %d disconnected\n", client_sock);
			close(client_sock);
			return NULL;
		}

		NumberOfByteRecieve = num_bytes;

		uint32_t timeout = 4000000;
		while (NumberOfByteSend == 0) {
			timeout -= 1;
			if (timeout == 0) break;
		}

		if (NumberOfByteSend > 0) {
			size_t len = NumberOfByteSend;
			if (send(client_sock, SendLan, len, 0) != len) {
				perror("send");
				close(client_sock);
				return NULL;
			}
			NumberOfByteSend = 0;
		}
		else {
			char buf[] = "ConnectionTimeout";
			size_t len = strlen(buf);

			if (send(client_sock, buf, len, 0) != (ssize_t)len) {
				perror("send");
				close(client_sock);
				return NULL;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int ret;
	//	char sz[] = "Hello, World!\n";	/* Hover mouse over "sz" while debugging to see its contents */
	//	printf("%s", sz);	
	//	fflush(stdout); /* <============== Put a breakpoint here */
	
	
		// Set NetIF on Linux (make command)
//	unsigned char command[100] = { 0 };
	//	sprintf((char*)command,
	//		"ifconfig %s %d.%d.%d.%d netmask %d.%d.%d.%d",
	//		ifr_name,
	//		NetIF_IP_Address[0],
	//		NetIF_IP_Address[1],
	//		NetIF_IP_Address[2],
	//		NetIF_IP_Address[3],
	//		NetIF_Subnet_Address[0],
	//		NetIF_Subnet_Address[1],
	//		NetIF_Subnet_Address[2],
	//		NetIF_Subnet_Address[3]);
	//	system(command); // Run Command in Linux Terminal
//	printf("NetIF updated sucessfully.\n%s\nIP: %d.%d.%d.%d \nNetmask: %d.%d.%d.%d\n", ifr_name, NetIF_IP_Address[0], NetIF_IP_Address[1], NetIF_IP_Address[2], NetIF_IP_Address[3], NetIF_Subnet_Address[0], NetIF_Subnet_Address[1], NetIF_Subnet_Address[2], NetIF_Subnet_Address[3]);
	
	// Socket create and verification
	int Server_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Server_Socket == -1) {
		printf("socket creation failed...\n");
		perror("socket");
		return -1;
	}
	else {
		printf("Socket successfully created..\n");
	}


	int optval = 1;
	if (setsockopt(Server_Socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		perror("setsockopt");
		close(Server_Socket);
		return -1;
	}

	// Assign IP, PORT
	struct sockaddr_in Server_Address;
	Server_Address.sin_family = AF_INET;
	Server_Address.sin_addr.s_addr = INADDR_ANY;
	Server_Address.sin_port = htons(50007);

	// Binding newly created socket to given IP and verification
	if ((bind(Server_Socket, (struct sockaddr*)&Server_Address, sizeof(Server_Address))) < 0) {
		printf("socket bind failed...\n");
		perror("bind");
		close(Server_Socket);
		return -1;
	}
	else {
		printf("Socket successfully binded.\n");
	}

	// Now server is ready to listen and verification
	if ((listen(Server_Socket, MAX_PENDING_CONNECTIONS)) < 0) {
		// SOMAXCONN
		printf("Listen failed.\n");
		perror("listen");
		close(Server_Socket);
		return -1;
	}
	else {
		printf("Server listening...\n");
	}

	while (1) {
		struct sockaddr_in client_addr = { 0 };
		socklen_t client_addrlen = sizeof(client_addr);

		int client_sock = accept(Server_Socket, (struct sockaddr *)&client_addr, &client_addrlen);
		if (client_sock < 0) {
			perror("accept");
			close(Server_Socket);
			return -1;
		}
		
		uint32_t ClientAddr =  (uint32_t) client_addr.sin_addr.s_addr;
		printf("Client %d.%d.%d.%d Connected\n", (uint8_t)(ClientAddr >> 0), (uint8_t)(ClientAddr >> 8), (uint8_t)(ClientAddr >> 16), (uint8_t)(ClientAddr >> 24));

		pthread_t tid;
		if (pthread_create(&tid, NULL, client_handler, &client_sock) != 0) {
			perror("pthread_create");
			close(client_sock);
			close(Server_Socket);
			return -1;
		}

		pthread_detach(tid);
	}

	pthread_exit(NULL);
	
	return 0;
}