#pragma once

#include <iostream>

// счетчики определены в Tests.cpp
extern int g_testFailures;
extern int g_testChecks;

// проверка условия
// свои макросы вместо assert потому что assert отключается в Release
#define CHECK(cond)                                                                     \
    do {                                                                                \
        ++g_testChecks;                                                                 \
        if (!(cond)) {                                                                  \
            ++g_testFailures;                                                           \
            std::cout << "\n  ошибка " << __FILE__ << ":" << __LINE__ << " " << #cond; \
        }                                                                               \
    } while (0)

// проверка что выражение бросает исключение нужного типа
#define CHECK_THROWS(expr, ExceptionType)                                                       \
    do {                                                                                        \
        ++g_testChecks;                                                                         \
        bool thrown = false;                                                                    \
        try {                                                                                   \
            expr;                                                                               \
        } catch (const ExceptionType&) {                                                        \
            thrown = true;                                                                      \
        } catch (...) {                                                                         \
        }                                                                                       \
        if (!thrown) {                                                                          \
            ++g_testFailures;                                                                   \
            std::cout << "\n  нет исключения " << __FILE__ << ":" << __LINE__ << " " << #expr; \
        }                                                                                       \
    } while (0)