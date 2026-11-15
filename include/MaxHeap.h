#ifndef MAX_HEAP_H
#define MAX_HEAP_H

#include <vector>
#include <stdexcept>
#include "CandidateGenerator.h"
#include "Scorer.h"

// Entry stored in the Max-Heap
struct ScoredCandidate {
    double score;
    CandidateMatch candidate;
    ScoreBreakdown breakdown;

    ScoredCandidate() : score(0.0) {}
    ScoredCandidate(double s, const CandidateMatch& c, const ScoreBreakdown& b)
        : score(s), candidate(c), breakdown(b) {}
};

// Custom Array-Based Binary Max-Heap (Contribution C4)
// - Array indexing: left = 2i + 1, right = 2i + 2, parent = (i - 1) / 2
// - Floyd's bottom-up build_heap in linear O(m) time
// - Top-K extraction in O(K log m) time
class MaxHeap {
private:
    std::vector<ScoredCandidate> heap;

    void siftDown(size_t index);
    void siftUp(size_t index);

public:
    MaxHeap();

    // Floyd's O(m) Bottom-Up Heap Construction
    explicit MaxHeap(std::vector<ScoredCandidate>& elements);

    void push(const ScoredCandidate& item);
    ScoredCandidate extractMax();
    const ScoredCandidate& peekMax() const;

    bool isEmpty() const { return heap.empty(); }
    size_t size() const { return heap.size(); }
    void clear() { heap.clear(); }

    // Helper: Extracts Top-K candidates above a minimum score threshold
    // Total Time Complexity: O(m + K log m)
    static std::vector<ScoredCandidate> extractTopK(
        std::vector<ScoredCandidate>& elements,
        int K,
        double minScoreThreshold = 0.0);
};

#endif // MAX_HEAP_H
