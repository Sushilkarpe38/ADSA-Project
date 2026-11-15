#include "../include/Trie.h"
#include "../include/CategoryIndex.h"
#include "../include/ItemRecord.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "====================================================\n";
    std::cout << "     Testing Trie & CategoryIndex Deletion          \n";
    std::cout << "====================================================\n\n";

    // 1. Test Trie Deletion
    std::cout << "[*] 1. Testing Trie Postings and Node Deletion ...\n";
    Trie trie;
    trie.insert("samsung", 101);
    trie.insert("samsung", 102);
    trie.insert("sam", 103);

    std::cout << "    - Initial doc_freq for 'samsung': " << trie.getDocFreq("samsung") << " (Items: 101, 102)\n";
    std::cout << "    - Initial doc_freq for 'sam': " << trie.getDocFreq("sam") << " (Item: 103)\n";
    std::cout << "    - Total Trie unique words: " << trie.getWordCount() << "\n";
    assert(trie.getDocFreq("samsung") == 2);

    // Remove item 101
    trie.remove("samsung", 101);
    std::cout << "    - Removed item 101 from 'samsung'. New doc_freq: " << trie.getDocFreq("samsung") << "\n";
    assert(trie.getDocFreq("samsung") == 1);
    const auto& postings = trie.getPostings("samsung");
    assert(postings.size() == 1 && postings[0] == 102);

    // Remove item 102 (last item containing 'samsung')
    trie.remove("samsung", 102);
    std::cout << "    - Removed item 102 from 'samsung'. New doc_freq: " << trie.getDocFreq("samsung") << "\n";
    assert(trie.getDocFreq("samsung") == 0);
    assert(trie.searchExact("samsung") == nullptr && "'samsung' should be completely pruned/removed from Trie!");
    // But 'sam' should still exist!
    assert(trie.getDocFreq("sam") == 1 && "'sam' prefix should remain intact!");
    std::cout << "    [+] Trie deletion and node pruning verified successfully!\n\n";

    // 2. Test CategoryIndex Deletion
    std::cout << "[*] 2. Testing CategoryIndex Deletion ...\n";
    CategoryIndex catIndex;
    ItemRecord item1(201, ReportType::FOUND, "phone", "Galaxy", "black", "Library", "desc");
    ItemRecord item2(202, ReportType::FOUND, "phone", "iPhone", "silver", "Canteen", "desc");
    catIndex.insert(&item1);
    catIndex.insert(&item2);

    std::cout << "    - Initial FOUND phones count: " << catIndex.getCount("phone", ReportType::FOUND) << "\n";
    assert(catIndex.getCount("phone", ReportType::FOUND) == 2);

    catIndex.remove(201, "phone", ReportType::FOUND);
    std::cout << "    - Removed item 201. New count: " << catIndex.getCount("phone", ReportType::FOUND) << "\n";
    assert(catIndex.getCount("phone", ReportType::FOUND) == 1);

    const auto& bucket = catIndex.lookup("phone", ReportType::FOUND);
    assert(bucket.size() == 1 && bucket[0]->id == 202);

    catIndex.remove(202, "phone", ReportType::FOUND);
    std::cout << "    - Removed item 202. New count: " << catIndex.getCount("phone", ReportType::FOUND) << "\n";
    assert(catIndex.getCount("phone", ReportType::FOUND) == 0);
    std::cout << "    [+] CategoryIndex deletion verified successfully!\n\n";

    std::cout << "[+] ALL TRIE & HASHTABLE DELETION TESTS PASSED!\n";
    return 0;
}
