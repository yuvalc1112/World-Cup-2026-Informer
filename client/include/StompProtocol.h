#pragma once
#include "../include/ConnectionHandler.h"
#include "../include/event.h" 
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <mutex>
#include <atomic>


class StompProtocol {
private:
    int subscriptionIdCounter;
    int receiptIdCounter;
    std::string currentUser;
    std::atomic<bool> isConnected;
    int disconnectReceiptId;
    std::map<std::string, int> activeSubscriptions;
    std::map<std::string, std::map<std::string, std::vector<Event>>> gameEventsHistory;
    std::mutex eventsMutex;

public:
    StompProtocol();
    std::vector<std::string> processInput(std::string input);
    static std::vector<std::string> split(const std::string &str, char spliter);
    
    void saveEvent(const std::string& gameName, const std::string& currentUser, const Event& event);
    bool processServerFrame(const std::string& frame);
    void setConnected(bool status) { isConnected = status; }
    bool getConnected() const { return isConnected; }
};