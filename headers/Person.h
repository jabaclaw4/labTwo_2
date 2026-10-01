#pragma once

#include <string>
#include <utility>

class Person {
private:
    std::string firstName_;
    std::string middleName_;
    std::string lastName_;
    int birthYear_;

public:
    Person() : birthYear_(0) {}

    Person(std::string firstName, std::string middleName, std::string lastName, int birthYear)
            : firstName_(std::move(firstName)),
              middleName_(std::move(middleName)),
              lastName_(std::move(lastName)),
              birthYear_(birthYear) {}

    std::string GetFirstName() const { return firstName_; }

    std::string GetMiddleName() const { return middleName_; }

    std::string GetLastName() const { return lastName_; }

    // фамилия имя отчество
    std::string GetFullName() const {
        return lastName_ + " " + firstName_ + " " + middleName_;
    }

    // фамилия и инициалы
    std::string GetFIO() const {
        std::string result = lastName_;
        if (!firstName_.empty()) {
            result += " ";
            result += firstName_[0];
            result += ".";
        }
        if (!middleName_.empty()) {
            result += middleName_[0];
            result += ".";
        }
        return result;
    }

    int GetBirthYear() const { return birthYear_; }

    // возраст на заданный год
    int GetAge(int year) const { return year - birthYear_; }
};