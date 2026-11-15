#include "../include/MaxHeap.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

int main() {
    std::cout << "====================================================\n";
    std::cout << "    ReFind: Phase 5 Custom Max-Heap Unit Test       \n";
    std::cout << "====================================================\n\n";

    // 1. Test Manual Heap Push and ExtractMax
    std::cout << "[*] 1. Testing heap push and extractMax ...\n";
    MaxHeap heap;
    std::vector<double> scores = {45.0, 72.5, 30.0, 95.0, 60.0, 88.0, 15.0};

    for (double s : scores) {
        CandidateMatch cm;
        ScoreBreakdown sb;
        sb.totalScore = s;
        heap.push(ScoredCandidate(s, cm, sb));
    }

    std::cout << "    - Pushed " << scores.size() << " elements. Heap size: " << heap.size() << "\n";
    std::cout << "    - Extracting in descending order: ";

    double prev = 1000.0;
    while (!heap.isEmpty()) {
        ScoredCandidate top = heap.extractMax();
        std::cout << top.score << " ";
        assert(top.score <= prev && "MaxHeap extractMax violation: items not descending!");
        prev = top.score;
    }
    std::cout << "\n    [+] Heap push/extractMax invariant verified successfully!\n\n";

    // 2. Test Floyd's O(m) Bottom-Up build_heap
    std::cout << "[*] 2. Testing Floyd's Linear O(m) Heap Construction ...\n";
    std::vector<ScoredCandidate> testList;
    std::vector<double> randomScores = {12.0, 85.0, 42.0, 99.5, 35.0, 77.0, 50.0, 63.0, 91.0, 20.0};
    for (double s : randomScores) {
        CandidateMatch cm;
        ScoreBreakdown sb;
        sb.totalScore = s;
        testList.emplace_back(s, cm, sb);
    }

    MaxHeap bulkHeap(testList);
    std::cout << "    - Built Max-Heap from " << bulkHeap.size() << " candidates using Floyd's algorithm.\n";
    std::cout << "    - Extracting all elements: ";

    prev = 1000.0;
    while (!bulkHeap.isEmpty()) {
        ScoredCandidate top = bulkHeap.extractMax();
        std::cout << top.score << " ";
        assert(top.score <= prev && "Floyd build_heap extraction order incorrect!");
        prev = top.score;
    }
    std::cout << "\n    [+] Floyd's O(m) construction verified successfully!\n\n";

    // 3. Test Top-K Extraction with Minimum Score Threshold
    std::cout << "[*] 3. Testing extractTopK (K = 3, threshold = 60.0) ...\n";
    std::vector<ScoredCandidate> candidates;
    for (double s : randomScores) {
        CandidateMatch cm;
        ScoreBreakdown sb;
        sb.totalScore = s;
        candidates.emplace_back(s, cm, sb);
    }

    int K = 3;
    double threshold = 60.0;
    auto topK = MaxHeap::extractTopK(candidates, K, threshold);

    std::cout << "    - Requested Top-" << K << " with score >= " << threshold << "\n";
    std::cout << "    - Returned " << topK.size() << " items:\n";
    for (size_t i = 0; i < topK.size(); ++i) {
        std::cout << "      Rank " << (i + 1) << ": Score " << topK[i].score << "\n";
        assert(topK[i].score >= threshold && "Threshold filtering failed!");
    }
    assert(topK.size() == 3 && "Expected exactly 3 items in Top-K!");
    assert(topK[0].score == 99.5 && "Top 1 score should be 99.5!");
    assert(topK[1].score == 91.0 && "Top 2 score should be 91.0!");
    assert(topK[2].score == 85.0 && "Top 3 score should be 85.0!");

    std::cout << "    [+] Top-K extraction with threshold verified successfully!\n\n";

    std::cout << "[+] ALL PHASE 5 MAX-HEAP TESTS PASSED!\n";
    return 0;
}
