World Cup 2026 Informer 🏆

Project Overview 🌍

A community-led sports update subscription service built on a robust client-server architecture.

Utilizes the Simple-Text-Oriented-Messaging-Protocol (STOMP 1.2) for asynchronous message passing.

Users can subscribe to specific World Cup game channels, report live events, and receive real-time updates from other subscribed users.

System Architecture 🏗️

Server (Java): Supports both Thread-Per-Client (TPC) and Reactor concurrency models. It manages topic subscriptions, message routing, and client connections.

Client (C++): A multithreaded application that simultaneously handles user console input and asynchronous socket reading.

Database (SQLite & Python): Persistently tracks user registrations, login timestamps, logout events, and uploaded file histories via a custom Python-based SQL execution server.

Building and Running the Database Server 🗄️

The SQLite database is managed through a Python SQL server. This server must be running before the Java server initiates database operations.

Run the Python Server: Execute the provided Python script to start the SQL server (e.g., python3 server.py).

Functionality: It receives SQL strings from the Java server, executes them on the SQLite database, and returns the results. It successfully tracks new user registrations, login/logout timestamps, and file tracking.

Server-Side Report: The system also includes a server-side report function that prints a summary of registered users, their login history, and uploaded files using SQL queries.

Building and Running the STOMP Server 🖥️

The server is built using Maven. From the server directory, run the following commands:

Compile: mvn compile

Run TPC: mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args=" tpc"

Run Reactor: mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args=" reactor"

Building and Running the Client 💻

The client is built using Make. From the client directory, run the following commands:

Compile: make

Run: Execute the generated binary ./bin/StompWCIClient

Supported Client Commands 🎮

login {host:port} {username} {password}: Connects and authenticates the user with the server.

join {game_name}: Subscribes the user to receive updates for a specific game.

exit {game_name}: Unsubscribes the user from the game channel.

report {file.json}: Parses a JSON event file and broadcasts the game updates to all subscribed users on that channel.

summary {game_name} {user} {file}: Generates a summarized report of all events reported by a specific user for a specific game and outputs it to a file.

logout: Gracefully disconnects the client from the server.
