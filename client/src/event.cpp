#include "../include/event.h"
#include "../include/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
using json = nlohmann::json;

Event::Event(std::string team_a_name, std::string team_b_name, std::string name, int time,
             std::map<std::string, std::string> game_updates, std::map<std::string, std::string> team_a_updates,
             std::map<std::string, std::string> team_b_updates, std::string discription)
    : team_a_name(team_a_name), team_b_name(team_b_name), name(name),
      time(time), game_updates(game_updates), team_a_updates(team_a_updates),
      team_b_updates(team_b_updates), description(discription)
{
}

Event::~Event()
{
}

const std::string &Event::get_team_a_name() const
{
    return this->team_a_name;
}

const std::string &Event::get_team_b_name() const
{
    return this->team_b_name;
}

const std::string &Event::get_name() const
{
    return this->name;
}

int Event::get_time() const
{
    return this->time;
}

const std::map<std::string, std::string> &Event::get_game_updates() const
{
    return this->game_updates;
}

const std::map<std::string, std::string> &Event::get_team_a_updates() const
{
    return this->team_a_updates;
}

const std::map<std::string, std::string> &Event::get_team_b_updates() const
{
    return this->team_b_updates;
}

const std::string &Event::get_discription() const
{
    return this->description;
}
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}
std::vector<std::string> split_str(const std::string& s, char spliter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = s.find(spliter); 

    while (end != std::string::npos) { 
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(spliter, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

Event::Event(const std::string &frame_body) : team_a_name(""), team_b_name(""), name(""), time(0), game_updates(), team_a_updates(), team_b_updates(), description("")
{
    std::vector<std::string> lines = split_str(frame_body, '\n');
    std::string current_map = ""; 

    for (const std::string& raw_line : lines) {
        std::string line = trim(raw_line);
        if (line.empty()) continue; 

        if (line == "general game updates:") {
            current_map = "general"; continue;
        } else if (line == "team a updates:") {
            current_map = "team_a"; continue;
        } else if (line == "team b updates:") {
            current_map = "team_b"; continue;
        } else if (line == "description:") {
            current_map = "description"; continue;
        }

        if (current_map == "description") {
            if (!description.empty()) description += "\n";
            description += line;
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string val = trim(line.substr(colonPos + 1));

            if (key == "user"){continue;} 
            else if (key == "team a") team_a_name = val;
            else if (key == "team b") team_b_name = val;
            else if (key == "event name") name = val;
            else if (key == "time") {
                try { time = std::stoi(val); } catch (...) { time = 0; }
            }
            else if (current_map == "general") game_updates[key] = val;
            else if (current_map == "team_a") team_a_updates[key] = val;
            else if (current_map == "team_b") team_b_updates[key] = val;
        }
    }

}

names_and_events parseEventsFile(std::string json_path)
{
    std::ifstream f(json_path);
    json data = json::parse(f);

    std::string team_a_name = data["team a"];
    std::string team_b_name = data["team b"];
    std::vector<Event> events;
    for (auto &event : data["events"])
    {
        std::string name = event["event name"];
        int time = event["time"];
        std::string description = event["description"];
        std::map<std::string, std::string> game_updates;
        std::map<std::string, std::string> team_a_updates;
        std::map<std::string, std::string> team_b_updates;
        for (auto &update : event["general game updates"].items())
        {
            if (update.value().is_string())
                game_updates[update.key()] = update.value();
            else
                game_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team a updates"].items())
        {
            if (update.value().is_string())
                team_a_updates[update.key()] = update.value();
            else
                team_a_updates[update.key()] = update.value().dump();
        }

        for (auto &update : event["team b updates"].items())
        {
            if (update.value().is_string())
                team_b_updates[update.key()] = update.value();
            else
                team_b_updates[update.key()] = update.value().dump();
        }
        
        events.push_back(Event(team_a_name, team_b_name, name, time, game_updates, team_a_updates, team_b_updates, description));
    }
    names_and_events events_and_names{team_a_name, team_b_name, events};

    return events_and_names;
}