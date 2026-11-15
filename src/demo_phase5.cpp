#include "../include/ItemRecord.h"
#include "../include/Normalizer.h"
#include "../include/CategoryIndex.h"
#include "../include/Trie.h"
#include "../include/CandidateGenerator.h"
#include "../include/Scorer.h"
#include "../include/MaxHeap.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <chrono>
#include <fstream>

class ReFindApp {
private:
    Normalizer normalizer;
    CategoryIndex categoryIndex;
    Trie foundTrie;   // Inverted index for FOUND items
    Trie lostTrie;    // Inverted index for LOST items
    std::deque<ItemRecord> allItems; // deque preserves pointer stability on push_back
    std::unordered_map<int, const ItemRecord*> itemLookup;
    int foundCount;
    int lostCount;
    int nextId;
    std::string currentCsvPath;

public:
    ReFindApp() : foundCount(0), lostCount(0), nextId(1) {}

    bool initialize(const std::string& csvPath = "") {
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

    bool saveAllToCSV() const {
        std::ofstream csvOut(currentCsvPath);
        if (!csvOut.is_open()) return false;

        csvOut << "id,type,category,name,color,location,description,pair_id\n";
        for (const auto& item : allItems) {
            csvOut << item.toCSV() << "\n";
        }
        return true;
    }

    void claimAndResolve(int itemId) {
        auto itLookup = itemLookup.find(itemId);
        if (itLookup == itemLookup.end() || !itLookup->second) {
            std::cout << "[!] Error: Item ID #" << itemId << " not found in database.\n";
            return;
        }

        const ItemRecord* itm = itLookup->second;
        ReportType resolvedType = itm->type;
        std::string cat = itm->category;

        // 1. Remove all words of the item from the Inverted Trie (M3)
        Trie& targetTrie = (resolvedType == ReportType::FOUND) ? foundTrie : lostTrie;
        for (const auto& w : itm->name_words) {
            targetTrie.remove(w, itemId);
        }
        for (const auto& w : itm->description_words) {
            targetTrie.remove(w, itemId);
        }

        // 2. Remove from Category Hash Map (M2)
        categoryIndex.remove(itemId, cat, resolvedType);

        // 3. Decrement counters
        if (resolvedType == ReportType::FOUND) foundCount--;
        else if (resolvedType == ReportType::LOST) lostCount--;

        // 4. Remove from itemLookup hash table
        itemLookup.erase(itemId);

        // 5. Remove from allItems deque
        for (auto it = allItems.begin(); it != allItems.end(); ++it) {
            if (it->id == itemId) {
                allItems.erase(it);
                break;
            }
        }

        // 6. Rewrite CSV to persist deletion on disk
        saveAllToCSV();

        std::cout << "\n---------------------------------------------------------------\n";
        std::cout << "[+] SUCCESS: Item claimed & resolved!\n";
        std::cout << "    Report ID #" << itemId << " (" << reportTypeToString(resolvedType)
                  << ") has been removed from active inventory.\n";
        std::cout << "    (Database now contains " << allItems.size() << " records: "
                  << lostCount << " LOST, " << foundCount << " FOUND)\n";
        std::cout << "---------------------------------------------------------------\n";
    }

    void registerNewReport(ItemRecord& query) {
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
        }
        std::cout << "    (Database now contains " << allItems.size() << " records: "
                  << lostCount << " LOST, " << foundCount << " FOUND)\n";
        std::cout << "---------------------------------------------------------------\n";
    }

    // Top-level matching driver with Max-Heap Top-K ranking (M6 & M7)
    void executeSearchAndProcess(ItemRecord& query, int K = 10, double minScoreThreshold = 40.0) {
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

        // Step 1: Candidate Generation (Module M4)
        CandidateGenerator candidateGen(categoryIndex, targetTrie, itemLookup);
        Scorer scorer(targetTrie, targetTotalCount);

        auto start = std::chrono::high_resolution_clock::now();
        auto candidates = candidateGen.generateCandidates(query);

        // Step 2: Score Survivors (Module M5)
        std::vector<ScoredCandidate> scoredList;
        scoredList.reserve(candidates.size());
        for (const auto& pair : candidates) {
            ScoreBreakdown sb = scorer.computeScore(query, pair.second);
            scoredList.emplace_back(sb.totalScore, pair.second, sb);
        }

        // Step 3: Max-Heap Top-K Extraction (Module M6)
        std::vector<ScoredCandidate> topKMatches = MaxHeap::extractTopK(scoredList, K, minScoreThreshold);

        auto end = std::chrono::high_resolution_clock::now();
        double totalLatencyUs = std::chrono::duration<double, std::micro>(end - start).count();

        size_t bucketSize = categoryIndex.getCount(query.category, targetType);

        std::cout << "\nSEARCH METRICS:\n";
        std::cout << "  Total Records in Database: " << allItems.size() << "\n";
        std::cout << "  Category Bucket Size:      " << bucketSize << "\n";
        std::cout << "  Candidates Scored:         " << candidates.size() << "\n";
        std::cout << "  Search Time:               " << std::fixed << std::setprecision(2) << totalLatencyUs << " microseconds ("
                  << (totalLatencyUs / 1000.0) << " ms)\n";

        if (topKMatches.empty()) {
            std::cout << "\nNo matching " << reportTypeToString(targetType)
                      << " records found above threshold (" << minScoreThreshold << " pts).\n";
        } else {
            std::cout << "\n\n";
            std::cout << "  MATCHING " << reportTypeToString(targetType) << " ITEMS:\n";
            std::cout << "\n";

            for (size_t i = 0; i < topKMatches.size(); ++i) {
                const auto& item = topKMatches[i];
                const ItemRecord* rec = item.candidate.item;

                std::cout << "\n[" << (i + 1) << "] ID: " << rec->id
                          << " | " << reportTypeToString(rec->type)
                          << " | Score: " << std::fixed << std::setprecision(1) << item.score << " / 100\n";
                std::cout << "    Name:        \"" << rec->name << "\"\n";
                std::cout << "    Category:    " << rec->category
                          << " | Color: " << (rec->color.empty() ? "(none)" : rec->color)
                          << " | Location: " << (rec->location.empty() ? "(none)" : rec->location) << "\n";
                std::cout << "    Description: \"" << rec->description << "\"\n";

                item.breakdown.printSummary();
            }
        }

        // -------------------------------------------------------------
        // INTERACTIVE RESOLUTION / REGISTRATION DECISION
        // -------------------------------------------------------------
        if (!topKMatches.empty()) {
            std::cout << "\n---------------------------------------------------------------\n";
            std::cout << "Was your matching item found in the list above?\n";
            std::cout << "  [1 to " << topKMatches.size() << "] Enter number (1-" << topKMatches.size()
                      << ") to CLAIM & REMOVE it from inventory\n";
            std::cout << "  [0]      Not in list (Save my query as a new "
                      << reportTypeToString(query.type) << " report)\n";
            std::cout << "---------------------------------------------------------------\n";
            std::cout << "Select option [0-" << topKMatches.size() << "]: ";

            std::string choiceStr;
            std::getline(std::cin, choiceStr);
            int choice = 0;
            try {
                if (!choiceStr.empty()) choice = std::stoi(choiceStr);
            } catch (...) {
                choice = 0;
            }

            if (choice >= 1 && choice <= static_cast<int>(topKMatches.size())) {
                int matchedItemId = topKMatches[choice - 1].candidate.item->id;
                claimAndResolve(matchedItemId);
                return;
            }
        }

        // If not in list or choice == 0, register the query into the database
        registerNewReport(query);
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

        executeSearchAndProcess(query, 10, 40.0);
    }

    void printStats() const {
        std::cout << "Database loaded: " << allItems.size() << " total records ("
                  << lostCount << " LOST, " << foundCount << " FOUND)\n";
    }
};

int main() {
    std::cout << "-----------------------------\n";
    std::cout << "ReFind: Lost & Found Matching\n";
    std::cout << "-----------------------------\n";

    ReFindApp app;
    if (!app.initialize()) {
        std::cerr << "[!] Error loading dataset from data/reports_1k.csv\n";
        return 1;
    }
    app.printStats();

    while (true) {
        std::cout << "\nSelect Operation : \n";
        std::cout << "  [1] Search a LOST Item  (Finds & Ranks matching Found items)\n";
        std::cout << "  [2] Search a FOUND Item (Finds & Ranks matching Lost items)\n";
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
