#ifndef CANDIDATE_GENERATOR_H
#define CANDIDATE_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "ItemRecord.h"
#include "CategoryIndex.h"
#include "Trie.h"

// Stores candidate details and which query words matched with what edit distance
struct CandidateMatch {
    const ItemRecord* item;
    // Map of query word -> best edit distance (0 for exact, 1 or 2 for typo)
    std::unordered_map<std::string, int> matchedNameWords;
    std::unordered_map<std::string, int> matchedDescWords;
    std::unordered_map<std::string, int> matchedColorWords;
    std::unordered_map<std::string, int> matchedLocationWords;

    CandidateMatch() : item(nullptr) {}
    explicit CandidateMatch(const ItemRecord* itm) : item(itm) {}
};

class CandidateGenerator {
private:
    const CategoryIndex& categoryIndex;
    const Trie& trie;
    const std::unordered_map<int, const ItemRecord*>& itemLookup;

public:
    CandidateGenerator(const CategoryIndex& catIdx, const Trie& tr,
                       const std::unordered_map<int, const ItemRecord*>& lookup)
        : categoryIndex(catIdx), trie(tr), itemLookup(lookup) {}

    // Executes 3-level pruning pipeline:
    // 1. Category bucket filter (M2)
    // 2. Exact and typo-tolerant Trie search (M3)
    // 3. Fallback color/location match
    // Returns map of itemId -> CandidateMatch (m candidates where m << N)
    std::unordered_map<int, CandidateMatch> generateCandidates(const ItemRecord& query) const;
};

#endif // CANDIDATE_GENERATOR_H
