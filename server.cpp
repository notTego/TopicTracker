#include <iostream>
#include <string.h>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <map>

#define MAX_CLIENTS 10

using namespace std;

#define BUFLEN 256

struct Subscription {
    string topic;
    int sf;
};

struct ServerMessage {
	string ip; // IP of UDP client
	int port;
    char topic[50];
    uint8_t type;
	string value;
};

int get_sockfd_from_id(const map<int, string> &socket_id, const string &id) {
    int sockfd = -1;
    for (auto &entry : socket_id) {
        if (entry.second == id) {
            sockfd = entry.first;
            break;
        }
    }
    return sockfd;
}

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

void handle_exit(int sig, int tcp_sockfd, int udp_sockfd,
                unordered_map<string, vector<Subscription>> &subscriptions) {
    // Close all active TCP client connections
    // for (auto &entry : subscriptions) {
    //     int sockfd = entry.first;
    //     close(sockfd);
    // }

    // Close server sockets
    close(tcp_sockfd);
    close(udp_sockfd);

    cout << "Server shut down." << endl;
    exit(0);
}

// Parse INT and FLOAT
int32_t parse_buffer(char buffer[]) {
    int32_t result;
    memcpy(&result, buffer + 52, sizeof(result));
    result = ntohl(result);
    return result;
}

tuple<string, string, int> parse_command(char buffer[]) {
	char topic[50], type[50];
	int sf;
	sscanf(buffer, "%s %s %d", topic, type, &sf);
	return make_tuple(topic, type, sf);
}

int main(int argc, char *argv[]) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <PORT>" << endl;
        return 1;
    }

    int port = atoi(argv[1]);

    int tcp_sockfd, udp_sockfd;
    struct sockaddr_in serv_addr;

    // Create TCP socket
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
        cerr << "Error opening TCP socket" << endl;
        return 1;
    }

    // Create UDP socket
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        cerr << "Error opening UDP socket" << endl;
        return 1;
    }

    // Initialize server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind TCP socket to server address
    if (bind(tcp_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error on binding TCP socket" << endl;
        return 1;
    }

    // Bind UDP socket to server address
    if (bind(udp_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error on binding UDP socket" << endl;
        return 1;
    }
    // Listen for incoming TCP connections
    listen(tcp_sockfd, MAX_CLIENTS);

    // Initialize client tracking data structures
	// If tcp FD is greater than udp FD, max_fd = tcp FD, else max_fd = udp FD
    int max_fd = tcp_sockfd > udp_sockfd ? tcp_sockfd : udp_sockfd;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(tcp_sockfd, &read_fds);
    FD_SET(udp_sockfd, &read_fds);
    unordered_map<string, vector<Subscription>> subscriptions;
	map<int, string> socket_id;
	map<string, vector<string>> topics;

	int newsockfd;
	char id[BUFLEN];

    while (true) {
        fd_set tmp_fds = read_fds;
        if (select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
            perror("Error in select");
            exit(1);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    // Handle input from server command line
                    string input;
                    getline(cin, input);
                    if (input == "exit") {
                        handle_exit(0, tcp_sockfd, udp_sockfd, subscriptions);
                    } else if (input == "test") {
						char buffer_test [200]= "1.2.3.4:4573 - UPB/precis/1/temperature - SHORT_REAL - 23.5";
						cout <<buffer_test<< endl;
						// Send buffer test to tcp client
						int n = send(newsockfd, buffer_test, sizeof(buffer_test), 0);
							if (n == -1) {
    							perror("Error in send");
							} else {
    							cout << "Sent " << n << " bytes to TCP client" << endl;
							}
						
					}
                } else if (i == tcp_sockfd) {
                    // Handle new incoming TCP connection
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cli_addr, &clilen);
                    if (newsockfd == -1) {
                        perror("Error in accept");
                    } else {
                        // Add new client to tracking data structures
                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd > max_fd) {
                            max_fd = newsockfd;
                        }

						memset(id, 0, BUFLEN);
          				int n = recv(newsockfd, id, BUFLEN, 0);
                        subscriptions[id] = {};
                        printf("New client %s connected from %s:%d.\n", id,
                               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

						// Add socketfd - client id correspondence
						socket_id.insert(pair<int, string>(newsockfd, id));
                    }
                } else if (i == udp_sockfd) {
                    // Handle incoming UDP message
                    char buffer[2000];
					memset(buffer, 0, 2000);
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    int n = recvfrom(udp_sockfd, buffer, sizeof(buffer), 0,
                                     (struct sockaddr *) &cli_addr, &clilen);
                    if (n < 0) {
                        perror("Error in recvfrom");
                    } else {
                        // Process UDP message and send to subscribed TCP clients
						char topic[51];
						memset(topic, 0, 51);
						strncpy(topic, buffer, 49);

						int type = (int8_t)buffer[50];

						char content[1501];
						memset(content, 0, 1500);
						strncpy(content, buffer + 51, 1499);


						int32_t parsed;
						long long int_value;
						float float_value;
						char port[6];
						sprintf(port, "%u", htons(cli_addr.sin_port));
						char payload[2000] = "";

						// Tests failed for some reason if the payload contained "<ip>:<port>-"
						//strcat(payload, strcat(strcat(inet_ntoa(cli_addr.sin_addr), ":" ), port));
						//strcat(strcat(strcat(payload, " - "), topic), " - ");
						strcat(strcat(payload, topic), " - ");
						char value[1500];

						switch(type) {
							case 0:
              					parsed = parse_buffer(buffer);
              					// Adjust for sign byte
								if (buffer[51] == 0) {
                					int_value = (long long)parsed;
              					} else {
                					int_value = (long long)parsed * (-1);
              					}
								sprintf(value, "%lld", int_value);
								strcat(strcat(payload, "INT - "), value);
								break;

							case 1:
    							parsed = (static_cast<int8_t>(buffer[51]) << 8) + static_cast<int8_t>(buffer[52]);
    							float_value = static_cast<float>(parsed) / 100;

								sprintf(value, "%f", float_value);
								strcat(strcat(payload, "SHORT REAL - "), value);
								break;
							
							case 2:
							    parsed = parse_buffer(buffer);
              					float_value =
                  					((float)parsed) /
                  					(float)((int32_t)pow(10, (int8_t)buffer[56]));
								
								sprintf(value, "%f", float_value);
								strcat(strcat(payload, "FLOAT - "), value);
								break;

							case 3:
								strncat(strcat(payload, "STRING - "), content, 1500);
								break;
							
							default:
								cout<<endl;
								break;
						}

						//Send payload to tcp clients
						for (auto &entry : subscriptions) {
							string id = entry.first;
							vector<Subscription> subs = entry.second;
							for (auto &sub : subs) {
								if(strlen(topic) == 50)
									topic[51] = '\0';
								if (sub.topic == topic) {
									int sockfd = get_sockfd_from_id(socket_id, id);
									if (sockfd == -1) {
										// Handle error: client with given ID not found
										perror("ID not found");
									} else {
										cout<<"Sending payload:\n";
										int n = send(sockfd, payload, strlen(payload), 0);
										if (n < 0) {
											perror("Error in send");
										}
									}
								}
							}
						}
					}
                } else {
                    // Handle incoming data from TCP client
                    char buffer[1024];
					memset(buffer, 0, 1024);
                    int n = recv(i, buffer, sizeof(buffer), 0);
                    if (n < 0) {
                        perror("Error in recv");
                    } else if (n == 0) {
                        // Handle TCP client disconnection
						cout << "Client "<< socket_id.find(i)->second<< " disconnected.\n";
                        close(i);
                        FD_CLR(i, &read_fds);
                        //subscriptions.erase(i);
                    } else {
                        printf("Received data from TCP client %d.\n", i);
                        // Process data from TCP client
						string command, topic;
						int sf;

						tie(command, topic, sf) = parse_command(buffer);

						if (command == "subscribe") {
    						Subscription sub = {topic, sf};
							vector<Subscription> aux_vec = subscriptions.find(socket_id.find(i)->second)->second;
							aux_vec.push_back(sub);
							subscriptions[socket_id.find(i)->second] = aux_vec;
							topics.insert(make_pair(socket_id.find(i)->second, vector<string>{topic}));

    						cout << "Client " << socket_id.find(i)->second << " subscribed to " << topic << endl;
						} else if (command == "unsubscribe") {
    						bool topic_invalid = false;

    						vector<Subscription> &vec = subscriptions.find(socket_id.find(i)->second)->second;
    						auto it = std::find_if(vec.begin(), vec.end(),
        						[&topic](const Subscription& sub) { return strncmp(topic.c_str(), sub.topic.c_str(), strlen(topic.c_str())) == 0; });

    						if (it == vec.end()) {
        						fprintf(stderr, "Topic inexistent\n");
        						exit(0);
    						}

    						vec.erase(it);

    						cout << "Client " << socket_id.find(i)->second << " unsubscribed from " << topic << endl;
						}

						// Show every topic client is subscribed to (Debugging)
						// for (auto sub : subscriptions[socket_id.find(i)->second]) {
						// 	cout << sub.topic << " " << sub.sf << endl;
						
						// }
                    }
                }
            }
        }
    }

    close(tcp_sockfd);
    close(udp_sockfd);

    return 0;
}
