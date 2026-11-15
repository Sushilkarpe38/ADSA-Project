#ifndef CATEGORY_INDEX_H
#define CATEGORY_INDEX_H

#include <string>
#include <vector>
#include <unordered_map>
#include "ItemRecord.h"

class CategoryIndex {
private:
    // Hash maps for fast O(1) average lookup by category
    std::unordered_map<std::string, std::vector<const ItemRecord*>> lostByCategory;
    std::unordered_map<std::string, std::vector<const ItemRecord*>> foundByCategory;

public:
    CategoryIndex();

    // Insert an item into the appropriate category bucket
    void insert(const ItemRecord* item);

    // Remove an item from the category bucket (when claimed / returned)
    void remove(int itemId, const std::string& category, ReportType type);

    // Retrieve all items of a given category and opposite type
    const std::vector<const ItemRecord*>& lookup(const std::string& category, ReportType type) const;

    // Get count of items in a category for a given type
    size_t getCount(const std::string& category, ReportType type) const;

    void clear();
};

#endif // CATEGORY_INDEX_H
