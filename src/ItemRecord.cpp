#include "../include/ItemRecord.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Helper to escape CSV field if necessary
static std::string escapeCSV(const std::string& field) {
    if (field.find(',') != std::string::npos || field.find('"') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

// Helper to parse a line with potential quoted commas
static std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                token += '"';
                ++i; // skip escaped quote
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    tokens.push_back(token);
    return tokens;
}

std::string ItemRecord::toCSV() const {
    std::stringstream ss;
    ss << id << ","
       << reportTypeToString(type) << ","
       << escapeCSV(category) << ","
       << escapeCSV(name) << ","
       << escapeCSV(color) << ","
       << escapeCSV(location) << ","
       << escapeCSV(description) << ","
       << pair_id;
    return ss.str();
}

ItemRecord ItemRecord::fromCSV(const std::string& line) {
    auto tokens = parseCSVLine(line);
    if (tokens.size() < 8) {
        return ItemRecord();
    }

    try {
        int id = std::stoi(tokens[0]);
        ReportType type = stringToReportType(tokens[1]);
        std::string category = tokens[2];
        std::string name = tokens[3];
        std::string color = tokens[4];
        std::string location = tokens[5];
        std::string description = tokens[6];
        int pair_id = std::stoi(tokens[7]);

        return ItemRecord(id, type, category, name, color, location, description, pair_id);
    } catch (...) {
        return ItemRecord();
    }
}

std::vector<ItemRecord> ItemRecord::loadFromCSV(const std::string& filepath) {
    std::vector<ItemRecord> records;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Error] Unable to open dataset file: " << filepath << std::endl;
        return records;
    }

    std::string line;
    bool isFirstLine = true;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Skip CSV header
        if (isFirstLine) {
            isFirstLine = false;
            if (line.find("id,") != std::string::npos || line.find("ID,") != std::string::npos) {
                continue;
            }
        }

        ItemRecord rec = ItemRecord::fromCSV(line);
        if (rec.id != -1) {
            records.push_back(rec);
        }
    }

    return records;
}
