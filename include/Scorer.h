#ifndef SCORER_H
#define SCORER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include "ItemRecord.h"
#include "CandidateGenerator.h"
#include "Trie.h"

// Detailed explainability container showing point attribution
struct ScoreBreakdown {
    double categoryScore;     // Max 30
    double nameScore;         // Max 25
    double colorScore;        // Max 15
    double locationScore;     // Max 15
    double descriptionScore;  // Max 15
    double totalScore;        // Max 100

    std::vector<std::string> explanations;

    ScoreBreakdown()
        : categoryScore(0.0), nameScore(0.0), colorScore(0.0),
          locationScore(0.0), descriptionScore(0.0), totalScore(0.0) {}

    void printSummary() const;
};

class Scorer {
private:
    const Trie& trie;
    int totalOppositeItems;

    // Computes Inverse Document Frequency (IDF) using postings count from Trie
    // IDF(w) = ln((N + 1) / (doc_freq + 1)) + 1.0
    double calculateIDF(const std::string& word) const;

    // Computes IDF-weighted word overlap with typo discount penalty (25% per edit distance)
    double calculateWordOverlap(
        const std::vector<std::string>& queryWords,
        const std::unordered_map<std::string, int>& matchedWordsWithDist,
        std::vector<std::string>* outDetails = nullptr,
        const std::string& fieldName = "") const;

public:
    Scorer(const Trie& tr, int totalOpposite)
        : trie(tr), totalOppositeItems(std::max(1, totalOpposite)) {}

    void setTotalOppositeItems(int total) {
        totalOppositeItems = std::max(1, total);
    }

    // Scores candidate against query out of 100 points
    ScoreBreakdown computeScore(const ItemRecord& query, const CandidateMatch& candidate) const;
};

#endif // SCORER_H
