#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

using namespace std;

#define BUFLEN 256

enum CommandType {
    SUBSCRIBE,
    UNSUBSCRIBE,
    EXIT,
    UNKNOWN
};

// Server message structure
struct ServerMessage {
	string ip; // IP of UDP client
	int port;
    char topic[50];
    uint8_t type;
	string value;
};


CommandType getCommandType(const char *cmd) {
    if (strcmp(cmd, "subscribe") == 0) {
        return SUBSCRIBE;
    }
    else if (strcmp(cmd, "unsubscribe") == 0) {
        return UNSUBSCRIBE;
    }
    else if (strcmp(cmd, "exit") == 0) {
        return EXIT;
    }
    else {
        return UNKNOWN;
    }
}

// Parse message received from server
bool parseServerMessage(const char *buffer, ServerMessage &message) {
    // Parse message
    char ip[BUFLEN], topic[BUFLEN], type[BUFLEN], value[BUFLEN];
    int port;
	// ip string read until :, int port read, rest of strings read until - or \n
    int n = sscanf(buffer, "%[^:]:%d - %[^-] - %[^-] - %[^\n]", ip, &port, topic, type, value);
    if (n != 5) {
        return false;
    }

    // Store parsed values in message struct
    message.ip = ip;
    message.port = port;
    strncpy(message.topic, topic, sizeof(message.topic));
    message.type = atoi(type);
    message.value = value;

    return true;
}

int main(int argc, char *argv[]) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Check command line arguments
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <ID_CLIENT> <IP_SERVER> <PORT_SERVER>" << endl;
        return -1;
    }

    // Parse command line arguments
    string id_client = argv[1];
    string ip_server = argv[2];
    int port_server = atoi(argv[3]);

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error creating socket" << endl;
        return -1;
    }

    // Set up server address structure
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_server);
    if (inet_aton(ip_server.c_str(), &serv_addr.sin_addr) == 0) {
        cerr << "Invalid server IP address" << endl;
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error connecting to server" << endl;
        return -1;
    }

    // Send ID_CLIENT to server
	int n = send(sockfd, id_client.c_str(), id_client.length(), 0);
	if (n < 0) {
    	cerr << "Error sending ID_CLIENT to server" << endl;
    	return -1;
	}

    // Main loop
    while (true) {
    	// Set up file descriptor sets for select()
    	fd_set read_fds;
    	FD_ZERO(&read_fds);		//Initialise read_fds set to be empty
    	FD_SET(STDIN_FILENO, &read_fds);	// Add Standard Input file descriptor into the set
    	FD_SET(sockfd, &read_fds);		// Add the socket file descriptor into the set

        // Call select() - will block until one of the FD inside the read_fds set becomes ready for reading
    	int maxfd = sockfd;
    	int n = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
    	if (n < 0) {
        	cerr << "Error calling select" << endl;
        	return -1;
    	}

    	// Check if data is available on stdin
    	if (FD_ISSET(STDIN_FILENO, &read_fds)) {
        	// Read command from stdin
        	char buffer[BUFLEN];
        	memset(buffer, 0, BUFLEN);
        	if (fgets(buffer, BUFLEN - 1, stdin) == NULL) {
            	cerr << "Error reading command from stdin" << endl;
            	return -1;
        	}

        	// Parse command
        	char cmd[BUFLEN/3], topic[BUFLEN/3];
        	int sf;
        	int n = sscanf(buffer, "%s %s %d", cmd, topic, &sf);
        	if (n < 1) {
            	cerr << "Invalid command" << endl;
            	continue;
        	}


        	// Handle commands
        	switch (getCommandType(cmd)) {
            	case SUBSCRIBE:
                	if (n != 3) {
                    	cerr << "Invalid subscribe command" << endl;
                    	continue;
                	}

                	// Send subscribe command to server
            		sprintf(buffer, "%s %s %d %s", cmd, topic, sf, id_client);	// Format message containing subscribe command and store it in buffer
					cout<<"Subscribed to topic.\n";
					buffer[strlen(buffer) + 1] = '\0';
            		if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                		cerr << "Error sending subscribe command to server" << endl;
                		return -1;
            		}
                	break;

            	case UNSUBSCRIBE:
                	if (n != 2) {
                    	cerr << "Invalid unsubscribe command" << endl;
                    	continue;
                	}

            		// Send unsubscribe command to server
            		sprintf(buffer, "%s %s %s", cmd, topic, id_client);	// Analogue to send subscribe command to server
            		if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                		cerr << "Error sending unsubscribe command to server" <<endl;
                		return -1;
            		}
                	break;

            	case EXIT:
                	// Pass Client ID to server, disconnect from server and exit
					sprintf(buffer, "%s", id_client);
					close(sockfd);
					exit(0);
                	break;

            	case UNKNOWN:
                	cerr << "Unknown command" << endl;
                	continue;

            	default:
                	break;
        	}
    	}

    	// Check if data is available on sockfd
    	if (FD_ISSET(sockfd, &read_fds)) {
        	// Read message from server
        	char buffer[BUFLEN];
        	memset(buffer, 0, BUFLEN);
        	int n = recv(sockfd, buffer, sizeof(buffer), 0);
        	if (n < 0) {
            	cerr << "Error receiving message from server" << endl;
            	return -1;
        	}
        	else if (n == 0) {
            	cerr << "Server closed connection" << endl;
            	return -1;
        	}

			cout << buffer<< endl;
    	}

    }

    // Close socket
    close(sockfd);

    return 0;
}