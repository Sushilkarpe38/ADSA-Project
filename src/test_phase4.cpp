#include "../include/ItemRecord.h"
#include "../include/Normalizer.h"
#include "../include/CategoryIndex.h"
#include "../include/Trie.h"
#include "../include/CandidateGenerator.h"
#include "../include/Scorer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>

int main() {
    std::cout << "====================================================\n";
    std::cout << "   ReFind: Phase 4 Candidate Generation & Scorer    \n";
    std::cout << "====================================================\n\n";

    // 1. Initialize Normalizer
    Normalizer normalizer;
    normalizer.loadStopWords("data/stopwords.txt");
    normalizer.loadSynonyms("data/synonyms.txt");

    // 2. Load dataset
    std::string datasetPath = "data/reports_1k.csv";
    std::cout << "[*] 1. Loading dataset from " << datasetPath << " ...\n";
    std::vector<ItemRecord> items = ItemRecord::loadFromCSV(datasetPath);
    std::cout << "    - Total records: " << items.size() << "\n";

    // 3. Prepare items and build indexes
    CategoryIndex categoryIndex;
    Trie foundTrie;
    std::unordered_map<int, const ItemRecord*> itemLookup;
    int foundCount = 0;

    for (auto& item : items) {
        normalizer.prepareItem(item);
        itemLookup[item.id] = &item;
        categoryIndex.insert(&item);

        if (item.type == ReportType::FOUND) {
            foundCount++;
            for (const auto& w : item.name_words) foundTrie.insert(w, item.id);
            for (const auto& w : item.description_words) foundTrie.insert(w, item.id);
        }
    }
    std::cout << "    - FOUND items indexed: " << foundCount << "\n";
    std::cout << "    - Vocabulary in Trie:  " << foundTrie.getWordCount() << " unique words\n\n";

    // 4. Test Candidate Generation (M4) and Scorer (M5)
    CandidateGenerator candidateGen(categoryIndex, foundTrie, itemLookup);
    Scorer scorer(foundTrie, foundCount);

    std::cout << "[*] 2. Testing Candidate Generation & Explainable Scoring on Matched Pairs...\n\n";

    // Find the first 3 valid LOST queries that have a known pair_id
    int tested = 0;
    for (const auto& query : items) {
        if (query.type != ReportType::LOST || query.pair_id == -1) continue;

        std::cout << "============================================================\n";
        std::cout << "TEST CASE #" << (tested + 1) << " [Query ID: " << query.id
                  << ", Target PairID: " << query.pair_id << "]\n";
        std::cout << "  LOST: " << query.category << " | \"" << query.name << "\" | Color: "
                  << query.color << " | Loc: " << query.location << "\n";
        std::cout << "  Desc: \"" << query.description << "\"\n";
        std::cout << "------------------------------------------------------------\n";

        // Generate Candidates (M4)
        auto candidates = candidateGen.generateCandidates(query);
        std::cout << "  [M4 Candidate Pruning] Total Stored: " << items.size()
                  << " -> Allowed Category Bucket: " << categoryIndex.getCount(query.category, ReportType::FOUND)
                  << " -> Generated Candidates (m): " << candidates.size() << "\n";

        // Verify pruning efficiency
        assert(candidates.size() < items.size() && "Pruning failed to reduce search space!");

        // Score Candidates (M5)
        std::vector<std::pair<double, const CandidateMatch*>> scoredList;
        for (const auto& pair : candidates) {
            ScoreBreakdown sb = scorer.computeScore(query, pair.second);
            scoredList.push_back({sb.totalScore, &pair.second});
        }

        // Sort descending
        std::sort(scoredList.begin(), scoredList.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        std::cout << "  Top Match Found:\n";
        if (!scoredList.empty()) {
            const auto* best = scoredList.front().second;
            ScoreBreakdown bestScore = scorer.computeScore(query, *best);
            std::cout << "    ID: " << best->item->id
                      << " | Name: \"" << best->item->name << "\""
                      << " | Color: " << best->item->color
                      << " | Loc: " << best->item->location
                      << " | PairID: " << best->item->pair_id << "\n";

            bestScore.printSummary();

            if (best->item->pair_id == query.pair_id) {
                std::cout << "  >>> [SUCCESS] Correct Ground-Truth Pair Identified!\n";
            } else {
                std::cout << "  >>> [INFO] Ground truth pair was ranked lower or had variations.\n";
            }
        } else {
            std::cout << "  [-] No candidates survived pruning.\n";
        }
        std::cout << "\n";

        tested++;
        if (tested >= 3) break;
    }

    std::cout << "[+] Phase 4 unit tests completed successfully!\n";
    return 0;
}
