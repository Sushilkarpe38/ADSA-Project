#include "../include/Scorer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void ScoreBreakdown::printSummary() const {
    std::cout << "  --------------------------------------------------\n";
    std::cout << "  SCORE BREAKDOWN (Total: "
              << std::fixed << std::setprecision(1) << totalScore << " / 100)\n";
    std::cout << "  --------------------------------------------------\n";
    std::cout << "    Category:    " << std::setw(5) << categoryScore << " / 30.0 pts\n";
    std::cout << "    Name:        " << std::setw(5) << nameScore     << " / 25.0 pts\n";
    std::cout << "    Color:       " << std::setw(5) << colorScore    << " / 15.0 pts\n";
    std::cout << "    Location:    " << std::setw(5) << locationScore << " / 15.0 pts\n";
    std::cout << "    Description: " << std::setw(5) << descriptionScore << " / 15.0 pts\n";
    std::cout << "  --------------------------------------------------\n";
}

double Scorer::calculateIDF(const std::string& word) const {
    int df = trie.getDocFreq(word);
    double numerator = static_cast<double>(totalOppositeItems + 1);
    double denominator = static_cast<double>(df + 1);
    return std::log(numerator / denominator) + 1.0;
}

double Scorer::calculateWordOverlap(
    const std::vector<std::string>& queryWords,
    const std::unordered_map<std::string, int>& matchedWordsWithDist,
    std::vector<std::string>* outDetails,
    const std::string& fieldName) const {

    if (queryWords.empty()) {
        return 0.0;
    }

    double totalIDF = 0.0;
    double gainedIDF = 0.0;

    for (const auto& w : queryWords) {
        if (w.empty()) continue;
        double idf = calculateIDF(w);
        totalIDF += idf;

        auto it = matchedWordsWithDist.find(w);
        if (it != matchedWordsWithDist.end()) {
            int dist = it->second;
            // 25% penalty per edit distance
            double discount = std::max(0.0, 1.0 - (0.25 * dist));
            double gained = idf * discount;
            gainedIDF += gained;

            if (outDetails) {
                std::stringstream ss;
                ss << fieldName << " word '" << w << "' matched (edit dist: " << dist
                   << ", IDF: " << std::fixed << std::setprecision(2) << idf
                   << ", weight: " << static_cast<int>(discount * 100) << "%)";
                outDetails->push_back(ss.str());
            }
        }
    }

    if (totalIDF <= 0.0) return 0.0;
    return std::min(1.0, gainedIDF / totalIDF);
}

ScoreBreakdown Scorer::computeScore(const ItemRecord& query, const CandidateMatch& candidate) const {
    ScoreBreakdown breakdown;
    if (!candidate.item) {
        return breakdown;
    }

    const ItemRecord& target = *(candidate.item);

    // 1. Category Score (30 pts)
    if (!query.category.empty() && query.category == target.category) {
        breakdown.categoryScore = 30.0;
        breakdown.explanations.push_back("Exact category match: '" + query.category + "' (+30.0 pts)");
    } else {
        breakdown.categoryScore = 0.0;
    }

    // 2. Name Score (25 pts max)
    double nameOverlap = calculateWordOverlap(query.name_words, candidate.matchedNameWords,
                                              &breakdown.explanations, "Name");
    breakdown.nameScore = 25.0 * nameOverlap;

    // 3. Color Score (15 pts)
    if (!query.color.empty() && !target.color.empty() && query.color == target.color) {
        breakdown.colorScore = 15.0;
        breakdown.explanations.push_back("Exact color match: '" + query.color + "' (+15.0 pts)");
    } else {
        breakdown.colorScore = 0.0;
    }

    // 4. Location Score (15 pts)
    if (!query.location.empty() && !target.location.empty() && query.location == target.location) {
        breakdown.locationScore = 15.0;
        breakdown.explanations.push_back("Exact location match: '" + query.location + "' (+15.0 pts)");
    } else {
        breakdown.locationScore = 0.0;
    }

    // 5. Description Score (15 pts max)
    double descOverlap = calculateWordOverlap(query.description_words, candidate.matchedDescWords,
                                              &breakdown.explanations, "Description");
    breakdown.descriptionScore = 15.0 * descOverlap;

    // Total Score (out of 100)
    breakdown.totalScore = breakdown.categoryScore + breakdown.nameScore +
                           breakdown.colorScore + breakdown.locationScore +
                           breakdown.descriptionScore;

    return breakdown;
}
