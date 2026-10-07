#include "Tests.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include "Benchmark.h"
#include "BinaryTree.h"
#include "CompositeKey.h"
#include "DataGenerator.h"
#include "DynamicArray.h"
#include "HashTable.h"
#include "Index.h"
#include "Person.h"
#include "TestUtils.h"

int g_testFailures = 0;
int g_testChecks = 0;

namespace {

    using FullKey = CompositeKey3<std::string, std::string, int>;

// хеш который всегда возвращает ноль чтобы все ключи попали в одну цепочку
    size_t ConstantHash(const int&) {
        return 0;
    }

// достает год рождения из человека
    int YearOf(const Person& p) {
        return p.GetBirthYear();
    }

// достает составной ключ имя фамилия год
    FullKey FullKeyOf(const Person& p) {
        return FullKey(p.GetFirstName(), p.GetLastName(), p.GetBirthYear());
    }

// проверяет что индекс обоих видов дает те же позиции что и линейный поиск
    template <typename TKey, typename Extractor>
    void CheckIndexEqualsLinear(const DynamicArray<Person>& people, Extractor extractor, size_t samples) {
        IndexPair<TKey> idx;
        BuildIndex(people, extractor, idx.hash);
        BuildIndex(people, extractor, idx.tree);
        CHECK(idx.hash.GetCount() == idx.tree.GetCount());
        for (size_t i = 0; i < samples && i < people.GetSize(); ++i) {
            TKey key = extractor(people[i]);
            DynamicArray<int> viaHash = FindByKey(idx.hash, key);
            DynamicArray<int> viaTree = FindByKey(idx.tree, key);
            DynamicArray<int> viaLinear = LinearFind(people, extractor, key);
            CHECK(SameContent(viaHash, viaLinear));
            CHECK(SameContent(viaTree, viaLinear));
        }
    }

    void TestDynamicArray() {
        DynamicArray<int> a;
        CHECK(a.IsEmpty());
        for (int i = 0; i < 100; ++i) {
            a.PushBack(i);
        }
        CHECK(a.GetSize() == 100);
        CHECK(a[0] == 0 && a[99] == 99);
        CHECK_THROWS(a.Get(100), std::out_of_range);

        DynamicArray<int> copy = a;
        copy[0] = 500;
        CHECK(a[0] == 0);
    }

    void TestHashTableBasic() {
        HashTable<int, int> t(&HashKey<int>, 4);
        t.Add(1, 10);
        t.Add(2, 20);
        CHECK(t.GetCount() == 2);
        CHECK(t.Get(1) == 10);
        t.Get(1) = 11;
        CHECK(t.Get(1) == 11);
        CHECK(t.ContainsKey(2));
        CHECK(!t.ContainsKey(3));
        CHECK_THROWS(t.Get(3), std::out_of_range);
        CHECK_THROWS(t.Add(1, 5), std::invalid_argument);
        CHECK_THROWS(t.Remove(3), std::out_of_range);
        t.Remove(1);
        CHECK(!t.ContainsKey(1));
        CHECK(t.GetCount() == 1);
    }

    void TestHashTableRebuild() {
        HashTable<int, int> t(&HashKey<int>, 4, 4.0, 2.0);
        CHECK(t.GetCapacity() == 4);
        for (int i = 0; i < 4; ++i) {
            t.Add(i, i);
        }
        // элементов стало столько же сколько корзин поэтому таблица выросла в два раза
        CHECK(t.GetCapacity() == 8);
        for (int i = 4; i < 8; ++i) {
            t.Add(i, i);
        }
        CHECK(t.GetCapacity() == 16);
        for (int i = 0; i < 8; ++i) {
            CHECK(t.Get(i) == i);
        }
        // после удаления четырех элементов осталось 4 и это не больше 16 делить на 4
        for (int i = 0; i < 4; ++i) {
            t.Remove(i);
        }
        CHECK(t.GetCapacity() == 8);
        for (int i = 4; i < 8; ++i) {
            CHECK(t.Get(i) == i);
        }
    }

    void TestHashTableCollisions() {
        HashTable<int, int> t(&ConstantHash, 4);
        for (int i = 0; i < 100; ++i) {
            t.Add(i, i * 2);
        }
        CHECK(t.GetCount() == 100);
        CHECK(t.GetMaxChainLength() == 100);
        for (int i = 0; i < 100; ++i) {
            CHECK(t.Get(i) == i * 2);
        }
        t.Remove(0);
        CHECK(!t.ContainsKey(0));
        CHECK(t.ContainsKey(1));
    }

    void TestBinaryTreeBasic() {
        BinaryTree<int, int> tree;
        CHECK_THROWS(tree.Get(1), std::out_of_range);
        CHECK_THROWS(tree.Remove(1), std::out_of_range);

        tree.Add(5, 50);
        tree.Add(3, 30);
        tree.Add(8, 80);
        CHECK(tree.GetCount() == 3);
        CHECK(tree.Get(3) == 30);
        CHECK(tree.ContainsKey(8));
        CHECK(!tree.ContainsKey(4));
        CHECK_THROWS(tree.Add(5, 1), std::invalid_argument);
        CHECK(tree.GetHeight() == 2);
    }

    void TestBinaryTreeRemove() {
        BinaryTree<int, int> tree;
        int keys[] = {50, 30, 70, 20, 40, 60, 80, 65};
        for (int key : keys) {
            tree.Add(key, key * 10);
        }
        // лист
        tree.Remove(20);
        // один потомок
        tree.Remove(60);
        // два потомка
        tree.Remove(70);
        tree.Remove(50);
        CHECK(tree.GetCount() == 4);
        CHECK(!tree.ContainsKey(20));
        CHECK(!tree.ContainsKey(60));
        CHECK(!tree.ContainsKey(70));
        CHECK(!tree.ContainsKey(50));
        CHECK(tree.Get(30) == 300);
        CHECK(tree.Get(40) == 400);
        CHECK(tree.Get(65) == 650);
        CHECK(tree.Get(80) == 800);
    }

    void TestBinaryTreeRange() {
        DynamicArray<int> keys = MakeKeys(100, KeyOrder::Random);
        BinaryTree<int, int> tree;
        for (size_t i = 0; i < keys.GetSize(); ++i) {
            tree.Add(keys[i], keys[i]);
        }
        auto count = [&tree](int lo, int hi) {
            size_t c = 0;
            tree.ForEachInRange(lo, hi, [&c](const int&, const int&) { ++c; });
            return c;
        };
        CHECK(count(10, 20) == 11);
        CHECK(count(42, 42) == 1);
        CHECK(count(20, 10) == 0);
        CHECK(count(200, 300) == 0);
        CHECK(count(-1000, 1000) == 100);
    }

    void TestBinaryTreeDegenerate() {
        const size_t n = 5000;
        BinaryTree<int, int> sorted;
        DynamicArray<int> sortedKeys = MakeKeys(n, KeyOrder::Sorted);
        for (size_t i = 0; i < n; ++i) {
            sorted.Add(sortedKeys[i], 0);
        }
        // на упорядоченных ключах дерево превратилось в список
        CHECK(sorted.GetHeight() == n);

        BinaryTree<int, int> random;
        DynamicArray<int> randomKeys = MakeKeys(n, KeyOrder::Random);
        for (size_t i = 0; i < n; ++i) {
            random.Add(randomKeys[i], 0);
        }
        CHECK(random.GetHeight() < 100);
    }

    void TestCompositeKey() {
        CompositeKey2<std::string, int> a("ivanov", 30);
        CompositeKey2<std::string, int> b("ivanov", 30);
        CompositeKey2<std::string, int> c("ivanov", 31);
        CompositeKey2<std::string, int> d("petrov", 1);
        CHECK(a == b);
        CHECK(!(a == c));
        CHECK(a < c);
        CHECK(c < d);
        CHECK(!(a < b));
        CHECK(HashKey(a) == HashKey(b));
    }

    void TestIndexes() {
        DynamicArray<Person> people = GeneratePeople(2000, 1);
        CHECK(people.GetSize() == 2000);

        CheckIndexEqualsLinear<int>(people, YearOf, 300);
        CheckIndexEqualsLinear<FullKey>(people, FullKeyOf, 300);

        IndexPair<int> idx;
        BuildIndex(people, YearOf, idx.hash);
        BuildIndex(people, YearOf, idx.tree);
        CHECK(FindByKey(idx.hash, 1800).IsEmpty());

        DynamicArray<int> viaTree = FindRangeTree(idx.tree, 1960, 1970);
        DynamicArray<int> viaHash = FindRangeHashScan(idx.hash, 1960, 1970);
        DynamicArray<int> viaLinear = LinearFindRange(people, YearOf, 1960, 1970);
        CHECK(viaLinear.GetSize() > 0);
        CHECK(SameContent(viaTree, viaLinear));
        CHECK(SameContent(viaHash, viaLinear));
    }

    void TestIndexEdgeCases() {
        // пустые данные
        DynamicArray<Person> none;
        IndexPair<int> emptyIdx;
        BuildIndex(none, YearOf, emptyIdx.hash);
        BuildIndex(none, YearOf, emptyIdx.tree);
        CHECK(emptyIdx.hash.GetCount() == 0);
        CHECK(FindByKey(emptyIdx.tree, 1990).IsEmpty());
        CHECK(FindRangeTree(emptyIdx.tree, 1900, 2100).IsEmpty());

        // все люди с одним годом
        DynamicArray<Person> same;
        for (int i = 0; i < 500; ++i) {
            same.PushBack(Person("A", "B", "C", 2000));
        }
        IndexPair<int> sameIdx;
        BuildIndex(same, YearOf, sameIdx.hash);
        BuildIndex(same, YearOf, sameIdx.tree);
        CHECK(sameIdx.hash.GetCount() == 1);
        CHECK(FindByKey(sameIdx.hash, 2000).GetSize() == 500);
        CHECK(FindByKey(sameIdx.tree, 2000).GetSize() == 500);
    }

    using TestFunction = void (*)();

    void RunTest(const char* name, TestFunction test) {
        int before = g_testFailures;
        std::cout << name << " ... ";
        try {
            test();
        } catch (const std::exception& e) {
            ++g_testFailures;
            std::cout << "\n  неожиданное исключение " << e.what();
        } catch (...) {
            ++g_testFailures;
            std::cout << "\n  неожиданное исключение";
        }
        std::cout << (g_testFailures == before ? "ok" : "\n  провал") << "\n";
    }

}  // namespace

int RunAllTests() {
    g_testFailures = 0;
    g_testChecks = 0;

    RunTest("динамический массив", TestDynamicArray);
    RunTest("хеш таблица основные операции", TestHashTableBasic);
    RunTest("хеш таблица перестройка", TestHashTableRebuild);
    RunTest("хеш таблица коллизии", TestHashTableCollisions);
    RunTest("дерево основные операции", TestBinaryTreeBasic);
    RunTest("дерево удаление", TestBinaryTreeRemove);
    RunTest("дерево диапазон", TestBinaryTreeRange);
    RunTest("дерево вырождение", TestBinaryTreeDegenerate);
    RunTest("составной ключ", TestCompositeKey);
    RunTest("индексы против линейного поиска", TestIndexes);
    RunTest("индексы граничные случаи", TestIndexEdgeCases);

    std::cout << "проверок " << g_testChecks << " провалов " << g_testFailures << "\n";
    return g_testFailures;
}