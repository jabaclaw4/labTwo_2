#pragma once

#include <algorithm>
#include <vector>

#include "BinaryTree.h"
#include "CompositeKey.h"
#include "DynamicArray.h"
#include "HashTable.h"
#include "IDictionary.h"

// два индекса по одному и тому же атрибуту
// значение в индексе это позиции в исходной последовательности
template <typename TKey>
struct IndexPair {
    HashTable<TKey, DynamicArray<int>> hash;
    BinaryTree<TKey, DynamicArray<int>> tree;

    IndexPair() : hash(&HashKey<TKey>) {}
};

// построитель индекса
// работает с любой реализацией словаря и с любым типом элементов
template <typename T, typename KeyExtractor, typename TKey>
void BuildIndex(const DynamicArray<T>& source, KeyExtractor extractor,
                IDictionary<TKey, DynamicArray<int>>& index) {
    for (size_t i = 0; i < source.GetSize(); ++i) {
        TKey key = extractor(source[i]);
        if (index.ContainsKey(key)) {
            index.Get(key).PushBack(static_cast<int>(i));
        } else {
            DynamicArray<int> positions;
            positions.PushBack(static_cast<int>(i));
            index.Add(key, positions);
        }
    }
}

// поиск по ключу через индекс
template <typename TKey>
DynamicArray<int> FindByKey(const IDictionary<TKey, DynamicArray<int>>& index, const TKey& key) {
    if (!index.ContainsKey(key)) {
        return DynamicArray<int>();
    }
    return index.Get(key);
}

// диапазон через дерево
template <typename TKey>
DynamicArray<int> FindRangeTree(const BinaryTree<TKey, DynamicArray<int>>& tree, const TKey& lo, const TKey& hi) {
    DynamicArray<int> result;
    tree.ForEachInRange(lo, hi, [&result](const TKey&, const DynamicArray<int>& positions) {
        for (size_t i = 0; i < positions.GetSize(); ++i) {
            result.PushBack(positions[i]);
        }
    });
    return result;
}

// диапазон через хеш таблицу возможен только полным перебором
template <typename TKey>
DynamicArray<int> FindRangeHashScan(const HashTable<TKey, DynamicArray<int>>& table, const TKey& lo, const TKey& hi) {
    DynamicArray<int> result;
    table.ForEach([&result, &lo, &hi](const TKey& key, const DynamicArray<int>& positions) {
        if (!(key < lo) && !(hi < key)) {
            for (size_t i = 0; i < positions.GetSize(); ++i) {
                result.PushBack(positions[i]);
            }
        }
    });
    return result;
}

// поиск без индекса для сравнения
template <typename T, typename KeyExtractor, typename TKey>
DynamicArray<int> LinearFind(const DynamicArray<T>& source, KeyExtractor extractor, const TKey& key) {
    DynamicArray<int> result;
    for (size_t i = 0; i < source.GetSize(); ++i) {
        if (extractor(source[i]) == key) {
            result.PushBack(static_cast<int>(i));
        }
    }
    return result;
}

template <typename T, typename KeyExtractor, typename TKey>
DynamicArray<int> LinearFindRange(const DynamicArray<T>& source, KeyExtractor extractor, const TKey& lo,
                                  const TKey& hi) {
    DynamicArray<int> result;
    for (size_t i = 0; i < source.GetSize(); ++i) {
        TKey key = extractor(source[i]);
        if (!(key < lo) && !(hi < key)) {
            result.PushBack(static_cast<int>(i));
        }
    }
    return result;
}

// сравнение результатов без учета порядка
inline DynamicArray<int> SortedCopy(const DynamicArray<int>& source) {
    std::vector<int> tmp;
    for (size_t i = 0; i < source.GetSize(); ++i) {
        tmp.push_back(source[i]);
    }
    std::sort(tmp.begin(), tmp.end());
    DynamicArray<int> result;
    for (size_t i = 0; i < tmp.size(); ++i) {
        result.PushBack(tmp[i]);
    }
    return result;
}

inline bool SameContent(const DynamicArray<int>& a, const DynamicArray<int>& b) {
    if (a.GetSize() != b.GetSize()) {
        return false;
    }
    DynamicArray<int> sortedA = SortedCopy(a);
    DynamicArray<int> sortedB = SortedCopy(b);
    for (size_t i = 0; i < sortedA.GetSize(); ++i) {
        if (sortedA[i] != sortedB[i]) {
            return false;
        }
    }
    return true;
}