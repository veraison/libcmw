#pragma once

#include <optional>
#include <string>
#include <utility>

namespace cmw {

template <typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.value_ = std::move(value);
        return r;
    }

    static Result err(std::string message) {
        Result r;
        r.error_ = std::move(message);
        return r;
    }

    bool has_value() const { return value_.has_value(); }
    explicit operator bool() const { return has_value(); }

    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }
    T&& take_value() { return std::move(value_.value()); }

    const std::string& error() const { return error_; }

private:
    std::optional<T> value_;
    std::string error_;
};

template <>
class Result<void> {
public:
    static Result ok() { return Result{true, {}}; }
    static Result err(std::string message) { return Result{false, std::move(message)}; }

    bool has_value() const { return ok_; }
    explicit operator bool() const { return ok_; }
    const std::string& error() const { return error_; }

private:
    Result(bool ok, std::string error) : ok_(ok), error_(std::move(error)) {}

    bool ok_ = false;
    std::string error_;
};

}  // namespace cmw
