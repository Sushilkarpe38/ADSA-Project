#include "../include/ItemRecord.h"
#include "../include/Normalizer.h"
#include "../include/CategoryIndex.h"
#include "../include/Trie.h"
#include "../include/CandidateGenerator.h"
#include "../include/Scorer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
#include <fstream>

class ReFindDemoApp {
private:
    Normalizer normalizer;
    CategoryIndex categoryIndex;
    Trie foundTrie;   // Inverted index for FOUND items
    Trie lostTrie;    // Inverted index for LOST items
    std::list<ItemRecord> allItems; // doubly-linked list preserves pointer stability on push_back
    std::unordered_map<int, const ItemRecord*> itemLookup;
    int foundCount;
    int lostCount;
    int nextId;
    std::string currentCsvPath;

public:
    ReFindDemoApp() : foundCount(0), lostCount(0), nextId(1) {}

    bool initialize(const std::string& csvPath = "") {
        // Detect whether running from project root or inside src/
        std::string prefix = "";
        std::ifstream testRoot("data/reports_1k.csv");
        if (!testRoot.is_open()) {
            std::ifstream testSrc("../data/reports_1k.csv");
            if (testSrc.is_open()) {
                prefix = "../";
            }
        }

        normalizer.loadStopWords(prefix + "data/stopwords.txt");
        normalizer.loadSynonyms(prefix + "data/synonyms.txt");

        currentCsvPath = csvPath.empty() ? (prefix + "data/reports_1k.csv") : csvPath;
        std::vector<ItemRecord> loaded = ItemRecord::loadFromCSV(currentCsvPath);
        if (loaded.empty()) {
            return false;
        }

        allItems.clear();
        categoryIndex.clear();
        foundTrie.clear();
        lostTrie.clear();
        itemLookup.clear();
        foundCount = 0;
        lostCount = 0;
        nextId = 1;

        for (auto& item : loaded) {
            normalizer.prepareItem(item);
            allItems.push_back(item);
            if (item.id >= nextId) {
                nextId = item.id + 1;
            }
        }

        for (auto& item : allItems) {
            itemLookup[item.id] = &item;
            categoryIndex.insert(&item);

            if (item.type == ReportType::FOUND) {
                foundCount++;
                for (const auto& w : item.name_words) foundTrie.insert(w, item.id);
                for (const auto& w : item.description_words) foundTrie.insert(w, item.id);
            } else if (item.type == ReportType::LOST) {
                lostCount++;
                for (const auto& w : item.name_words) lostTrie.insert(w, item.id);
                for (const auto& w : item.description_words) lostTrie.insert(w, item.id);
            }
        }
        return true;
    }

    void executeSearchAndAdd(ItemRecord& query) {
        normalizer.prepareItem(query);

        ReportType targetType = (query.type == ReportType::LOST) ? ReportType::FOUND : ReportType::LOST;
        const Trie& targetTrie = (query.type == ReportType::LOST) ? foundTrie : lostTrie;
        int targetTotalCount = (query.type == ReportType::LOST) ? foundCount : lostCount;

        std::cout << "\n\n";
        std::cout << "SEARCH RESULTS :\n";
        std::cout << "\n";
        std::cout << "QUERY DETAILS:\n";
        std::cout << "  Type:        " << reportTypeToString(query.type) << "\n";
        std::cout << "  Category:    " << (query.category.empty() ? "(none)" : query.category) << "\n";
        std::cout << "  Name:        \"" << query.name << "\"\n";
        std::cout << "  Color:       " << (query.color.empty() ? "(none)" : query.color) << "\n";
        std::cout << "  Location:    " << (query.location.empty() ? "(none)" : query.location) << "\n";
        std::cout << "  Description: \"" << query.description << "\"\n";

        std::cout << "\nNORMALIZED TOKENS:\n";
        std::cout << "  Name Words: [ ";
        for (const auto& w : query.name_words) std::cout << "\"" << w << "\" ";
        std::cout << "]\n";
        std::cout << "  Description Words: [ ";
        for (const auto& w : query.description_words) std::cout << "\"" << w << "\" ";
        std::cout << "]\n";

        CandidateGenerator candidateGen(categoryIndex, targetTrie, itemLookup);
        Scorer scorer(targetTrie, targetTotalCount);

        auto start = std::chrono::high_resolution_clock::now();
        auto candidates = candidateGen.generateCandidates(query);

        // Raw candidate scoring in the order matched (unsorted)
        std::vector<std::pair<double, CandidateMatch>> scoredList;
        for (const auto& pair : candidates) {
            ScoreBreakdown sb = scorer.computeScore(query, pair.second);
            scoredList.push_back({sb.totalScore, pair.second});
        }

        auto end = std::chrono::high_resolution_clock::now();
        double latencyUs = std::chrono::duration<double, std::micro>(end - start).count();

        size_t bucketSize = categoryIndex.getCount(query.category, targetType);

        std::cout << "\nSEARCH METRICS:\n";
        std::cout << "  Total Records in Database: " << allItems.size() << "\n";
        std::cout << "  Category Bucket Size:      " << bucketSize << "\n";
        std::cout << "  Candidates Scored:         " << candidates.size() << "\n";
        std::cout << "  Search Time:               " << std::fixed << std::setprecision(2) << latencyUs << " microseconds ("
                  << (latencyUs / 1000.0) << " ms)\n";

        if (scoredList.empty()) {
            std::cout << "\nNo matching " << reportTypeToString(targetType)
                      << " records found.\n";
        } else {
            int countToShow = static_cast<int>(scoredList.size());
            std::cout << "\n\n";
            std::cout << "  ALL " << countToShow << " MATCHING " << reportTypeToString(targetType) << " ITEMS (ORDER AS MATCHED)\n";
            std::cout << "\n";

            for (int i = 0; i < countToShow; ++i) {
                const auto& cand = scoredList[i].second;
                ScoreBreakdown sb = scorer.computeScore(query, cand);

                std::cout << "\n[" << (i + 1) << "] ID: " << cand.item->id
                          << " | " << reportTypeToString(cand.item->type)
                          << " | Score: " << std::fixed << std::setprecision(1) << sb.totalScore << " / 100\n";
                std::cout << "    Name:        \"" << cand.item->name << "\"\n";
                std::cout << "    Category:    " << cand.item->category
                          << " | Color: " << (cand.item->color.empty() ? "(none)" : cand.item->color)
                          << " | Location: " << (cand.item->location.empty() ? "(none)" : cand.item->location) << "\n";
                std::cout << "    Description: \"" << cand.item->description << "\"\n";

                sb.printSummary();
            }
        }

        // -------------------------------------------------------------
        // ADD REPORT TO DATABASE & SAVE TO DISK (CSV PERSISTENCE)
        // -------------------------------------------------------------
        allItems.push_back(query);
        ItemRecord* storedItem = &allItems.back();
        itemLookup[storedItem->id] = storedItem;
        categoryIndex.insert(storedItem);

        if (storedItem->type == ReportType::FOUND) {
            foundCount++;
            for (const auto& w : storedItem->name_words) foundTrie.insert(w, storedItem->id);
            for (const auto& w : storedItem->description_words) foundTrie.insert(w, storedItem->id);
        } else if (storedItem->type == ReportType::LOST) {
            lostCount++;
            for (const auto& w : storedItem->name_words) lostTrie.insert(w, storedItem->id);
            for (const auto& w : storedItem->description_words) lostTrie.insert(w, storedItem->id);
        }

        // Permanently persist the new record to CSV file
        std::ofstream csvOut(currentCsvPath, std::ios::app);
        bool fileSaved = false;
        if (csvOut.is_open()) {
            csvOut << storedItem->toCSV() << "\n";
            csvOut.close();
            fileSaved = true;
        }

        std::cout << "\n---------------------------------------------------------------\n";
        std::cout << "[+] Registered " << reportTypeToString(storedItem->type)
                  << " item into database as Report ID #" << storedItem->id << "\n";
        if (fileSaved) {
            std::cout << "    [Persisted]: Saved permanently to " << currentCsvPath << "\n";
        } else {
            std::cout << "    [Warning]: In memory only; could not write to " << currentCsvPath << "\n";
        }
        std::cout << "    (Database now contains " << allItems.size() << " records: "
                  << lostCount << " LOST, " << foundCount << " FOUND)\n";
        std::cout << "---------------------------------------------------------------\n";
    }

    void handleInput(ReportType type) {
        ItemRecord query;
        query.id = nextId++;
        query.type = type;
        query.pair_id = -1;

        std::cout << "\n\n";
        std::cout << " ENTER :" << reportTypeToString(type) << " DETAILS\n";
        std::cout << "\n";

        std::cout << "1. Category (phone, laptop, wallet, bag, watch, card, keys, water bottle, umbrella, earphones, book, usb drive):\n   > ";
        std::getline(std::cin, query.category);

        std::cout << "2. Name / Model phrase:\n   > ";
        std::getline(std::cin, query.name);

        std::cout << "3. Color (optional, press Enter to skip):\n   > ";
        std::getline(std::cin, query.color);

        std::cout << "4. Location (optional, press Enter to skip):\n   > ";
        std::getline(std::cin, query.location);

        std::cout << "5. Description (optional, press Enter to skip):\n   > ";
        std::getline(std::cin, query.description);

        if (query.category.empty() && query.name.empty()) {
            std::cout << "[!] Please enter at least a category or name.\n";
            return;
        }

        executeSearchAndAdd(query);
    }

    void printStats() const {
        std::cout << "Database loaded: " << allItems.size() << " total records ("
                  << lostCount << " LOST, " << foundCount << " FOUND)\n";
    }
};

int main() {
    std::cout << "-----------------------------\n";
    std::cout << "ReFind: Lost & Found Matching                 \n";
    std::cout << "-----------------------------\n";

    ReFindDemoApp app;
    if (!app.initialize()) {
        std::cerr << "[!] Error loading dataset from data/reports_1k.csv\n";
        return 1;
    }
    app.printStats();

    while (true) {
        std::cout << "\nSelect Operation : \n";
        std::cout << "  [1] Search a LOST Item  (Finds matching Found items)\n";
        std::cout << "  [2] Search a FOUND Item (Finds matching Lost items)\n";
        std::cout << "  [0] Exit\n";
        std::cout << "\n";
        std::cout << "Select option [0-2]: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) break;

        if (choice == "1") {
            app.handleInput(ReportType::LOST);
        } else if (choice == "2") {
            app.handleInput(ReportType::FOUND);
        } else if (choice == "0") {
            std::cout << "\nExiting application.\n";
            break;
        } else {
            std::cout << "Invalid choice. Please enter 1, 2, or 0.\n";
        }
    }

    return 0;
}
