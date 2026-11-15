#include "../include/CategoryIndex.h"
#include <algorithm>

static const std::vector<const ItemRecord*> EMPTY_RECORD_LIST;

CategoryIndex::CategoryIndex() {}

void CategoryIndex::insert(const ItemRecord* item) {
    if (!item) return;

    if (item->type == ReportType::LOST) {
        lostByCategory[item->category].push_back(item);
    } else if (item->type == ReportType::FOUND) {
        foundByCategory[item->category].push_back(item);
    }
}

void CategoryIndex::remove(int itemId, const std::string& category, ReportType type) {
    auto predicate = [itemId](const ItemRecord* itm) {
        return itm && itm->id == itemId;
    };

    if (type == ReportType::LOST) {
        auto it = lostByCategory.find(category);
        if (it != lostByCategory.end()) {
            it->second.erase(std::remove_if(it->second.begin(), it->second.end(), predicate), it->second.end());
            if (it->second.empty()) {
                lostByCategory.erase(it);
            }
        }
    } else if (type == ReportType::FOUND) {
        auto it = foundByCategory.find(category);
        if (it != foundByCategory.end()) {
            it->second.erase(std::remove_if(it->second.begin(), it->second.end(), predicate), it->second.end());
            if (it->second.empty()) {
                foundByCategory.erase(it);
            }
        }
    }
}

const std::vector<const ItemRecord*>& CategoryIndex::lookup(const std::string& category, ReportType type) const {
    if (type == ReportType::LOST) {
        auto it = lostByCategory.find(category);
        if (it != lostByCategory.end()) {
            return it->second;
        }
    } else if (type == ReportType::FOUND) {
        auto it = foundByCategory.find(category);
        if (it != foundByCategory.end()) {
            return it->second;
        }
    }
    return EMPTY_RECORD_LIST;
}

size_t CategoryIndex::getCount(const std::string& category, ReportType type) const {
    return lookup(category, type).size();
}

void CategoryIndex::clear() {
    lostByCategory.clear();
    foundByCategory.clear();
}
