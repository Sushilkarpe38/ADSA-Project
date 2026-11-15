#include "../include/Trie.h"
#include <algorithm>
#include <iostream>

static const std::vector<int> EMPTY_POSTINGS;

Trie::Trie() : root(new TrieNode()), uniqueWordCount(0), totalNodeCount(1) {}

Trie::~Trie() {
    delete root;
    root = nullptr;
}

void Trie::insert(const std::string& word, int itemId) {
    if (word.empty()) return;

    TrieNode* current = root;
    for (char c : word) {
        if (current->children.find(c) == current->children.end()) {
            current->children[c] = new TrieNode();
            totalNodeCount++;
        }
        current = current->children[c];
    }

    if (!current->isEnd) {
        current->isEnd = true;
        uniqueWordCount++;
    }

    // Add itemId to postings if not already the last added item
    if (current->postings.empty() || current->postings.back() != itemId) {
        current->postings.push_back(itemId);
    }
}

bool Trie::removeHelper(TrieNode* current, const std::string& word, size_t depth, int itemId, bool& removed) {
    if (!current) return false;

    if (depth == word.length()) {
        if (current->isEnd) {
            auto it = std::find(current->postings.begin(), current->postings.end(), itemId);
            if (it != current->postings.end()) {
                current->postings.erase(it);
                removed = true;
            }
            if (current->postings.empty()) {
                current->isEnd = false;
                if (uniqueWordCount > 0) uniqueWordCount--;
            }
        }
        // Node can be pruned if it has no children and is not an end of another word
        return current->children.empty() && !current->isEnd;
    }

    char c = word[depth];
    auto childIt = current->children.find(c);
    if (childIt == current->children.end()) {
        return false;
    }

    bool shouldDeleteChild = removeHelper(childIt->second, word, depth + 1, itemId, removed);

    if (shouldDeleteChild) {
        delete childIt->second;
        current->children.erase(childIt);
        if (totalNodeCount > 1) totalNodeCount--;
        return current->children.empty() && !current->isEnd;
    }

    return false;
}

void Trie::remove(const std::string& word, int itemId) {
    if (word.empty()) return;
    bool removed = false;
    removeHelper(root, word, 0, itemId, removed);
}

TrieNode* Trie::searchExact(const std::string& word) const {
    if (word.empty()) return nullptr;

    TrieNode* current = root;
    for (char c : word) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            return nullptr;
        }
        current = it->second;
    }

    return current->isEnd ? current : nullptr;
}

int Trie::getDocFreq(const std::string& word) const {
    TrieNode* node = searchExact(word);
    return node ? node->getDocFreq() : 0;
}

const std::vector<int>& Trie::getPostings(const std::string& word) const {
    TrieNode* node = searchExact(word);
    return node ? node->postings : EMPTY_POSTINGS;
}

void Trie::fuzzyWalk(TrieNode* node, char letter, const std::string& query,
                     const std::vector<int>& prevRow, std::string& currentPrefix,
                     int maxDist, std::vector<FuzzyMatch>& results,
                     int& nodesVisited) const {
    nodesVisited++;
    int n = static_cast<int>(query.length());
    std::vector<int> currRow(n + 1);

    // 1. Make a new row. First number is first of row above + 1
    currRow[0] = prevRow[0] + 1;
    int minVal = currRow[0];

    // 2. For each position in query word
    for (int j = 1; j <= n; ++j) {
        int insertCost  = currRow[j - 1] + 1;                       // Insertion (left + 1)
        int deleteCost  = prevRow[j] + 1;                           // Deletion (above + 1)
        int matchCost   = (query[j - 1] == letter) ? 0 : 1;
        int replaceCost = prevRow[j - 1] + matchCost;              // Substitution (diagonal + cost)

        int val = std::min({insertCost, deleteCost, replaceCost});
        currRow[j] = val;
        if (val < minVal) {
            minVal = val;
        }
    }

    // 3. If last number of row <= maxDist and node is end of word, record match
    if (currRow[n] <= maxDist && node->isEnd) {
        results.emplace_back(currentPrefix, currRow[n], &node->postings);
    }

    // 4. Row Pruning: If minVal <= maxDist, branch could still lead to a match; otherwise SKIP whole subtree
    if (minVal <= maxDist) {
        for (const auto& pair : node->children) {
            char ch = pair.first;
            TrieNode* child = pair.second;
            currentPrefix.push_back(ch);
            fuzzyWalk(child, ch, query, currRow, currentPrefix, maxDist, results, nodesVisited);
            currentPrefix.pop_back();
        }
    }
}

std::vector<FuzzyMatch> Trie::fuzzySearch(const std::string& word, int maxDist,
                                         int* outNodesVisited) const {
    std::vector<FuzzyMatch> results;
    if (word.empty()) return results;

    // Adaptive max distance based on word length
    if (maxDist < 0) {
        maxDist = (word.length() <= 5) ? 1 : 2;
    }

    int n = static_cast<int>(word.length());
    std::vector<int> initialRow(n + 1);
    for (int i = 0; i <= n; ++i) {
        initialRow[i] = i;
    }

    int nodesVisited = 1; // Count root
    std::string currentPrefix;

    for (const auto& pair : root->children) {
        char ch = pair.first;
        TrieNode* child = pair.second;
        currentPrefix.push_back(ch);
        fuzzyWalk(child, ch, word, initialRow, currentPrefix, maxDist, results, nodesVisited);
        currentPrefix.pop_back();
    }

    if (outNodesVisited) {
        *outNodesVisited = nodesVisited;
    }

    // Sort matches by distance ascending (closest match first)
    std::sort(results.begin(), results.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
        if (a.distance != b.distance) return a.distance < b.distance;
        return a.word < b.word;
    });

    return results;
}

void Trie::clear() {
    delete root;
    root = new TrieNode();
    uniqueWordCount = 0;
    totalNodeCount = 1;
}
