#ifndef ITEM_RECORD_H
#define ITEM_RECORD_H

#include <string>
#include <vector>
#include <iostream>

enum class ReportType {
    LOST,
    FOUND,
    UNKNOWN
};

inline std::string reportTypeToString(ReportType type) {
    switch (type) {
        case ReportType::LOST:  return "LOST";
        case ReportType::FOUND: return "FOUND";
        default:                return "UNKNOWN";
    }
}

inline ReportType stringToReportType(const std::string& str) {
    if (str == "LOST" || str == "lost") return ReportType::LOST;
    if (str == "FOUND" || str == "found") return ReportType::FOUND;
    return ReportType::UNKNOWN;
}

struct ItemRecord {
    int id;                         // Unique report ID
    ReportType type;                // LOST or FOUND
    std::string category;           // phone, bag, wallet, etc.
    std::string name;               // Brand and model phrase
    std::string color;              // Color of the item
    std::string location;           // Campus location
    std::string description;        // Free-text description
    int pair_id;                    // Hidden ground truth (-1 if unmatched)

    // Tokenized and normalized words (populated by M1 Normalizer)
    std::vector<std::string> name_words;
    std::vector<std::string> description_words;

    ItemRecord() : id(-1), type(ReportType::UNKNOWN), pair_id(-1) {}

    ItemRecord(int id, ReportType type, const std::string& category,
               const std::string& name, const std::string& color,
               const std::string& location, const std::string& description,
               int pair_id = -1)
        : id(id), type(type), category(category), name(name),
          color(color), location(location),
          description(description), pair_id(pair_id) {}

    // Print item summary
    void print() const {
        std::cout << "[" << id << "] " << reportTypeToString(type)
                  << " | Cat: " << category
                  << " | Name: \"" << name << "\""
                  << " | Color: " << color
                  << " | Loc: " << location
                  << " | Desc: \"" << description << "\""
                  << " | PairID: " << pair_id << "\n";
    }

    // Convert to CSV line
    std::string toCSV() const;

    // Parse from CSV line
    static ItemRecord fromCSV(const std::string& line);

    // Load multiple records from a CSV file
    static std::vector<ItemRecord> loadFromCSV(const std::string& filepath);
};

#endif // ITEM_RECORD_H
