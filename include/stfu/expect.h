//
// Created by hemil on 02/07/21.
//

#ifndef STFU_EXPECT_H
#define STFU_EXPECT_H

#include <exception>
#include <string>
#include <utility>

template<class T>
std::string stfu_debug_string(T custom_object) {
    return std::to_string(custom_object);
}
/// These are functions provided for users. They are obviously
/// not used by us
template<>
std::string stfu_debug_string(std::string custom_object) {
    return custom_object;
}

template<>
std::string stfu_debug_string(const char *custom_object) {
    return custom_object;
}

/// By default, std::to_string seems to print 0 for false and 1 for true
/// So overloaded that to print false and true
template<>
std::string stfu_debug_string(bool custom_object) {
    return custom_object ? "true" : "false";
}

namespace stfu {
    namespace impl {
        class AssertionFailed : std::exception {
            const std::string error;
        public:
            AssertionFailed(const std::string &expectedExpression, const std::string &actual,
                            const std::string& file, int line)
                    : error("Assertion Failed. Expression: " + expectedExpression +
                            "\nActual: " + actual + '\n'
                            + file + ':' + std::to_string(line) + '\n') {}

            const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_USE_NOEXCEPT override {
                return error.c_str();
            }
        };

        template<class T>
        class ExpressionComparison {
            T lhs;
            std::string expression, file;
            int line;
        public:
            ExpressionComparison(T t, std::string actualExpression, std::string file, int line)
                    : lhs(t), expression(std::move(actualExpression)), file(std::move(file)), line(line) {}

            template<class U>
            void operator==(U rhs) {
                if (lhs == rhs) {
                } else {
                    /// We could have implemented == in terms of != but that would
                    /// mean custom types would have to implement both
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " != " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator!=(U rhs) {
                if (lhs != rhs) {

                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " == " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator<(U rhs) {
                if (lhs < rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " >= " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator<=(U rhs) {
                if (lhs <= rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " > " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator>(U rhs) {
                if (lhs > rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " <= " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator>=(U rhs) {
                if (lhs < rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " < " + stfu_debug_string(rhs), file, line);
                }
            }
        };

        template<>
        class ExpressionComparison<bool> {
            bool lhs;
            std::string expression, file;
            int line;

            bool used = false;
        public:
            ExpressionComparison(bool t, std::string actualExpression, std::string file, int line)
                    : lhs(t), expression(std::move(actualExpression)), file(std::move(file)), line(line) {}

            template<class U>
            void operator==(U rhs) {
                used = true;
                if (lhs == rhs) {
                } else {
                    /// We could have implemented == in terms of != but that would
                    /// mean custom types would have to implement both
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " != " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            bool operator!=(U rhs) {
                used = true;
                if (lhs != rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " == " + stfu_debug_string(rhs), file, line);
                }
            }


            /// Special case. What happens when we write expect(false).
            /// In that case, none of the operators will be called.
            ///
            /// I am taking advantage of the fact that expect is written
            /// in such a way that the object will be destroyed at the end
            /// of the line. Because that's the lifetime of temporaries.
            ///
            /// I am checking if the operators were used. If not, i am checking
            /// if the bool value of the object got as parameter is false.
            /// If so, throw AssertionFailed with appropriate values
            ///
            /// We need the noexcept(false) because in C++11,
            /// destructors are noexcept by default
            ~ExpressionComparison() noexcept(false) {
                if (!used && !lhs) {
                    throw AssertionFailed(expression, stfu_debug_string(lhs), file, line);
                }
            }
        };

        struct ExpressionWrapper {
            std::string actualExpression, file;
            int line;

            explicit ExpressionWrapper(std::string actualText, std::string file, int line)
                    : actualExpression(std::move(actualText)), file(std::move(file)), line(line) {}

            template<class T>
            ExpressionComparison<T> operator<<(T other) {
                return ExpressionComparison<T>(other, actualExpression, file, line);
            }
        };

        std::ostream &operator<<(std::ostream &os, const ExpressionWrapper &e) {
            os << e.actualExpression << '\n';
            return os;
        }
    } /// namespace impl
} /// namespace stfu

#define expect(condition) (stfu::impl::ExpressionWrapper(#condition, __FILE__, __LINE__) << condition) // NOLINT(bugprone-macro-parentheses)
#endif //STFU_EXPECT_H
