#include "TCPClientServer.h"

/* Инициализирует сокет для сервера или клиента, в зависимости от наличия или отсутствия адреса */
int InitSocket(const char* serverAddr, int listenPort)
{
	int sockfd = -1;
	struct sockaddr_in servAddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		fprintf(stderr, "Could not create socket\n");
		return -1;
	}
	memset(&servAddr, 0, sizeof(sockaddr_in));

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(listenPort);

	if (serverAddr)
	{
		/* Client mode */
		if (inet_pton(AF_INET, serverAddr, &servAddr.sin_addr) <= 0)
		{
			fprintf(stderr, "Error in server address: %s\n", serverAddr);
			return -1;
		}
		if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
		{
			close(sockfd);
			fprintf(stderr, "Connect to %s:%d failed\n", serverAddr, listenPort);
			return -1;
		}
		
		fprintf(stdout, "Client connection to %s:%d succsess", serverAddr, listenPort);
		return sockfd;
	}
	else
	{
		/* Server mode */
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
		{
			close(sockfd);
			fprintf(stderr, "Bind to port %d failed\n", listenPort);
			return -1;
		}
		if (listen(sockfd, MaxConnection) < 0)
		{
			fprintf(stderr, "Listen port %d failed\n", listenPort);
			return -1;
		}
		
		int listenfd = -1;
		if ((listenfd = accept(sockfd, NULL, 0) < 0))
		{
			close(sockfd);
			fprintf(stderr, "Accept connection failed\n");
			return -1;
		}
		if (sockfd)
			close(sockfd);
		
		fprintf(stdout, "Server start listen on port %d", listenPort);
		return listenfd;
	}
}