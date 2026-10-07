#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "Benchmark.h"
#include "CompositeKey.h"
#include "DataGenerator.h"
#include "Index.h"
#include "Person.h"
#include "Tests.h"
#include "Timer.h"

namespace {

    using FullKey = CompositeKey3<std::string, std::string, int>;

// достает год рождения из человека
    int YearOf(const Person& p) {
        return p.GetBirthYear();
    }

// достает составной ключ имя фамилия год
    FullKey FullKeyOf(const Person& p) {
        return FullKey(p.GetFirstName(), p.GetLastName(), p.GetBirthYear());
    }

// все что программа помнит между пунктами меню
    struct AppState {
        DynamicArray<Person> people;
        IndexPair<int> byYear;
        IndexPair<FullKey> byFullKey;
        bool indexBuilt;

        AppState() : indexBuilt(false) {}
    };

    std::string ReadLine(const std::string& prompt) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::exit(0);
        }
        return line;
    }

// спрашивает число пока не введут целое
    int ReadInt(const std::string& prompt) {
        while (true) {
            std::string line = ReadLine(prompt);
            try {
                size_t used = 0;
                int value = std::stoi(line, &used);
                if (used == line.size()) {
                    return value;
                }
            } catch (const std::exception&) {
            }
            std::cout << "нужно целое число\n";
        }
    }

    void PrintPeople(const DynamicArray<Person>& people, const DynamicArray<int>& positions, size_t limit) {
        for (size_t i = 0; i < positions.GetSize() && i < limit; ++i) {
            const Person& p = people[static_cast<size_t>(positions[i])];
            std::cout << "  [" << positions[i] << "] " << p.GetFullName() << " " << p.GetBirthYear() << "\n";
        }
        if (positions.GetSize() > limit) {
            std::cout << "  и еще " << (positions.GetSize() - limit) << "\n";
        }
    }

    void GenerateData(AppState& state) {
        int count = ReadInt("сколько человек сгенерировать: ");
        if (count <= 0) {
            std::cout << "количество должно быть положительным\n";
            return;
        }
        int seed = ReadInt("seed: ");
        state.people = GeneratePeople(static_cast<size_t>(count), static_cast<unsigned>(seed));
        state.indexBuilt = false;
        std::cout << "сгенерировано " << state.people.GetSize() << "\n";
    }

    void ShowData(const AppState& state) {
        if (state.people.IsEmpty()) {
            std::cout << "данных нет\n";
            return;
        }
        int count = ReadInt("сколько показать: ");
        for (int i = 0; i < count && static_cast<size_t>(i) < state.people.GetSize(); ++i) {
            const Person& p = state.people[static_cast<size_t>(i)];
            std::cout << "  [" << i << "] " << p.GetFullName() << " " << p.GetBirthYear() << "\n";
        }
    }

// строит оба индекса по одному атрибуту и печатает время
    template <typename TKey, typename Extractor>
    void BuildBoth(const char* title, const DynamicArray<Person>& people, Extractor extractor, IndexPair<TKey>& idx) {
        idx = IndexPair<TKey>();
        double hashMs = MeasureMs([&]() { BuildIndex(people, extractor, idx.hash); });
        double treeMs = MeasureMs([&]() { BuildIndex(people, extractor, idx.tree); });
        std::cout << "  " << title << ": хеш " << hashMs << " мс дерево " << treeMs << " мс разных ключей "
                  << idx.hash.GetCount() << "\n";
    }

    void BuildAllIndexes(AppState& state) {
        if (state.people.IsEmpty()) {
            std::cout << "сначала нужны данные\n";
            return;
        }
        std::cout << "время построения индексов\n";
        BuildBoth("год рождения", state.people, YearOf, state.byYear);
        BuildBoth("имя фамилия год", state.people, FullKeyOf, state.byFullKey);
        state.indexBuilt = true;
    }

// ищет ключ тремя способами и сравнивает время и результат
    template <typename TKey, typename Extractor>
    void CompareSearch(const DynamicArray<Person>& people, Extractor extractor, IndexPair<TKey>& idx,
                       const TKey& key) {
        DynamicArray<int> viaHash;
        DynamicArray<int> viaTree;
        DynamicArray<int> viaLinear;
        double hashMs = MeasureMs([&]() { viaHash = FindByKey(idx.hash, key); });
        double treeMs = MeasureMs([&]() { viaTree = FindByKey(idx.tree, key); });
        double linearMs = MeasureMs([&]() { viaLinear = LinearFind(people, extractor, key); });

        std::cout << "найдено " << viaTree.GetSize() << "\n";
        std::cout << "  хеш таблица " << hashMs * 1000.0 << " мкс\n";
        std::cout << "  дерево " << treeMs * 1000.0 << " мкс\n";
        std::cout << "  линейный поиск " << linearMs * 1000.0 << " мкс\n";
        bool same = SameContent(viaHash, viaTree) && SameContent(viaTree, viaLinear);
        std::cout << "результаты трех способов совпадают: " << (same ? "да" : "нет") << "\n";
        PrintPeople(people, viaTree, 10);
    }

    void SearchByYear(AppState& state) {
        if (!state.indexBuilt) {
            std::cout << "сначала постройте индексы\n";
            return;
        }
        int year = ReadInt("год рождения: ");
        CompareSearch(state.people, YearOf, state.byYear, year);
    }

    void SearchByFullKey(AppState& state) {
        if (!state.indexBuilt) {
            std::cout << "сначала постройте индексы\n";
            return;
        }
        std::string first = ReadLine("имя: ");
        std::string last = ReadLine("фамилия: ");
        int year = ReadInt("год рождения: ");
        CompareSearch(state.people, FullKeyOf, state.byFullKey, FullKey(first, last, year));
    }

    void SearchByRange(AppState& state) {
        if (!state.indexBuilt) {
            std::cout << "сначала постройте индексы\n";
            return;
        }
        int lo = ReadInt("от года: ");
        int hi = ReadInt("до года включительно: ");

        DynamicArray<int> viaTree;
        DynamicArray<int> viaHash;
        DynamicArray<int> viaLinear;
        double treeMs = MeasureMs([&]() { viaTree = FindRangeTree(state.byYear.tree, lo, hi); });
        double hashMs = MeasureMs([&]() { viaHash = FindRangeHashScan(state.byYear.hash, lo, hi); });
        double linearMs = MeasureMs([&]() { viaLinear = LinearFindRange(state.people, YearOf, lo, hi); });

        std::cout << "найдено " << viaTree.GetSize() << "\n";
        std::cout << "  дерево " << treeMs << " мс\n";
        std::cout << "  хеш таблица полный перебор " << hashMs << " мс\n";
        std::cout << "  линейный поиск " << linearMs << " мс\n";
        bool same = SameContent(viaHash, viaTree) && SameContent(viaTree, viaLinear);
        std::cout << "результаты трех способов совпадают: " << (same ? "да" : "нет") << "\n";
        PrintPeople(state.people, viaTree, 10);
    }

// размеры для замеров
    DynamicArray<size_t> BenchmarkSizes() {
        DynamicArray<size_t> sizes;
        sizes.PushBack(1000);
        sizes.PushBack(10000);
        sizes.PushBack(100000);
        return sizes;
    }

    void PrintMenu() {
        std::cout << "\n1 сгенерировать данные\n";
        std::cout << "2 показать данные\n";
        std::cout << "3 построить индексы\n";
        std::cout << "4 поиск по году рождения\n";
        std::cout << "5 поиск по составному ключу\n";
        std::cout << "6 поиск по диапазону лет\n";
        std::cout << "7 замер построения индексов\n";
        std::cout << "8 замер поиска и диапазона\n";
        std::cout << "9 запуск тестов\n";
        std::cout << "0 выход\n";
    }

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << std::fixed << std::setprecision(3);

    AppState state;
    while (true) {
        PrintMenu();
        int choice = ReadInt("выбор: ");
        if (choice == 0) {
            break;
        } else if (choice == 1) {
            GenerateData(state);
        } else if (choice == 2) {
            ShowData(state);
        } else if (choice == 3) {
            BuildAllIndexes(state);
        } else if (choice == 4) {
            SearchByYear(state);
        } else if (choice == 5) {
            SearchByFullKey(state);
        } else if (choice == 6) {
            SearchByRange(state);
        } else if (choice == 7) {
            RunBuildBenchmark(BenchmarkSizes(), "build_results.csv");
        } else if (choice == 8) {
            RunSearchBenchmark(BenchmarkSizes(), "search_results.csv");
            RunRangeBenchmark(BenchmarkSizes(), "range_results.csv");
        } else if (choice == 9) {
            int failures = RunAllTests();
            std::cout << (failures == 0 ? "все тесты прошли\n" : "есть провалы\n");
        } else {
            std::cout << "нет такого пункта\n";
        }
    }
    return 0;
}