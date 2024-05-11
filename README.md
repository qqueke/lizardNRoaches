# lizardNRoaches Game

Welcome to lizardNRoaches, a game where players take on the role of lizards trying to eat as many roaches as possible while avoiding wasps. This game comes in two parts:

## Part 1
In Part 1, the game consists of two separate components: the server and the client. The server is responsible for managing the game state and communicating with clients, while the client is responsible for displaying the game and sending user input to the server.

### Requirements
- Curses library
- Protocol Buffers
- ZeroMQ

### How to Compile and Run
1. Navigate to the `part1` or `part2` directory
2. Compile server and clients using `make`
3. Run server with `./bin/lizardsNroaches-server`
4. Run client with `./bin/Lizard-client`
5. If on Part1 run `./bin/Display-app` for UI
6. Add bots by running `./bin/Roaches-Client` and/or `./bin/Wasps-Client` (wasps are only available in Part2)

