#pragma once

#include <cstddef>

// интерфейс словаря
// хеш таблица и дерево будут его реализовывать
template <typename TKey, typename TElement>
class IDictionary {
public:
    virtual ~IDictionary() = default;

    // сколько элементов лежит сейчас
    virtual size_t GetCount() const = 0;

    // сколько влезет без перестройки
    virtual size_t GetCapacity() const = 0;

    // возвращает ссылку на значение чтобы его можно было менять прямо в словаре
    // бросает исключение если ключа нет
    virtual TElement& Get(const TKey& key) = 0;

    // то же самое только для константного словаря
    virtual const TElement& Get(const TKey& key) const = 0;

    // не бросает исключений
    virtual bool ContainsKey(const TKey& key) const = 0;

    // бросает исключение если такой ключ уже есть
    virtual void Add(const TKey& key, const TElement& element) = 0;

    // бросает исключение если ключа нет
    virtual void Remove(const TKey& key) = 0;
};