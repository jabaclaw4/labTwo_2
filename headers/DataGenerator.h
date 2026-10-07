#pragma once

#include <cstddef>
#include <random>

#include "DynamicArray.h"
#include "Person.h"

// генерация людей для автоматического режима
// одинаковый seed всегда дает одинаковый набор
inline DynamicArray<Person> GeneratePeople(size_t count, unsigned seed) { //inline сообщает что одинаковые копии допустимы и нужно оставить одну по той же причине inline стоит у HashInt и HashString в Hash.h
    static const char* const firstNames[] = {
            "Ivan", "Petr", "Alexey", "Dmitry", "Sergey", "Nikolay", "Maria", "Anna", "Olga", "Elena",
            "Irina", "Natalia", "Pavel", "Andrey", "Victor", "Tatiana", "Svetlana", "Yuri", "Oleg", "Vera"};
    static const char* const middleNames[] = {
            "Ivanovich", "Petrovich", "Alexeevich", "Sergeevich", "Andreevich",
            "Nikolaevich", "Pavlovna", "Ivanovna", "Petrovna", "Sergeevna"};
    static const char* const lastNames[] = {
            "Ivanov", "Petrov", "Sidorov", "Smirnov", "Kuznetsov", "Popov", "Vasiliev", "Sokolov", "Mikhailov", "Novikov",
            "Fedorov", "Morozov", "Volkov", "Alexeev", "Lebedev", "Semenov", "Egorov", "Pavlov", "Kozlov", "Stepanov"};
//массив создаётся один раз за всё время работы программы
    const size_t firstCount = sizeof(firstNames) / sizeof(firstNames[0]);//получаем число элементов
    const size_t middleCount = sizeof(middleNames) / sizeof(middleNames[0]);
    const size_t lastCount = sizeof(lastNames) / sizeof(lastNames[0]);

    std::mt19937 rng(seed);//seed задаёт его начальное состояние: одинаковый seed даёт одинаковую последовательность.
    DynamicArray<Person> people;
    for (size_t i = 0; i < count; ++i) {
        // порядок вызовов rng фиксируем отдельными строками
        size_t firstIndex = rng() % firstCount;
        size_t middleIndex = rng() % middleCount;
        size_t lastIndex = rng() % lastCount;
        int birthYear = 1940 + static_cast<int>(rng() % 71);
        //чтобы у многих людей совпадали годы и в индексе у ключа было много позиций.
        people.PushBack(Person(firstNames[firstIndex], middleNames[middleIndex], lastNames[lastIndex], birthYear));
    }
    return people;
}