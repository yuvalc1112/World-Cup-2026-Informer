#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

void SocketListener (ConnectionHandler & connectionHandler, StompProtocol& protocol, bool& shouldTerminate) {
	while (!shouldTerminate) {
		std::string frame;
		if (!connectionHandler.getFrameAscii(frame, '\0')) {
			shouldTerminate = true;
			break;
		}
		if (!protocol.processServerFrame(frame)) {
			shouldTerminate = true;
			connectionHandler.close();
			break;
		}
	}
}
void sendFrame(ConnectionHandler & connectionHandler, StompProtocol& protocol, const std::string& line, bool& shouldTerminate){
	std::vector<std::string> framesToSend = protocol.processInput(line);   
        
		for (const std::string& frame : framesToSend) {
			if (!connectionHandler.sendFrameAscii(frame, '\0')) {
				std::cout << "Could not send frame to server" << std::endl;
				shouldTerminate = true;
				break;
			}
		}
}


int main(int argc, char *argv[]) {
	std::string nextLine = "";
	while (true) {
		StompProtocol protocol;
		bool shouldTerminate = false;
		const short bufsize = 1024;
		char buf[bufsize];
		std::string host = "";
		short port = 0;
		
		std::string line;
		while(1){
			if(nextLine.empty()){
				std::cin.getline(buf, bufsize);
				line = buf;
			}
			else{
				line = nextLine;
				nextLine = "";
			}	
			if (line.find("login") == 0) {
				size_t firstSpace = line.find(' ');
				size_t secondSpace = line.find(' ', firstSpace + 1);
				if (firstSpace == std::string::npos || secondSpace == std::string::npos) {
					std::cout << "Usage: login {host:port} {username} {password}" << std::endl;
					continue;
				}
				std::string hostPort = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
				size_t colonPos = hostPort.find(':');
				
				if (colonPos != std::string::npos) {
					host = hostPort.substr(0, colonPos);
					port = (short)stoi(hostPort.substr(colonPos + 1));
					break;
				}
			}
			else{
			
				std::cout << "Error: You must login first (command must start with 'login')." << std::endl;
				
			}
		}
		
		ConnectionHandler connectionHandler(host, port);
		if (!connectionHandler.connect()) {
			std::cerr << "Cannot connect to " << host << ":" << port << std::endl;
			continue;
		}
		std::thread listenerThread(SocketListener, std::ref(connectionHandler), std::ref(protocol), std::ref(shouldTerminate));
		sendFrame(connectionHandler, protocol, std::string(buf), shouldTerminate);
		
		while(!shouldTerminate){
			std::cin.getline(buf, bufsize);
			std::string currentLine(buf);
			
			if (shouldTerminate){
				nextLine = currentLine;
				break;
			} 
			sendFrame(connectionHandler, protocol, currentLine, shouldTerminate);
			if(currentLine == "logout"){
				break;
			}
			
		}
		
		listenerThread.join(); 
		connectionHandler.close();
		
	}
	return 0;
}

