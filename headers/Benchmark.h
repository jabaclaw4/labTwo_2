#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <utility>

#include "BinaryTree.h"
#include "DynamicArray.h"
#include "HashTable.h"
#include "Index.h"
#include "Timer.h"

enum class KeyOrder { Random, Sorted, Reversed };

inline const char* OrderName(KeyOrder order) {
    switch (order) {
        case KeyOrder::Random: return "random";
        case KeyOrder::Sorted: return "sorted";
        case KeyOrder::Reversed: return "reversed";
    }
    return "";
}

// уникальные ключи от 0 до n минус 1 в нужном порядке
inline DynamicArray<int> MakeKeys(size_t count, KeyOrder order, unsigned seed = 42) {
    DynamicArray<int> keys;
    for (size_t i = 0; i < count; ++i) {
        int value = (order == KeyOrder::Reversed) ? static_cast<int>(count - 1 - i) : static_cast<int>(i);
        keys.PushBack(value);
    }
    if (order == KeyOrder::Random) {
        std::mt19937 rng(seed);
        for (size_t i = count; i > 1; --i) {
            size_t j = rng() % i;
            std::swap(keys[i - 1], keys[j]);
        }
    }
    return keys;
}

inline int IdentityKey(const int& value) {
    return value;
}

// без балансировки дерево на упорядоченных данных работает за квадрат
// поэтому для них размер ограничен
const size_t kTreeOrderedLimit = 50000;

inline int RepeatsFor(size_t n) {
    return n <= 100000 ? 5 : 1;
}

// сравнение времени построения индекса
// в замер входит освобождение памяти структуры
inline void RunBuildBenchmark(const DynamicArray<size_t>& sizes, const std::string& csvPath) {
    std::ofstream csv(csvPath);
    csv << "n,order,hash_ms,tree_ms\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::setw(10) << "n" << std::setw(11) << "порядок" << std::setw(14) << "хеш мс"
              << std::setw(14) << "дерево мс" << "\n";

    KeyOrder orders[] = {KeyOrder::Random, KeyOrder::Sorted, KeyOrder::Reversed};
    for (size_t s = 0; s < sizes.GetSize(); ++s) {
        size_t n = sizes[s];
        for (KeyOrder order : orders) {
            DynamicArray<int> keys = MakeKeys(n, order);
            int repeats = RepeatsFor(n);

            double hashMs = MeasureMedianMs(
                    [&keys]() {
                        HashTable<int, DynamicArray<int>> table(&HashKey<int>);
                        BuildIndex(keys, IdentityKey, table);
                    },
                    repeats);

            bool skipTree = (order != KeyOrder::Random) && (n > kTreeOrderedLimit);
            double treeMs = 0;
            if (!skipTree) {
                treeMs = MeasureMedianMs(
                        [&keys]() {
                            BinaryTree<int, DynamicArray<int>> tree;
                            BuildIndex(keys, IdentityKey, tree);
                        },
                        repeats);
            }

            csv << n << "," << OrderName(order) << "," << hashMs << ",";
            if (!skipTree) {
                csv << treeMs;
            }
            csv << "\n";

            std::cout << std::setw(10) << n << std::setw(11) << OrderName(order) << std::setw(14) << hashMs;
            if (skipTree) {
                std::cout << std::setw(14) << "пропущено";
            } else {
                std::cout << std::setw(14) << treeMs;
            }
            std::cout << "\n";
        }
    }
    std::cout << "результаты записаны в " << csvPath << "\n";
}

// сравнение точного поиска в микросекундах на один запрос
inline void RunSearchBenchmark(const DynamicArray<size_t>& sizes, const std::string& csvPath) {
    std::ofstream csv(csvPath);
    csv << "n,hash_us,tree_us,linear_us\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::setw(10) << "n" << std::setw(14) << "хеш мкс" << std::setw(14) << "дерево мкс"
              << std::setw(16) << "линейный мкс" << "\n";

    const size_t queries = 1000;
    const size_t linearQueries = 20;
    size_t checksum = 0;

    for (size_t s = 0; s < sizes.GetSize(); ++s) {
        size_t n = sizes[s];
        DynamicArray<int> keys = MakeKeys(n, KeyOrder::Random);
        HashTable<int, DynamicArray<int>> table(&HashKey<int>);
        BinaryTree<int, DynamicArray<int>> tree;
        BuildIndex(keys, IdentityKey, table);
        BuildIndex(keys, IdentityKey, tree);

        std::mt19937 rng(7);
        DynamicArray<int> probes;
        for (size_t q = 0; q < queries; ++q) {
            probes.PushBack(static_cast<int>(rng() % n));
        }

        double hashMs = MeasureMs([&]() {
            for (size_t q = 0; q < queries; ++q) {
                checksum += FindByKey(table, probes[q]).GetSize();
            }
        });
        double treeMs = MeasureMs([&]() {
            for (size_t q = 0; q < queries; ++q) {
                checksum += FindByKey(tree, probes[q]).GetSize();
            }
        });
        double linearMs = MeasureMs([&]() {
            for (size_t q = 0; q < linearQueries; ++q) {
                checksum += LinearFind(keys, IdentityKey, probes[q]).GetSize();
            }
        });

        double hashUs = hashMs * 1000.0 / queries;
        double treeUs = treeMs * 1000.0 / queries;
        double linearUs = linearMs * 1000.0 / linearQueries;

        csv << n << "," << hashUs << "," << treeUs << "," << linearUs << "\n";
        std::cout << std::setw(10) << n << std::setw(14) << hashUs << std::setw(14) << treeUs
                  << std::setw(16) << linearUs << "\n";
    }
    std::cout << "контрольная сумма " << checksum << "\n";
    std::cout << "результаты записаны в " << csvPath << "\n";
}

// сравнение поиска по диапазону в миллисекундах
// диапазон покрывает один процент ключей
inline void RunRangeBenchmark(const DynamicArray<size_t>& sizes, const std::string& csvPath) {
    std::ofstream csv(csvPath);
    csv << "n,found,tree_ms,hash_scan_ms,linear_ms\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::setw(10) << "n" << std::setw(10) << "найдено" << std::setw(14) << "дерево мс"
              << std::setw(16) << "хеш перебор мс" << std::setw(16) << "линейный мс" << "\n";

    for (size_t s = 0; s < sizes.GetSize(); ++s) {
        size_t n = sizes[s];
        DynamicArray<int> keys = MakeKeys(n, KeyOrder::Random);
        HashTable<int, DynamicArray<int>> table(&HashKey<int>);
        BinaryTree<int, DynamicArray<int>> tree;
        BuildIndex(keys, IdentityKey, table);
        BuildIndex(keys, IdentityKey, tree);

        int lo = static_cast<int>(n / 2);
        int hi = static_cast<int>(n / 2 + n / 100);
        int repeats = RepeatsFor(n);
        size_t found = 0;

        double treeMs = MeasureMedianMs([&]() { found = FindRangeTree(tree, lo, hi).GetSize(); }, repeats);
        double hashMs = MeasureMedianMs([&]() { found = FindRangeHashScan(table, lo, hi).GetSize(); }, repeats);
        double linearMs = MeasureMedianMs([&]() { found = LinearFindRange(keys, IdentityKey, lo, hi).GetSize(); },
                                          repeats);

        csv << n << "," << found << "," << treeMs << "," << hashMs << "," << linearMs << "\n";
        std::cout << std::setw(10) << n << std::setw(10) << found << std::setw(14) << treeMs
                  << std::setw(16) << hashMs << std::setw(16) << linearMs << "\n";
    }
    std::cout << "результаты записаны в " << csvPath << "\n";
}