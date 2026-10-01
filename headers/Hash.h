#pragma once

#include <cstddef>
#include <string>

// хеш для целых чисел
// умножаем на константу чтобы близкие числа разошлись по таблице
inline size_t HashInt(const int& key) {
    size_t h = static_cast<size_t>(static_cast<unsigned int>(key));
    return h * 2654435761u;
}

// хеш для строк
// считаем в беззнаковом типе чтобы не было отрицательных значений
inline size_t HashString(const std::string& key) {
    size_t h = 0;
    for (char c : key) {
        h = h * 31 + static_cast<unsigned char>(c);
    }
    return h;
}