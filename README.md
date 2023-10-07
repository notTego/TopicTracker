# TopicTracker

TopicTracker is a real-time data monitoring system that allows clients to subscribe and unsubscribe to specific topics and receive updates from a server using UDP and TCP communication protocols. This repository contains three key components: `server.cpp`, `subscriber.cpp`, and `udp_client.cpp`.

## Components

### `server.cpp`

`server.cpp` is the server-side component of TopicTracker. It manages incoming client connections, handles subscriptions, and broadcasts updates to subscribed clients. Here are some key features of `server.cpp`:

- Manages TCP and UDP sockets for communication with clients.
- Accepts incoming TCP connections and registers clients.
- Listens for incoming UDP messages and forwards them to subscribed TCP clients.
- Supports subscribing and unsubscribing to topics.
- Handles server commands from the standard input, such as "exit" for graceful shutdown.

### `subscriber.cpp`

`subscriber.cpp` represents the client-side subscriber component of TopicTracker. Clients can connect to the server, subscribe to topics of interest, and receive real-time updates. Key features of `subscriber.cpp` include:

- Establishes a TCP connection with the server.
- Subscribes and unsubscribes from topics.
- Listens for updates from the server.
- Parses server messages and displays them to the user.
- Allows graceful disconnection from the server.

### `udp_client.cpp`

`udp_client.cpp` is a utility program that allows you to send UDP messages to the TopicTracker server for testing purposes. It takes a server IP address, port, and message as command-line arguments and sends the message to the specified server.

## Usage

Before running the components, make sure to compile them using the provided Makefile. Since the Makefile is included, you can skip build instructions.

### `server.cpp`

Run the server using the following command:

```bash
./server <PORT>
```

- `<PORT>` is the port number on which the server will listen for incoming connections.

### `subscriber.cpp`

Run the subscriber using the following command:

```bash
./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
```

- `<ID_CLIENT>` is the unique identifier for the subscriber.
- `<IP_SERVER>` is the IP address of the TopicTracker server.
- `<PORT_SERVER>` is the port number on which the server is listening.

### `udp_client.cpp`

Run the UDP client for testing purposes using the following command:

```bash
./udp_client <SERVER_IP> <SERVER_PORT> <MESSAGE>
```

- `<SERVER_IP>` is the IP address of the TopicTracker server.
- `<SERVER_PORT>` is the UDP port on which the server is listening.
- `<MESSAGE>` is the message you want to send to the server.