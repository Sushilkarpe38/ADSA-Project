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

    // Helper lambda to match words in Trie and update candidates
    auto processWords = [&](const std::vector<std::string>& words, bool isNameWord) {
        for (const auto& w : words) {
            if (w.empty()) continue;

            // Try exact search first
            auto exactNode = trie.searchExact(w);
            if (exactNode && !exactNode->postings.empty()) {
                for (int itemId : exactNode->postings) {
                    if (allowedIds.find(itemId) != allowedIds.end()) {
                        auto& cand = candidates[itemId];
                        if (!cand.item) {
                            auto it = itemLookup.find(itemId);
                            if (it != itemLookup.end()) cand.item = it->second;
                        }
                        if (isNameWord) {
                            auto it = cand.matchedNameWords.find(w);
                            if (it == cand.matchedNameWords.end() || it->second > 0) {
                                cand.matchedNameWords[w] = 0;
                            }
                        } else {
                            auto it = cand.matchedDescWords.find(w);
                            if (it == cand.matchedDescWords.end() || it->second > 0) {
                                cand.matchedDescWords[w] = 0;
                            }
                        }
                    }
                }
            } else {
                // If exact lookup fails, run bounded typo-tolerant search
                auto fuzzyMatches = trie.fuzzySearch(w);
                for (const auto& fm : fuzzyMatches) {
                    if (!fm.postings) continue;
                    for (int itemId : *(fm.postings)) {
                        if (allowedIds.find(itemId) != allowedIds.end()) {
                            auto& cand = candidates[itemId];
                            if (!cand.item) {
                                auto it = itemLookup.find(itemId);
                                if (it != itemLookup.end()) cand.item = it->second;
                            }
                            if (isNameWord) {
                                auto it = cand.matchedNameWords.find(w);
                                if (it == cand.matchedNameWords.end() || it->second > fm.distance) {
                                    cand.matchedNameWords[w] = fm.distance;
                                }
                            } else {
                                auto it = cand.matchedDescWords.find(w);
                                if (it == cand.matchedDescWords.end() || it->second > fm.distance) {
                                    cand.matchedDescWords[w] = fm.distance;
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    // 3. Step 2 Pruning: Process name words and description words
    processWords(query.name_words, true);
    processWords(query.description_words, false);

    // 4. Step 3 Fallback: Add allowed category items that have matching color or location
    // (Ensures items with short descriptions or pure color/location matches aren't missed)
    for (const auto* itm : categoryBucket) {
        if (!itm) continue;
        bool colorMatch = (!query.color.empty() && !itm->color.empty() && query.color == itm->color);
        bool locationMatch = (!query.location.empty() && !itm->location.empty() && query.location == itm->location);

        if (colorMatch || locationMatch) {
            if (candidates.find(itm->id) == candidates.end()) {
                candidates[itm->id] = CandidateMatch(itm);
            }
        }
    }

    return candidates;
}
