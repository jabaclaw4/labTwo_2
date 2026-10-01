#pragma once

#include <cstddef>
#include <string>

#include "Hash.h"

// хеш отдельных значений
inline size_t HashValue(const int& value) {
    return HashInt(value);
}

inline size_t HashValue(const std::string& value) {
    return HashString(value);
}

// смешивание двух хешей в один
inline size_t CombineHash(size_t seed, size_t value) {
    return seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2));
}

// составной ключ из двух полей
// сравнение лексикографическое сначала первое поле потом второе
template <typename A, typename B>
struct CompositeKey2 {
    A first;
    B second;

    CompositeKey2() : first(), second() {}
    CompositeKey2(const A& a, const B& b) : first(a), second(b) {}

    bool operator==(const CompositeKey2& other) const {
        return first == other.first && second == other.second;
    }

    bool operator<(const CompositeKey2& other) const {
        if (first < other.first) {
            return true;
        }
        if (other.first < first) {
            return false;
        }
        return second < other.second;
    }
};

template <typename A, typename B>
size_t HashValue(const CompositeKey2<A, B>& key) {
    return CombineHash(HashValue(key.first), HashValue(key.second));
}

// составной ключ из трех полей
template <typename A, typename B, typename C>
struct CompositeKey3 {
    A first;
    B second;
    C third;

    CompositeKey3() : first(), second(), third() {}
    CompositeKey3(const A& a, const B& b, const C& c) : first(a), second(b), third(c) {}

    bool operator==(const CompositeKey3& other) const {
        return first == other.first && second == other.second && third == other.third;
    }

    bool operator<(const CompositeKey3& other) const {
        if (first < other.first) {
            return true;
        }
        if (other.first < first) {
            return false;
        }
        if (second < other.second) {
            return true;
        }
        if (other.second < second) {
            return false;
        }
        return third < other.third;
    }
};

template <typename A, typename B, typename C>
size_t HashValue(const CompositeKey3<A, B, C>& key) {
    return CombineHash(CombineHash(HashValue(key.first), HashValue(key.second)), HashValue(key.third));
}

// единая функция хеша для любого ключа
// ее адрес передается в хеш таблицу
template <typename TKey>
size_t HashKey(const TKey& key) {
    return HashValue(key);
}