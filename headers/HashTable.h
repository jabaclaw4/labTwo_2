#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.h"
#include "IDictionary.h"

// хеш таблица с цепочками
// корзины лежат в динамическом массиве а в каждой корзине односвязный список
template <typename TKey, typename TElement>
class HashTable : public IDictionary<TKey, TElement> {
public:
    using HashFunction = size_t (*)(const TKey&);

private:
    struct Node {
        TKey key;
        TElement element;
        Node* next;

        Node(const TKey& k, const TElement& e, Node* n) : key(k), element(e), next(n) {}
    };

    DynamicArray<Node*> buckets_;
    size_t count_;
    HashFunction hash_;
    double p_;
    double q_;

    size_t IndexFor(const TKey& key, size_t capacity) const {
        return hash_(key) % capacity;
    }

    Node* FindNode(const TKey& key) const {
        Node* cur = buckets_[IndexFor(key, buckets_.GetSize())];
        while (cur != nullptr) {
            if (cur->key == key) {
                return cur;
            }
            cur = cur->next;
        }
        return nullptr;
    }

    // удаляет все узлы и оставляет таблицу пустой
    void FreeNodes() {
        for (size_t i = 0; i < buckets_.GetSize(); ++i) {
            Node* cur = buckets_[i];
            while (cur != nullptr) {
                Node* next = cur->next;
                delete cur;
                cur = next;
            }
            buckets_[i] = nullptr;
        }
        count_ = 0;
    }

    // перестройка
    // узлы не копируются а только перевешиваются в новые корзины
    void Rebuild(size_t newCapacity) {
        DynamicArray<Node*> newBuckets(newCapacity, nullptr);
        for (size_t i = 0; i < buckets_.GetSize(); ++i) {
            Node* cur = buckets_[i];
            while (cur != nullptr) {
                Node* next = cur->next;
                size_t index = IndexFor(cur->key, newCapacity);
                cur->next = newBuckets[index];
                newBuckets[index] = cur;
                cur = next;
            }
        }
        buckets_ = std::move(newBuckets);
    }

    // расширение когда элементов стало столько же сколько корзин
    void GrowIfNeeded() {
        size_t capacity = buckets_.GetSize();
        if (count_ >= capacity) {
            size_t newCapacity = static_cast<size_t>(static_cast<double>(capacity) * q_);
            if (newCapacity <= capacity) {
                newCapacity = capacity + 1;
            }
            Rebuild(newCapacity);
        }
    }

    // сжатие когда элементов стало не больше чем вместимость делить на p
    void ShrinkIfNeeded() {
        size_t capacity = buckets_.GetSize();
        if (static_cast<double>(count_) <= static_cast<double>(capacity) / p_) {
            size_t newCapacity = static_cast<size_t>(static_cast<double>(capacity) / q_);
            if (newCapacity < 1) {
                newCapacity = 1;
            }
            if (newCapacity < capacity) {
                Rebuild(newCapacity);
            }
        }
    }

public:
    // hash хеш функция для ключа
    // p и q параметры перестройки и должно выполняться p >= q > 1
    HashTable(HashFunction hash, size_t initialCapacity = 16, double p = 4.0, double q = 2.0)
            : buckets_(initialCapacity, nullptr), count_(0), hash_(hash), p_(p), q_(q) {
        if (hash == nullptr) {
            throw std::invalid_argument("HashTable hash function is null");
        }
        if (initialCapacity == 0) {
            throw std::invalid_argument("HashTable capacity must be positive");
        }
        if (!(q > 1.0) || !(p >= q)) {
            throw std::invalid_argument("HashTable requires p >= q > 1");
        }
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // после перемещения старый объект использовать нельзя
    HashTable(HashTable&& other) noexcept
            : buckets_(std::move(other.buckets_)),
            count_(other.count_),
    hash_(other.hash_),
    p_(other.p_),
    q_(other.q_) {
        other.count_ = 0;
    }

    HashTable& operator=(HashTable&& other) noexcept {
        if (this != &other) {
            FreeNodes();
            buckets_ = std::move(other.buckets_);
            count_ = other.count_;
            hash_ = other.hash_;
            p_ = other.p_;
            q_ = other.q_;
            other.count_ = 0;
        }
        return *this;
    }

    ~HashTable() override {
        FreeNodes();
    }

    size_t GetCount() const override { return count_; }

    size_t GetCapacity() const override { return buckets_.GetSize(); }

    TElement& Get(const TKey& key) override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("HashTable::Get key not found");
        }
        return node->element;
    }

    const TElement& Get(const TKey& key) const override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("HashTable::Get key not found");
        }
        return node->element;
    }

    bool ContainsKey(const TKey& key) const override {
        return FindNode(key) != nullptr;
    }

    void Add(const TKey& key, const TElement& element) override {
        if (FindNode(key) != nullptr) {
            throw std::invalid_argument("HashTable::Add key already exists");
        }
        size_t index = IndexFor(key, buckets_.GetSize());
        buckets_[index] = new Node(key, element, buckets_[index]);
        ++count_;
        GrowIfNeeded();
    }

    void Remove(const TKey& key) override {
        size_t index = IndexFor(key, buckets_.GetSize());
        Node* prev = nullptr;
        Node* cur = buckets_[index];
        while (cur != nullptr && !(cur->key == key)) {
            prev = cur;
            cur = cur->next;
        }
        if (cur == nullptr) {
            throw std::out_of_range("HashTable::Remove key not found");
        }
        if (prev != nullptr) {
            prev->next = cur->next;
        } else {
            buckets_[index] = cur->next;
        }
        delete cur;
        --count_;
        ShrinkIfNeeded();
    }

    // самая длинная цепочка нужна для тестов и для оценки качества хеша
    size_t GetMaxChainLength() const {
        size_t maxLength = 0;
        for (size_t i = 0; i < buckets_.GetSize(); ++i) {
            size_t length = 0;
            for (Node* cur = buckets_[i]; cur != nullptr; cur = cur->next) {
                ++length;
            }
            if (length > maxLength) {
                maxLength = length;
            }
        }
        return maxLength;
    }

    // обход всех элементов в порядке корзин
    template <typename Visitor>
    void ForEach(Visitor visit) const {
        for (size_t i = 0; i < buckets_.GetSize(); ++i) {
            for (Node* cur = buckets_[i]; cur != nullptr; cur = cur->next) {
                visit(cur->key, cur->element);
            }
        }
    }
};