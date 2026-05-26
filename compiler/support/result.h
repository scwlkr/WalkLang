#pragma once

#include <optional>
#include <string>
#include <utility>

namespace walk {

template <typename T>
class Result {
public:
    static Result success(T value) {
        Result result;
        result.value_ = std::move(value);
        return result;
    }

    static Result failure(std::string message) {
        Result result;
        result.error_ = std::move(message);
        return result;
    }

    [[nodiscard]] bool ok() const {
        return value_.has_value();
    }

    [[nodiscard]] const T& value() const {
        return *value_;
    }

    [[nodiscard]] T& value() {
        return *value_;
    }

    [[nodiscard]] T take_value() {
        T value = std::move(*value_);
        value_.reset();
        return value;
    }

    [[nodiscard]] const std::string& error() const {
        return error_;
    }

private:
    std::optional<T> value_;
    std::string error_;
};

template <>
class Result<void> {
public:
    static Result success() {
        return Result(true, "");
    }

    static Result failure(std::string message) {
        return Result(false, std::move(message));
    }

    [[nodiscard]] bool ok() const {
        return ok_;
    }

    [[nodiscard]] const std::string& error() const {
        return error_;
    }

private:
    Result(bool ok, std::string error) : ok_(ok), error_(std::move(error)) {}

    bool ok_ = false;
    std::string error_;
};

}  // namespace walk
