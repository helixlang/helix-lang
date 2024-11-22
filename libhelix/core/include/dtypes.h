///--- The Helix Project ------------------------------------------------------------------------///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
///------------------------------------------------------------------------------------ Helix ---///

#ifndef __$LIBHELIX_DTYPES__
#define __$LIBHELIX_DTYPES__

#include <cstddef>
#include <array>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <exception>

#include "config.h"
H_NAMESPACE_BEGIN

using byte   = libcxx::byte;
using string = libcxx::string;

template <typename _Ty, std::size_t _Size>
using array  = libcxx::array<_Ty, _Size>;
template <typename... Args>
using tuple  = libcxx::tuple<Args...>;
template <typename _Ty>
using list   = libcxx::vector<_Ty>;
template <typename... Args>
using set    = libcxx::set<Args...>;
template <typename... Args>
using map    = libcxx::map<Args...>;

using $int = int;

H_NAMESPACE_END
#endif


// template <size_t _Size = 1024>
// class $int {
// private:
//     static constexpr size_t MAX_DIGITS = _Size;
//     bool isNegative = false;
//     uint8_t digits[MAX_DIGITS] = {0};
//     size_t digitCount = 1;

//     template <typename T>
//     constexpr $int fromNumeric(T value) const;
//     constexpr void multiplyByBase(uint8_t base);
//     constexpr void addDigit(uint8_t value);
//     constexpr void karatsubaMultiply(const $int& other, $int& result) const;
//     constexpr void fastAdd(const $int& other, $int& result) const;
//     constexpr void fastSubtract(const $int& other, $int& result) const;
    
// public:
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int(T value);
//     constexpr $int() = default;
//     constexpr explicit $int(const char* numStr);
//     constexpr void parse(const char* numStr);
//     constexpr bool operator==(const $int& other) const;
//     constexpr bool operator!=(const $int& other) const;
//     constexpr bool operator<(const $int& other) const;
//     constexpr bool operator<=(const $int& other) const;
//     constexpr bool operator>(const $int& other) const;
//     constexpr bool operator>=(const $int& other) const;
//     constexpr $int operator+(const $int& other) const;
//     constexpr $int operator-(const $int& other) const;
//     constexpr $int operator*(const $int& other) const;
//     constexpr $int operator/(const $int& other) const;
//     constexpr $int operator%(const $int& other) const;

//     // Overloaded operators for compatibility with all numeric types
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int operator+(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int operator-(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int operator*(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int operator/(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int operator%(T value) const;

    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int& operator+=(T value) { *this = *this + value; return *this; }
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int& operator-=(T value) { *this = *this - value; return *this; }
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int& operator*=(T value) { *this = *this * value; return *this; }
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int& operator/=(T value) { *this = *this / value; return *this; }
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr $int& operator%=(T value) { *this = *this % value; return *this; }

    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator==(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator!=(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator<(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator<=(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator>(T value) const;
    
//     template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
//     constexpr bool operator>=(T value) const;
// };

// $int<> operator""i(const char *numStr) { return $int<>(numStr); }

// constexpr void $int<>::multiplyByBase(uint8_t base) {
//     uint16_t carry = 0;
//     for (size_t i = 0; i < digitCount; ++i) {
//         uint16_t result = digits[i] * base + carry;
//         digits[i] = result % 10;
//         carry = result / 10;
//     }
//     while (carry != 0 && digitCount < MAX_DIGITS) {
//         digits[digitCount++] = carry % 10;
//         carry /= 10;
//     }
//     if (carry != 0) {
//         throw std::overflow_error("Exceeded maximum digit storage");
//     }
// }

// constexpr void $int<>::addDigit(uint8_t value) {
//     uint16_t carry = value;
//     for (size_t i = 0; i < digitCount; ++i) {
//         uint16_t result = digits[i] + carry;
//         digits[i] = result % 10;
//         carry = result / 10;
//         if (carry == 0) {
//             return;
//         }
//     }
//     while (carry != 0 && digitCount < MAX_DIGITS) {
//         digits[digitCount++] = carry % 10;
//         carry /= 10;
//     }
//     if (carry != 0) {
//         throw std::overflow_error("Exceeded maximum digit storage");
//     }
// }

// constexpr void $int<>::karatsubaMultiply(const $int& other, $int& result) const {
//     // Implement fast multiplication using Karatsuba algorithm
//     if (digitCount == 1 || other.digitCount == 1) {
//         uint16_t carry = 0;
//         for (size_t i = 0; i < other.digitCount; ++i) {
//             for (size_t j = 0; j < digitCount; ++j) {
//                 uint16_t res = digits[j] * other.digits[i] + result.digits[i + j] + carry;
//                 result.digits[i + j] = res % 10;
//                 carry = res / 10;
//             }
//             if (carry != 0) {
//                 result.digits[i + digitCount] = carry;
//                 carry = 0;
//             }
//         }
//         result.digitCount = digitCount + other.digitCount;
//         while (result.digitCount > 1 && result.digits[result.digitCount - 1] == 0) {
//             --result.digitCount;
//         }
//         return;
//     }
//     // Placeholder for recursive multiplication.
// }

// constexpr void $int<>::fastAdd(const $int& other, $int& result) const {
//     size_t maxDigits = std::max(digitCount, other.digitCount);
//     uint16_t carry = 0;
//     for (size_t i = 0; i < maxDigits; ++i) {
//         uint16_t sum = digits[i] + other.digits[i] + carry;
//         result.digits[i] = sum % 10;
//         carry = sum / 10;
//     }
//     result.digitCount = maxDigits;
//     if (carry != 0) {
//         result.digits[result.digitCount++] = carry;
//     }
// }

// constexpr void $int<>::fastSubtract(const $int& other, $int& result) const {
//     if (*this < other) {
//         throw std::invalid_argument("Subtraction result would be negative");
//     }
//     uint16_t borrow = 0;
//     for (size_t i = 0; i < digitCount; ++i) {
//         int16_t sub = digits[i] - (i < other.digitCount ? other.digits[i] : 0) - borrow;
//         if (sub < 0) {
//             sub += 10;
//             borrow = 1;
//         } else {
//             borrow = 0;
//         }
//         result.digits[i] = sub;
//     }
//     result.digitCount = digitCount;
//     while (result.digitCount > 1 && result.digits[result.digitCount - 1] == 0) {
//         --result.digitCount;
//     }
// }

// template <typename T>
// constexpr $int $int<>::fromNumeric(T value) const {
//     static_assert(std::is_arithmetic_v<T>, "Template parameter must be a numeric type");
//     $int result;
//     if constexpr (std::is_signed_v<T>) {
//         if (value < 0) {
//             result.isNegative = true;
//             value = -value;
//         }
//     }
//     result.digitCount = 0;
//     do {
//         result.digits[result.digitCount++] = value % 10;
//         value /= 10;
//     } while (value != 0);
//     return result;
// }

// constexpr $int<>::$int(const char* numStr) {
//     parse(numStr);
// }

// template <typename T, typename>
// constexpr $int<>::$int(T value) {
//     *this = fromNumeric(value);
// }

// constexpr void $int<>::parse(const char* numStr) {
//     std::memset(digits, 0, sizeof(digits));
//     digitCount = 1;
//     isNegative = false;

//     size_t start = 0;
//     int base = 10;

//     // Handle sign
//     if (numStr[start] == '-') {
//         isNegative = true;
//         start++;
//     } else if (numStr[start] == '+') {
//         start++;
//     }

//     // Detect base
//     if (numStr[start] == '0') {
//         if (numStr[start + 1] == 'x' || numStr[start + 1] == 'X') {
//             base = 16;
//             start += 2;
//         } else if (numStr[start + 1] == 'b' || numStr[start + 1] == 'B') {
//             base = 2;
//             start += 2;
//         } else if (std::isdigit(numStr[start + 1])) {
//             base = 8;
//             start += 1;
//         }
//     }

//     // Parse digits
//     for (size_t i = start; numStr[i] != '\0'; ++i) {
//         char c = numStr[i];
//         uint8_t value = 0;

//         if (c >= '0' && c <= '9') {
//             value = c - '0';
//         } else if (c >= 'a' && c <= 'f') {
//             value = c - 'a' + 10;
//         } else if (c >= 'A' && c <= 'F') {
//             value = c - 'A' + 10;
//         } else {
//             throw std::invalid_argument("Invalid character in numeric string");
//         }

//         if (value >= base) {
//             throw std::invalid_argument("Digit exceeds base");
//         }

//         multiplyByBase(base);
//         addDigit(value);
//     }

//     // Remove leading zeros
//     while (digitCount > 1 && digits[digitCount - 1] == 0) {
//         --digitCount;
//     }
// }

// constexpr bool $int<>::operator==(const $int& other) const {
//     if (isNegative != other.isNegative || digitCount != other.digitCount) {
//         return false;
//     }
//     for (size_t i = 0; i < digitCount; ++i) {
//         if (digits[i] != other.digits[i]) {
//             return false;
//         }
//     }
//     return true;
// }

// constexpr bool $int<>::operator!=(const $int& other) const {
//     return !(*this == other);
// }

// constexpr bool $int<>::operator<(const $int& other) const {
//     if (isNegative != other.isNegative) {
//         return isNegative;
//     }
//     if (digitCount != other.digitCount) {
//         return isNegative ? digitCount > other.digitCount : digitCount < other.digitCount;
//     }
//     for (size_t i = 0; i < digitCount; ++i) {
//         if (digits[digitCount - 1 - i] != other.digits[digitCount - 1 - i]) {
//             return isNegative ? digits[digitCount - 1 - i] > other.digits[digitCount - 1 - i]
//                                 : digits[digitCount - 1 - i] < other.digits[digitCount - 1 - i];
//         }
//     }
//     return false;
// }

// constexpr bool $int<>::operator<=(const $int& other) const {
//     return *this < other || *this == other;
// }

// constexpr bool $int<>::operator>(const $int& other) const {
//     return !(*this <= other);
// }

// constexpr bool $int<>::operator>=(const $int& other) const {
//     return !(*this < other);
// }

// constexpr $int $int<>::operator+(const $int& other) const {
//     $int result;
//     fastAdd(other, result);
//     return result;
// }

// constexpr $int $int<>::operator-(const $int& other) const {
//     $int result;
//     fastSubtract(other, result);
//     return result;
// }

// constexpr $int $int<>::operator*(const $int& other) const {
//     $int result;
//     karatsubaMultiply(other, result);
//     return result;
// }

// constexpr $int $int<>::operator/(const $int& other) const {
//     // Implement fast division algorithm using long division
//     if (other == $int("0")) {
//         throw std::domain_error("Division by zero");
//     }
//     $int result;
//     $int remainder;
//     for (size_t i = digitCount; i > 0; --i) {
//         remainder = remainder * 10 + digits[i - 1];
//         uint8_t quotient_digit = 0;
//         while (remainder >= other) {
//             remainder = remainder - other;
//             ++quotient_digit;
//         }
//         result.digits[i - 1] = quotient_digit;
//     }
//     result.digitCount = digitCount;
//     while (result.digitCount > 1 && result.digits[result.digitCount - 1] == 0) {
//         --result.digitCount;
//     }
//     return result;
// }

// constexpr $int $int<>::operator%(const $int& other) const {
//     // Implement fast modulo algorithm
//     if (other == $int("0")) {
//         throw std::domain_error("Modulo by zero");
//     }
//     $int remainder;
//     for (size_t i = digitCount; i > 0; --i) {
//         remainder = remainder * 10 + digits[i - 1];
//         while (remainder >= other) {
//             remainder = remainder - other;
//         }
//     }
//     return remainder;
// }

// // Overloaded operators for compatibility with all numeric types
// template <typename T, typename>
// constexpr $int $int<>::operator+(T value) const { return *this + fromNumeric(value); }
// template <typename T, typename>
// constexpr $int $int<>::operator-(T value) const { return *this - fromNumeric(value); }
// template <typename T, typename>
// constexpr $int $int<>::operator*(T value) const { return *this * fromNumeric(value); }
// template <typename T, typename>
// constexpr $int $int<>::operator/(T value) const { return *this / fromNumeric(value); }
// template <typename T, typename>
// constexpr $int $int<>::operator%(T value) const { return *this % fromNumeric(value); }

// // template <typename T, typename>
// // constexpr $int& $int<>::operator+=(T value) { *this = *this + value; return *this; }
// // template <typename T, typename>
// // constexpr $int& $int<>::operator-=(T value) { *this = *this - value; return *this; }
// // template <typename T, typename>
// // constexpr $int& $int<>::operator*=(T value) { *this = *this * value; return *this; }
// // template <typename T, typename>
// // constexpr $int& $int<>::operator/=(T value) { *this = *this / value; return *this; }
// // template <typename T, typename>
// // constexpr $int& $int<>::operator%=(T value) { *this = *this % value; return *this; }

// template <typename T, typename>
// constexpr bool $int<>::operator==(T value) const { return *this == fromNumeric(value); }
// template <typename T, typename>
// constexpr bool $int<>::operator!=(T value) const { return *this != fromNumeric(value); }
// template <typename T, typename>
// constexpr bool $int<>::operator<(T value) const { return *this < fromNumeric(value); }
// template <typename T, typename>
// constexpr bool $int<>::operator<=(T value) const { return *this <= fromNumeric(value); }
// template <typename T, typename>
// constexpr bool $int<>::operator>(T value) const { return *this > fromNumeric(value); }
// template <typename T, typename>
// constexpr bool $int<>::operator>=(T value) const { return *this >= fromNumeric(value); }
