#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "DynamicArray.h"
#include "IDictionary.h"


enum class TraverseOrder {
    PreOrder,          // корень -> лево -> право
    InOrder,           // лево -> корень -> право
    PostOrder,         // лево -> право -> корень
    ReversePreOrder,   // корень -> право -> лево
    ReverseInOrder,    // право -> корень -> лево
    ReversePostOrder   // право -> лево -> корень
};

// бинарное дерево поиска
// от вырождения защищает периодическая перестройка в идеально сбалансированное
// дерево: как только верхняя оценка высоты превышает 4 * log2(n + 1) + 1,
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
    // верхняя оценка текущей высоты дерева
    // обновляется при Add (глубина нового узла) и точно уточняется при перестройке
    // при Remove высота может только уменьшаться поэтому оценка сверху безопасна
    size_t height_;

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
        height_ = 0;
    }

    // единственная служебная операция кроме поиска и удаления всех узлов
    // если верхняя оценка высоты не превышает порог — ничего не делает
    // иначе пересобирает дерево в идеально сбалансированное из отсортированного обхода
    // порог 4 * log2(n + 1) + 1 выбран так чтобы случайное дерево
    // (у него ожидаемая высота порядка 3 * log2(n)) не перестраивалось слишком часто
    // а вырожденное дерево (высота n) перестраивалось сразу
    void Rebalance() {
        size_t limit = static_cast<size_t>(4.0 * std::log2(static_cast<double>(count_ + 1))) + 1;
        if (height_ <= limit) {
            return;
        }

        // собираем узлы в порядке возрастания ключей без рекурсии
        DynamicArray<Node*> nodes;
        DynamicArray<Node*> stack;
        Node* cur = root_;
        while (cur != nullptr || !stack.IsEmpty()) {
            while (cur != nullptr) {
                stack.PushBack(cur);
                cur = cur->left;
            }
            cur = stack[stack.GetSize() - 1];
            stack.PopBack();
            nodes.PushBack(cur);
            cur = cur->right;
        }

        // рекурсивная сборка по середине отсортированного массива
        // глубина рекурсии здесь log2(n) поэтому переполнения стека не будет
        struct Builder {
            static Node* Build(const DynamicArray<Node*>& nodes, size_t lo, size_t hi) {
                if (lo >= hi) {
                    return nullptr;
                }
                size_t mid = lo + (hi - lo) / 2;
                Node* node = nodes[mid];
                node->left = Build(nodes, lo, mid);
                node->right = Build(nodes, mid + 1, hi);
                return node;
            }
        };
        root_ = Builder::Build(nodes, 0, nodes.GetSize());

        // уточняем height_ после перестройки
        // GetHeight итеративный поэтому рекурсии не будет
        height_ = GetHeight();
    }

public:
    // итератор поверх готовой последовательности обхода
    // хранит у себя копию результата Traverse и просто ходит по ней
    // это осознанно простой вариант: не ленивый обход со стеком,
    // а обход «в один проход с сохранением»
    class Iterator {
    private:
        DynamicArray<std::pair<TKey, TElement>> items_;
        size_t position_;

    public:
        Iterator() : items_(), position_(0) {}

        explicit Iterator(DynamicArray<std::pair<TKey, TElement>> items)
                : items_(std::move(items)), position_(0) {}

        bool HasNext() const { return position_ < items_.GetSize(); }

        const std::pair<TKey, TElement>& Current() const {
            if (!HasNext()) {
                throw std::out_of_range("BinaryTree::Iterator out of range");
            }
            return items_[position_];
        }

        const std::pair<TKey, TElement>& Next() {
            if (!HasNext()) {
                throw std::out_of_range("BinaryTree::Iterator out of range");
            }
            return items_[position_++];
        }

        // возвращает итератор в начало без повторного обхода
        void Reset() { position_ = 0; }

        size_t GetRemaining() const { return items_.GetSize() - position_; }
    };

    BinaryTree() : root_(nullptr), count_(0), height_(0) {}

    // глубокое копирование: структура дерева сохраняется как есть
    // каждый узел копируется отдельно поэтому двойного удаления не будет
    // обход итеративный потому что исходное дерево может быть вырожденным
    BinaryTree(const BinaryTree& other) : root_(nullptr), count_(0), height_(0) {
        count_ = other.count_;
        height_ = other.height_;
        if (other.root_ == nullptr) {
            return;
        }
        root_ = new Node(other.root_->key, other.root_->element);
        // в стеке храним пары старый узел — его копия
        DynamicArray<std::pair<Node*, Node*>> stack;
        stack.PushBack(std::make_pair(other.root_, root_));
        while (!stack.IsEmpty()) {
            std::pair<Node*, Node*> top = stack[stack.GetSize() - 1];
            stack.PopBack();
            Node* src = top.first;
            Node* dst = top.second;
            if (src->left != nullptr) {
                dst->left = new Node(src->left->key, src->left->element);
                stack.PushBack(std::make_pair(src->left, dst->left));
            }
            if (src->right != nullptr) {
                dst->right = new Node(src->right->key, src->right->element);
                stack.PushBack(std::make_pair(src->right, dst->right));
            }
        }
    }

    BinaryTree& operator=(const BinaryTree& other) {
        if (this != &other) {
            BinaryTree tmp(other);
            FreeAll();
            root_ = tmp.root_;
            count_ = tmp.count_;
            height_ = tmp.height_;
            tmp.root_ = nullptr;
            tmp.count_ = 0;
            tmp.height_ = 0;
        }
        return *this;
    }

    BinaryTree(BinaryTree&& other) noexcept
            : root_(other.root_), count_(other.count_), height_(other.height_) {
        other.root_ = nullptr;
        other.count_ = 0;
        other.height_ = 0;
    }

    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if (this != &other) {
            FreeAll();
            root_ = other.root_;
            count_ = other.count_;
            height_ = other.height_;
            other.root_ = nullptr;
            other.count_ = 0;
            other.height_ = 0;
        }
        return *this;
    }

    ~BinaryTree() override {
        FreeAll();
    }

    size_t GetCount() const override { return count_; }

    // GetCapacity унаследован от IDictionary и возвращает GetCount()

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
        if (root_ == nullptr) {
            root_ = new Node(key, element);
            count_ = 1;
            height_ = 1;
            return;
        }
        // идём по дереву одним указателем cur и держим текущую глубину
        // двойной указатель Node** тут не нужен: мы просто доходим до места,
        // где нужного ребёнка ещё нет, и вешаем туда новый узел
        // старые узлы на пути мы не теряем — они и так уже в дереве
        Node* cur = root_;
        size_t depth = 1;
        while (true) {
            if (key < cur->key) {
                if (cur->left == nullptr) {
                    cur->left = new Node(key, element);
                    ++depth;
                    break;
                }
                cur = cur->left;
            } else if (cur->key < key) {
                if (cur->right == nullptr) {
                    cur->right = new Node(key, element);
                    ++depth;
                    break;
                }
                cur = cur->right;
            } else {
                throw std::invalid_argument("BinaryTree::Add key already exists");
            }
            ++depth;
        }
        if (depth > height_) {
            height_ = depth;
        }
        ++count_;
        Rebalance();
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
        // высота после удаления может только уменьшиться поэтому height_ остаётся
        // верхней оценкой и перестройка возможна только если она всё ещё слишком велика
        Rebalance();
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

    // обход всего дерева в заданном порядке с обработкой каждого узла через visitor
    // это основная форма метода: обход не возвращает данные а отдаёт их по одному
    // в вызывающий код, поэтому на её основе без изменения дерева строятся
    // любые дальнейшие задачи (map, where, reduce, сериализация и т.п.)
    // порядок задаёт две вещи: когда выводить корень (сразу, в середине, в конце)
    // и в какую сторону идти по детям (обычное или зеркальное)
    // для всех шести порядков обход итеративный, рекурсии нет
    template <typename Visitor>
    void Traverse(TraverseOrder order, Visitor visit) const {
        if (root_ == nullptr) {
            return;
        }

        // mirrored означает что мы идём сначала вправо а не влево
        bool mirrored = (order == TraverseOrder::ReversePreOrder ||
                         order == TraverseOrder::ReverseInOrder ||
                         order == TraverseOrder::ReversePostOrder);
        bool rootFirst = (order == TraverseOrder::PreOrder ||
                          order == TraverseOrder::ReversePreOrder);
        bool rootMiddle = (order == TraverseOrder::InOrder ||
                           order == TraverseOrder::ReverseInOrder);
        // оставшиеся два (PostOrder, ReversePostOrder) — корень последний

        if (rootFirst) {
            // корень сразу в visitor
            // чтобы из стека первым вынулся нужный ребёнок, кладём его последним
            DynamicArray<Node*> stack;
            stack.PushBack(root_);
            while (!stack.IsEmpty()) {
                Node* node = stack[stack.GetSize() - 1];
                stack.PopBack();
                visit(node->key, node->element);
                if (mirrored) {
                    if (node->left != nullptr) {
                        stack.PushBack(node->left);
                    }
                    if (node->right != nullptr) {
                        stack.PushBack(node->right);
                    }
                } else {
                    if (node->right != nullptr) {
                        stack.PushBack(node->right);
                    }
                    if (node->left != nullptr) {
                        stack.PushBack(node->left);
                    }
                }
            }
            return;
        }

        if (rootMiddle) {
            // идём вглубь по одной стороне, потом корень, потом другая сторона
            DynamicArray<Node*> stack;
            Node* cur = root_;
            while (cur != nullptr || !stack.IsEmpty()) {
                while (cur != nullptr) {
                    stack.PushBack(cur);
                    cur = mirrored ? cur->right : cur->left;
                }
                cur = stack[stack.GetSize() - 1];
                stack.PopBack();
                visit(cur->key, cur->element);
                cur = mirrored ? cur->left : cur->right;
            }
            return;
        }

        // корень идёт последним
        // PostOrder (ЛПК) — это reverse(КПЛ), ReversePostOrder (ПЛК) — это reverse(КЛП)
        // значит для PostOrder ведём КПЛ (postMirrored = true),
        // а для ReversePostOrder — КЛП (postMirrored = false)
        // сначала копим узлы в обратном порядке, потом отдаём visitor в развёрнутом виде
        bool postMirrored = (order == TraverseOrder::PostOrder);
        DynamicArray<Node*> stack;
        DynamicArray<Node*> collected;
        stack.PushBack(root_);
        while (!stack.IsEmpty()) {
            Node* node = stack[stack.GetSize() - 1];
            stack.PopBack();
            collected.PushBack(node);
            if (postMirrored) {
                if (node->left != nullptr) {
                    stack.PushBack(node->left);
                }
                if (node->right != nullptr) {
                    stack.PushBack(node->right);
                }
            } else {
                if (node->right != nullptr) {
                    stack.PushBack(node->right);
                }
                if (node->left != nullptr) {
                    stack.PushBack(node->left);
                }
            }
        }
        for (size_t i = collected.GetSize(); i > 0; --i) {
            Node* node = collected[i - 1];
            visit(node->key, node->element);
        }
    }

    // удобная обёртка: тот же обход но результат собирается в массив пар ключ-значение
    // сама логика обхода живёт в visitor-форме выше, здесь только накопление
    DynamicArray<std::pair<TKey, TElement>> Traverse(TraverseOrder order) const {
        DynamicArray<std::pair<TKey, TElement>> result;
        Traverse(order, [&result](const TKey& key, const TElement& element) {
            result.PushBack(std::make_pair(key, element));
        });
        return result;
    }

    // итератор по дереву в заданном порядке
    // по умолчанию симметричный обход — ключи по возрастанию
    Iterator Begin(TraverseOrder order = TraverseOrder::InOrder) const {
        return Iterator(Traverse(order));
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