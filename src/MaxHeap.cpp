#include "../include/MaxHeap.h"
#include <algorithm>

MaxHeap::MaxHeap() {}

// Floyd's Linear O(m) Bottom-Up Heap Construction
MaxHeap::MaxHeap(std::vector<ScoredCandidate>& elements) {
    heap = std::move(elements);
    if (heap.size() <= 1) return;

    // Start from last parent node down to the root
    int startIdx = static_cast<int>(heap.size() / 2) - 1;
    for (int i = startIdx; i >= 0; --i) {
        siftDown(static_cast<size_t>(i));
    }
}

void MaxHeap::siftUp(size_t index) {
    while (index > 0) {
        size_t parentIdx = (index - 1) / 2;
        // In Max-Heap: child should be smaller than or equal to parent
        if (heap[index].score > heap[parentIdx].score) {
            std::swap(heap[index], heap[parentIdx]);
            index = parentIdx;
        } else {
            break;
        }
    }
}

void MaxHeap::siftDown(size_t index) {
    size_t n = heap.size();
    while (true) {
        size_t leftChild = 2 * index + 1;
        size_t rightChild = 2 * index + 2;
        size_t largest = index;

        if (leftChild < n && heap[leftChild].score > heap[largest].score) {
            largest = leftChild;
        }
        if (rightChild < n && heap[rightChild].score > heap[largest].score) {
            largest = rightChild;
        }

        if (largest != index) {
            std::swap(heap[index], heap[largest]);
            index = largest;
        } else {
            break;
        }
    }
}

void MaxHeap::push(const ScoredCandidate& item) {
    heap.push_back(item);
    siftUp(heap.size() - 1);
}

ScoredCandidate MaxHeap::extractMax() {
    if (heap.empty()) {
        throw std::runtime_error("Heap underflow: attempted to extract from empty MaxHeap");
    }

    ScoredCandidate root = heap.front();
    heap.front() = heap.back();
    heap.pop_back();

    if (!heap.empty()) {
        siftDown(0);
    }

    return root;
}

const ScoredCandidate& MaxHeap::peekMax() const {
    if (heap.empty()) {
        throw std::runtime_error("Heap is empty");
    }
    return heap.front();
}

std::vector<ScoredCandidate> MaxHeap::extractTopK(
    std::vector<ScoredCandidate>& elements,
    int K,
    double minScoreThreshold) {

    std::vector<ScoredCandidate> topK;
    if (elements.empty() || K <= 0) return topK;

    // Filter elements below the score threshold first
    std::vector<ScoredCandidate> qualified;
    for (auto& item : elements) {
        if (item.score >= minScoreThreshold) {
            qualified.push_back(item);
        }
    }

    if (qualified.empty()) return topK;

    // Build Max-Heap in O(m) time using Floyd's algorithm
    MaxHeap maxHeap(qualified);

    // Extract up to K elements in O(K log m) time
    int count = std::min(K, static_cast<int>(maxHeap.size()));
    topK.reserve(count);

    for (int i = 0; i < count; ++i) {
        topK.push_back(maxHeap.extractMax());
    }

    return topK;
}
