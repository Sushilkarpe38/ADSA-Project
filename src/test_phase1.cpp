#include "../include/ItemRecord.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>

int main(int argc, char* argv[]) {
    std::string csvPath = "data/reports_1k.csv";
    if (argc > 1) {
        csvPath = argv[1];
    }

    std::cout << "====================================================\n";
    std::cout << "    ReFind: Phase 1 Data Model & CSV Loader Test    \n";
    std::cout << "====================================================\n\n";

    std::cout << "[*] Loading dataset from: " << csvPath << " ...\n";
    std::vector<ItemRecord> records = ItemRecord::loadFromCSV(csvPath);

    if (records.empty()) {
        std::cerr << "[!] FAILED: No records loaded! Check file path.\n";
        return 1;
    }

    std::cout << "[+] SUCCESS: Loaded " << records.size() << " records.\n\n";

    // Gather statistics
    int lostCount = 0;
    int foundCount = 0;
    int pairedCount = 0;
    int noiseCount = 0;
    std::unordered_map<std::string, int> categoryCounts;
    std::unordered_set<int> uniquePairIds;

    for (const auto& rec : records) {
        if (rec.type == ReportType::LOST) lostCount++;
        else if (rec.type == ReportType::FOUND) foundCount++;

        if (rec.pair_id != -1) {
            pairedCount++;
            uniquePairIds.insert(rec.pair_id);
        } else {
            noiseCount++;
        }

        categoryCounts[rec.category]++;
    }

    std::cout << "--- Dataset Breakdown ---\n";
    std::cout << "Total Records:      " << records.size() << "\n";
    std::cout << "LOST Reports:       " << lostCount << "\n";
    std::cout << "FOUND Reports:      " << foundCount << "\n";
    std::cout << "Ground Truth Pairs: " << uniquePairIds.size() << " pairs (" << pairedCount << " items)\n";
    std::cout << "Unmatched Noise:    " << noiseCount << " items\n\n";

    std::cout << "--- Category Distribution ---\n";
    for (const auto& kv : categoryCounts) {
        std::cout << "  - " << std::left << std::setw(15) << kv.first << ": " << kv.second << "\n";
    }

    std::cout << "\n--- Sample Loaded Records ---\n";
    for (size_t i = 0; i < std::min((size_t)5, records.size()); ++i) {
        std::cout << "Record " << (i + 1) << ":\n";
        records[i].print();
        std::cout << "\n";
    }

    std::cout << "[+] Phase 1 validation completed successfully without zone!\n";
    return 0;
}
