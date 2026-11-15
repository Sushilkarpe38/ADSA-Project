#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <vector>
#include <unordered_map>

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool isEnd;
    std::vector<int> postings; // Item IDs containing this word

    TrieNode() : isEnd(false) {}

    ~TrieNode() {
        for (auto& pair : children) {
            delete pair.second;
        }
        children.clear();
    }

    int getDocFreq() const {
        return static_cast<int>(postings.size());
    }
};

struct FuzzyMatch {
    std::string word;
    int distance;
    const std::vector<int>* postings;

    FuzzyMatch(const std::string& w, int d, const std::vector<int>* p)
        : word(w), distance(d), postings(p) {}
};

class Trie {
private:
    TrieNode* root;
    size_t uniqueWordCount;
    size_t totalNodeCount;

    // Helper for recursive DFS edit-distance matrix row walk with pruning
    void fuzzyWalk(TrieNode* node, char letter, const std::string& query,
                   const std::vector<int>& prevRow, std::string& currentPrefix,
                   int maxDist, std::vector<FuzzyMatch>& results,
                   int& nodesVisited) const;

public:
    Trie();
    ~Trie();

    // Insert word and record item ID in its postings list
    void insert(const std::string& word, int itemId);

    // Exact word search returning the node (or nullptr if not found)
    TrieNode* searchExact(const std::string& word) const;

    // Get document frequency for IDF calculations
    int getDocFreq(const std::string& word) const;

    // Get the postings list of item IDs
    const std::vector<int>& getPostings(const std::string& word) const;

    // Typo-Tolerant Trie Search using Bounded Levenshtein Edit Distance with Subtree Pruning
    // maxDist = -1 uses adaptive bound (1 for <= 5 letters, 2 for longer words)
    std::vector<FuzzyMatch> fuzzySearch(const std::string& word, int maxDist = -1,
                                       int* outNodesVisited = nullptr) const;

    // Total unique words and nodes indexed
    size_t getWordCount() const { return uniqueWordCount; }
    size_t getNodeCount() const { return totalNodeCount; }

    TrieNode* getRoot() const { return root; }

    void clear();
};

#endif // TRIE_H
