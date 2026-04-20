#include "../include/StompProtocol.h"
#include <sstream>
#include <algorithm>
#include <mutex>
#include <fstream>

StompProtocol::StompProtocol() 
    : subscriptionIdCounter(0),
      receiptIdCounter(0),
      currentUser(""),
      isConnected(false),
      disconnectReceiptId(-1),
      activeSubscriptions(),
      gameEventsHistory(),
      eventsMutex()
{}
std::vector<std::string> StompProtocol::processInput(std::string input) {
    std::vector<std::string> framesToSend;
    std::vector<std::string> args = split(input, ' ');

    if (args.empty()) return framesToSend;
    std::string command = args[0];
    if (command == "login") {
        if (isConnected) { 
            std::cout << "The client is already logged in, log out before trying again" << std::endl;
            return framesToSend;
        }
        if (args.size() < 4) {
            std::cout << "Usage: login {host:port} {username} {password}" << std::endl;
            return framesToSend;
        }
        currentUser = args[2]; 
        std::string password = args[3];
        
        std::string frame = "CONNECT\n";
        frame += "accept-version:1.2\n";
        frame += "host:stomp.cs.bgu.ac.il\n";
        frame += "login:" + currentUser + "\n";
        frame += "passcode:" + password + "\n";
        frame += "\n";

        framesToSend.push_back(frame);
    }
   else if (command == "join") {
        if (!isConnected) { 
            std::cout << "Please login first" << std::endl;
            return framesToSend;
        }
        if (args.size() < 2) {
            std::cout << "Usage: join {game_name}" << std::endl;
            return framesToSend;
        }
        std::string gameName = args[1];
        if (activeSubscriptions.count(gameName) > 0) {
            std::cout << "Already subscribed to " << gameName << std::endl;
            return framesToSend;
        } 
        int id = subscriptionIdCounter++;
        activeSubscriptions[gameName] = id; 

        std::string frame = "SUBSCRIBE\n";
        frame += "destination:/" + gameName + "\n";
        frame += "id:" + std::to_string(id) + "\n";
        frame += "receipt:" + std::to_string(receiptIdCounter++) + "\n";
        frame += "\n";

        framesToSend.push_back(frame);

    }

    else if (command == "exit") {
        if (!isConnected) { 
            std::cout << "Please login first" << std::endl;
            return framesToSend;
        }
        if (args.size() < 2) {
            std::cout << "Usage: exit {game_name}" << std::endl;
            return framesToSend;
        }

        std::string gameName = args[1];
        if (activeSubscriptions.find(gameName) == activeSubscriptions.end()) {
            std::cout << "Error: Not subscribed to " << gameName << std::endl;
            return framesToSend;
        }

        int id = activeSubscriptions[gameName]; 
        activeSubscriptions.erase(gameName);  

        std::string frame = "UNSUBSCRIBE\n";
        frame += "id:" + std::to_string(id) + "\n";
        frame += "receipt:" + std::to_string(receiptIdCounter++) + "\n";
        frame += "\n";
        framesToSend.push_back(frame);

    }

    else if (command == "report") {
        if (!isConnected) return framesToSend;
        if (args.size() < 2) {
            std::cout << "Usage: report {file_path}" << std::endl;
            return framesToSend;
        }
        std::string jsonPath = args[1];
        try {

            names_and_events parsedData = parseEventsFile(jsonPath);
            
            std::string gameName = parsedData.team_a_name + "_" + parsedData.team_b_name;
            for (const Event& event : parsedData.events) {
                std::string frame = "SEND\n";
                frame += "destination:/" + gameName + "\n";
                frame += "file: " + jsonPath.substr(jsonPath.find_last_of('/') + 1, jsonPath.find('.') - jsonPath.find_last_of('/') - 1) + "\n";
                frame += "\n"; 
                
                std::string body = "user:" + currentUser + "\n";
                body += "team a: " + parsedData.team_a_name + "\n";
                body += "team b: " + parsedData.team_b_name + "\n";
                body += "event name: " + event.get_name() + "\n";
                body += "time: " + std::to_string(event.get_time()) + "\n";

                body += "general game updates: \n";
                for (auto const& pair : event.get_game_updates()) {
                    body += "\t" + pair.first + ":" + pair.second + "\n";
                }
                
                body += "team a updates: \n";
                for (auto const& pair : event.get_team_a_updates()) {
                    body += "\t" + pair.first + ":" + pair.second + "\n";
                }
                
                body += "team b updates: \n";
                for (auto const& pair : event.get_team_b_updates()) {
                    body += "\t" + pair.first + ":" + pair.second + "\n";
                }
                
                body += "description: \n" + event.get_discription() + "\n";
                
                frame += body;
                framesToSend.push_back(frame);
            }
        } catch (const std::exception &e) {
            std::cout << "Error parsing file: " << jsonPath << std::endl;
            return framesToSend;
        }
    }
    else if (command == "summary") {
        if (!isConnected) return framesToSend;
        if (args.size() < 4) {
            std::cout << "Usage: summary {game_name} {user} {file}" << std::endl;
            return framesToSend;
        }
        std::string gameName = args[1];
        std::string user = args[2];
        std::string filePath = args[3];
        std::lock_guard<std::mutex> lock(eventsMutex);
        if (gameEventsHistory.find(gameName) == gameEventsHistory.end() || 
            gameEventsHistory[gameName].find(user) == gameEventsHistory[gameName].end()) {
            std::cout << "No events found for user " << user << " in game " << gameName << std::endl;
            return framesToSend;
        }
        const std::vector<Event>& events = gameEventsHistory[gameName][user];
        if (events.empty()) {
            std::cout << "No events to summarize." << std::endl;
            return framesToSend;
        }

        std::map<std::string, std::string> generalStats;
        std::map<std::string, std::string> teamAStats;
        std::map<std::string, std::string> teamBStats;

        for (const auto& event : events) {
            for (const auto& pair : event.get_game_updates()) {
                generalStats[pair.first] = pair.second;
            }
            for (const auto& pair : event.get_team_a_updates()) {
                teamAStats[pair.first] = pair.second;
            }
            for (const auto& pair : event.get_team_b_updates()) {
                teamBStats[pair.first] = pair.second;
            }
        }
        std::ofstream file(filePath); 
        if (!file.is_open()) {
            std::cout << "Error opening file: " << filePath << std::endl;
            return framesToSend;
        }

        std::string teamA = events[0].get_team_a_name();
        std::string teamB = events[0].get_team_b_name();

        file << teamA << " vs " << teamB << "\n";
        
        file << "Game stats:\n";
        
        file << "General stats:\n";
        for (const auto& pair : generalStats) {
            file << pair.first << ": " << pair.second << "\n";
        }
        
        file << teamA << " stats:\n";
        for (const auto& pair : teamAStats) {
            file << pair.first << ": " << pair.second << "\n";
        }
        
        file << teamB << " stats:\n";
        for (const auto& pair : teamBStats) {
            file << pair.first << ": " << pair.second << "\n";
        }

        file << "Game event reports:\n";
        for (const auto& event : events) {
            file << event.get_time() << " - " << event.get_name() << ":\n\n";
            file << event.get_discription() << "\n\n";
        }
        file.close();
        std::cout << "Summary created successfully in " << filePath << std::endl;
    }
    else if (command == "logout") {
        if (!isConnected) return framesToSend;
        std::string frame = "DISCONNECT\n";
        frame += "receipt:" + std::to_string(receiptIdCounter) + "\n";
        frame += "\n";
        framesToSend.push_back(frame);
        disconnectReceiptId = receiptIdCounter++;
    }
    else {
        std::cout << "Unknown command: " << command << std::endl;
    }

    return framesToSend;
}

std::vector<std::string> StompProtocol::split(const std::string &str, char spliter) {
    std::vector<std::string> tokens;
    std::string currentToken = ""; 
    for (size_t i = 0; i < str.length(); i++) {
        char ch = str[i];

        if (ch == spliter) {
            tokens.push_back(currentToken);
            currentToken = ""; 
        } else {
            currentToken += ch;
        }
    }
    tokens.push_back(currentToken);

    return tokens;
}

void StompProtocol::saveEvent(const std::string& gameName, const std::string& currentUser, const Event& event) {
    std::lock_guard<std::mutex> lock(eventsMutex);
    gameEventsHistory[gameName][currentUser].push_back(event);
}



bool StompProtocol::processServerFrame(const std::string& frame){
    if (frame.length() == 0) {
        return true; 
    }

    size_t firstNewLine = frame.find('\n');
    

    if (firstNewLine == std::string::npos) {
        return true; 
    }

    std::string command = frame.substr(0, firstNewLine);

    if (command == "MESSAGE") {
        try {
            size_t bodyPos = frame.find("\n\n");
            if (bodyPos == std::string::npos) return true; 
            std::string headers = frame.substr(0, bodyPos);
            std::string body = frame.substr(bodyPos + 2);

            std::string gameName = "";
            std::vector<std::string> headerLines = split(headers, '\n');
            for (const std::string& line : headerLines) {
                if (line.find("destination:") == 0) {
                    gameName = line.substr(12); 
                    if (gameName.find('/') != std::string::npos) {
                        gameName = gameName.substr(1); 
                    }
                }
            }
            

            Event event(body); 
            std::string user = "";
            std::vector<std::string> bodyLines = split(body, '\n');
            for (const std::string& line : bodyLines) {
                if (line.find("user:") == 0) {
                    user = line.substr(5);
                    break;
                }
            }

            if (gameName != "" && user != "") {
                saveEvent(gameName, user, event);
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error parsing MESSAGE: " << e.what() << std::endl;
        }
        return true; 
    }
    else if (command == "RECEIPT") {
        std::string receiptIdStr = "";
        std::vector<std::string> lines = split(frame, '\n');
        for (const std::string& line : lines) {
            if (line.find("receipt-id:") == 0) {
                receiptIdStr = line.substr(11);
                break;
            }
        }
        int receivedId = -1;
        
        if (!receiptIdStr.empty()){
            try { receivedId = std::stoi(receiptIdStr); } catch (...) {}
        } 
        
        if (receivedId == disconnectReceiptId) {
            std::cout << "Logout confirmed. Disconnecting..." << std::endl;
            isConnected = false;
            return false; 
        } 
        
        return true;
    }
    else if (command == "CONNECTED") {
        std::cout << "Login successful." << std::endl;
        isConnected = true;
        return true;
    }
    else if (command == "ERROR") {
        std::cout << frame << std::endl;
        isConnected = false;
        
        return false; 
    }
    return true;
}