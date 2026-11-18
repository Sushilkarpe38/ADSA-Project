#include "../include/CandidateGenerator.h"
#include <algorithm>

std::unordered_map<int, CandidateMatch>
CandidateGenerator::generateCandidates(const ItemRecord& query) const {
    std::unordered_map<int, CandidateMatch> candidates;

    // 1. Determine target report type (opposite of query)
    ReportType targetType = (query.type == ReportType::LOST) ? ReportType::FOUND : ReportType::LOST;

    // 2. Step 1 Pruning: Retrieve items belonging strictly to query's category
    const auto& categoryBucket = categoryIndex.lookup(query.category, targetType);
    if (categoryBucket.empty()) {
        return candidates;
    }

    // Build fast membership set of allowed item IDs
    std::unordered_set<int> allowedIds;
    for (const auto* itm : categoryBucket) {
        if (itm) {
            allowedIds.insert(itm->id);
        }
    }

    // Helper lambda: match words from a query field in Trie and record best edit distance
    // field: 0=name, 1=desc, 2=color, 3=location
    auto processWords = [&](const std::vector<std::string>& words, int field) {
        for (const auto& w : words) {
            if (w.empty()) continue;

            auto recordMatch = [&](int itemId, int dist) {
                if (allowedIds.find(itemId) == allowedIds.end()) return;
                auto& cand = candidates[itemId];
                if (!cand.item) {
                    auto it = itemLookup.find(itemId);
                    if (it != itemLookup.end()) cand.item = it->second;
                }
                auto updateMap = [&](std::unordered_map<std::string, int>& m) {
                    auto it = m.find(w);
                    if (it == m.end() || it->second > dist) m[w] = dist;
                };
                if (field == 0) updateMap(cand.matchedNameWords);
                else if (field == 1) updateMap(cand.matchedDescWords);
                else if (field == 2) updateMap(cand.matchedColorWords);
                else                 updateMap(cand.matchedLocationWords);
            };

            // Try exact search first
            auto exactNode = trie.searchExact(w);
            if (exactNode && !exactNode->postings.empty()) {
                for (int itemId : exactNode->postings) recordMatch(itemId, 0);
            } else {
                // Bounded typo-tolerant search
                auto fuzzyMatches = trie.fuzzySearch(w);
                for (const auto& fm : fuzzyMatches) {
                    if (!fm.postings) continue;
                    for (int itemId : *(fm.postings)) recordMatch(itemId, fm.distance);
                }
            }
        }
    };

    // 3. Step 2 Pruning: Process name, description, color and location words through Trie
    processWords(query.name_words,     0);
    processWords(query.description_words, 1);
    processWords(query.color_words,    2);
    processWords(query.location_words, 3);

    return candidates;
}
