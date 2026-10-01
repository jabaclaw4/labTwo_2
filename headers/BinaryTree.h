#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.h"
#include "IDictionary.h"

// бинарное дерево поиска без балансировки
// все операции без рекурсии потому что на упорядоченных данных дерево вырождается в список
// рекурсия глубиной в сотни тысяч вызовов переполнила бы стек
template <typename TKey, typename TElement>
class BinaryTree : public IDictionary<TKey, TElement> {
private:
    struct Node {
        TKey key;
        TElement element;
        Node* left;
        Node* right;

        Node(const TKey& k, const TElement& e) : key(k), element(e), left(nullptr), right(nullptr) {}
    };

    Node* root_;
    size_t count_;

    Node* FindNode(const TKey& key) const {
        Node* cur = root_;
        while (cur != nullptr) {
            if (key < cur->key) {
                cur = cur->left;
            } else if (cur->key < key) {
                cur = cur->right;
            } else {
                return cur;
            }
        }
        return nullptr;
    }

    // удаляем узлы через свой стек чтобы деструктор тоже не уходил в рекурсию
    void FreeAll() {
        if (root_ == nullptr) {
            return;
        }
        DynamicArray<Node*> stack;
        stack.PushBack(root_);
        while (!stack.IsEmpty()) {
            Node* node = stack[stack.GetSize() - 1];
            stack.PopBack();
            if (node->left != nullptr) {
                stack.PushBack(node->left);
            }
            if (node->right != nullptr) {
                stack.PushBack(node->right);
            }
            delete node;
        }
        root_ = nullptr;
        count_ = 0;
    }

public:
    BinaryTree() : root_(nullptr), count_(0) {}

    BinaryTree(const BinaryTree&) = delete;
    BinaryTree& operator=(const BinaryTree&) = delete;

    BinaryTree(BinaryTree&& other) noexcept : root_(other.root_), count_(other.count_) {
        other.root_ = nullptr;
        other.count_ = 0;
    }

    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if (this != &other) {
            FreeAll();
            root_ = other.root_;
            count_ = other.count_;
            other.root_ = nullptr;
            other.count_ = 0;
        }
        return *this;
    }

    ~BinaryTree() override {
        FreeAll();
    }

    size_t GetCount() const override { return count_; }

    // у дерева нет перестройки поэтому вместимость равна количеству
    size_t GetCapacity() const override { return count_; }

    TElement& Get(const TKey& key) override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("BinaryTree::Get key not found");
        }
        return node->element;
    }

    const TElement& Get(const TKey& key) const override {
        Node* node = FindNode(key);
        if (node == nullptr) {
            throw std::out_of_range("BinaryTree::Get key not found");
        }
        return node->element;
    }

    bool ContainsKey(const TKey& key) const override {
        return FindNode(key) != nullptr;
    }

    void Add(const TKey& key, const TElement& element) override {
        // link указывает на то место куда надо повесить новый узел
        Node** link = &root_;
        while (*link != nullptr) {
            if (key < (*link)->key) {
                link = &(*link)->left;
            } else if ((*link)->key < key) {
                link = &(*link)->right;
            } else {
                throw std::invalid_argument("BinaryTree::Add key already exists");
            }
        }
        *link = new Node(key, element);
        ++count_;
    }

    void Remove(const TKey& key) override {
        Node** link = &root_;
        while (*link != nullptr) {
            if (key < (*link)->key) {
                link = &(*link)->left;
            } else if ((*link)->key < key) {
                link = &(*link)->right;
            } else {
                break;
            }
        }
        if (*link == nullptr) {
            throw std::out_of_range("BinaryTree::Remove key not found");
        }

        Node* node = *link;
        if (node->left == nullptr) {
            // нет левого потомка или это лист
            *link = node->right;
        } else if (node->right == nullptr) {
            *link = node->left;
        } else {
            // два потомка берем минимум правого поддерева и ставим на место удаляемого
            Node** successorLink = &node->right;
            while ((*successorLink)->left != nullptr) {
                successorLink = &(*successorLink)->left;
            }
            Node* successor = *successorLink;
            *successorLink = successor->right;
            successor->left = node->left;
            successor->right = node->right;
            *link = successor;
        }
        delete node;
        --count_;
    }

    // обход ключей из отрезка от lo до hi включительно по возрастанию
    // ветки целиком вне отрезка пропускаются поэтому время O(h + k)
    template <typename Visitor>
    void ForEachInRange(const TKey& lo, const TKey& hi, Visitor visit) const {
        DynamicArray<Node*> stack;
        Node* cur = root_;
        while (cur != nullptr || !stack.IsEmpty()) {
            while (cur != nullptr) {
                if (cur->key < lo) {
                    // этот узел и все левое меньше lo
                    cur = cur->right;
                } else {
                    stack.PushBack(cur);
                    cur = cur->left;
                }
            }
            if (stack.IsEmpty()) {
                break;
            }
            Node* node = stack[stack.GetSize() - 1];
            stack.PopBack();
            if (hi < node->key) {
                // дальше только большие ключи
                break;
            }
            visit(node->key, node->element);
            cur = node->right;
        }
    }

    // высота считается по уровням без рекурсии
    size_t GetHeight() const {
        if (root_ == nullptr) {
            return 0;
        }
        DynamicArray<Node*> level;
        level.PushBack(root_);
        size_t height = 0;
        while (!level.IsEmpty()) {
            ++height;
            DynamicArray<Node*> next;
            for (size_t i = 0; i < level.GetSize(); ++i) {
                if (level[i]->left != nullptr) {
                    next.PushBack(level[i]->left);
                }
                if (level[i]->right != nullptr) {
                    next.PushBack(level[i]->right);
                }
            }
            level = std::move(next);
        }
        return height;
    }
};