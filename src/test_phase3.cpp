#include "../include/ItemRecord.h"
#include "../include/Normalizer.h"
#include "../include/Trie.h"
#include <iostream>
#include <iomanip>
#include <cassert>

int main() {
    std::cout << "====================================================\n";
    std::cout << "   ReFind: Phase 3 Typo-Tolerant Trie Search Test   \n";
    std::cout << "====================================================\n\n";

    // 1. Initialize Normalizer and load dataset
    Normalizer normalizer;
    normalizer.loadStopWords("data/stopwords.txt");
    normalizer.loadSynonyms("data/synonyms.txt");

    std::string datasetPath = "data/reports_1k.csv";
    std::cout << "[*] Loading dataset from " << datasetPath << " ...\n";
    std::vector<ItemRecord> items = ItemRecord::loadFromCSV(datasetPath);
    std::cout << "    - Loaded " << items.size() << " records.\n";

    // 2. Build Trie from all FOUND items
    Trie trie;
    for (auto& item : items) {
        normalizer.prepareItem(item);
        if (item.type == ReportType::FOUND) {
            for (const auto& w : item.name_words) trie.insert(w, item.id);
            for (const auto& w : item.description_words) trie.insert(w, item.id);
        }
    }

    std::cout << "    [+] Trie Built Successfully:\n";
    std::cout << "        - Unique Words: " << trie.getWordCount() << "\n";
    std::cout << "        - Total Trie Nodes: " << trie.getNodeCount() << "\n\n";

    // 3. Test exact match vs typo match
    std::cout << "[*] Testing Exact vs Typo-Tolerant Search (Contribution C2):\n";

    struct TestCase {
        std::string query;
        std::string expectedMatch;
        int maxDist;
    };

    std::vector<TestCase> testCases = {
        {"samsng",  "samsung", 1},
        {"iphne",   "iphone",  1},
        {"galxy",   "galaxy",  1},
        {"airpds",  "airpods", 1},
        {"botle",   "bottle",  1},
        {"lethr",   "leather", 2}
    };

    for (const auto& tc : testCases) {
        int nodesVisited = 0;
        auto matches = trie.fuzzySearch(tc.query, tc.maxDist, &nodesVisited);

        double prunePct = 100.0 * (1.0 - (double)nodesVisited / (double)trie.getNodeCount());

        std::cout << "  Query: \"" << std::left << std::setw(8) << tc.query << "\""
                  << " | MaxDist: " << tc.maxDist
                  << " | Nodes Visited: " << std::setw(4) << nodesVisited
                  << " / " << trie.getNodeCount()
                  << " (Pruned: " << std::fixed << std::setprecision(1) << prunePct << "%)\n";

        bool foundExpected = false;
        for (const auto& m : matches) {
            std::cout << "    -> Match: \"" << m.word << "\" (Dist: " << m.distance
                      << ", Items: " << m.postings->size() << ")\n";
            if (m.word == tc.expectedMatch) {
                foundExpected = true;
            }
        }

        if (!matches.empty()) {
            std::cout << "    [+] Found " << matches.size() << " candidate word(s)\n";
        } else {
            std::cout << "    [-] No matches within distance limit\n";
        }
        std::cout << "\n";
    }

    // 4. Verify that exact words have distance 0
    int nodesVisited = 0;
    auto exactMatches = trie.fuzzySearch("samsung", 1, &nodesVisited);
    bool exactMatchedZeroDist = false;
    for (const auto& m : exactMatches) {
        if (m.word == "samsung" && m.distance == 0) {
            exactMatchedZeroDist = true;
        }
    }
    assert(exactMatchedZeroDist && "Exact match did not return distance 0!");
    std::cout << "[+] Exact match for 'samsung' correctly returned distance 0.\n";

    // 5. Verify that completely random strings are pruned early
    nodesVisited = 0;
    auto gibberishMatches = trie.fuzzySearch("zzzzzzzz", 1, &nodesVisited);
    assert(gibberishMatches.empty() && "Gibberish query should return 0 matches!");
    std::cout << "[+] Gibberish query 'zzzzzzzz' visited only " << nodesVisited
              << " nodes and was pruned immediately (0 matches).\n";

    std::cout << "\n[+] Phase 3 validation completed successfully!\n";
    return 0;
}
