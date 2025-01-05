# 50-Game

This project implements a simple multiplayer game using UDP sockets in C. The game allows two players to connect via a server and play a turn-based game where they try to reach a target score. The game features both server and client functionality, using non-blocking sockets for communication.

## Features

- **Multiplayer Support**: Two players can connect to each other and play the game.
- **UDP Communication**: The game is based on the User Datagram Protocol (UDP) for communication between clients and the server.
- **Non-blocking I/O**: The program uses non-blocking sockets for handling user input and network communication simultaneously.
- **Score Management**: Players take turns to modify the game score, with specific rules on how much they can change the score during their turn.
- **Graceful Shutdown**: The program can be closed safely using a termination signal (Ctrl+C), which will close the sockets and exit the program.

## Prerequisites

- A C compiler (e.g., `gcc`)
- POSIX compliant system (Linux, macOS)
- Basic understanding of C programming and UDP sockets

## Installation

1. Clone the repository:

    ```bash
    git clone https://github.com/yourusername/udp-game.git
    cd udp-game
    ```

2. Compile the code:

    ```bash
    gcc -o game game.c
    ```

## Usage

The game has two modes: **server** and **client**. The server is responsible for accepting connections from clients, while the client connects to the server and interacts with the game.

### Starting the Game

To start the server, use the following command:

```bash
./game <server_address> <port> [name]
```

- **`<server_address>`**: IP address of the server or "0.0.0.0" if it's running on the same machine.
- **`<port>`**: The port number for the game (e.g., 8080).
- **`[name]`**: (Optional) The name of the player (default is the server's address).

```bash
./game 0.0.0.0 8080 server_name
```

## Gameplay

- The game starts when both the server and the client are connected.
- Players take turns to enter a new score within a range of [1, 10] greater than the previous score.
- The game continues until one of the players reaches a score of 50 or more.
- You can type "end" to close the game at any time.

## Terminating the Game

To terminate the game, use the following command at any time:

```bash
end
```

Alternatively, use Ctrl+C to safely shut down the game.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
