//
// Created by hemil on 02/07/21.
//

#ifndef STFU_EXPECT_H
#define STFU_EXPECT_H

/// Basically everything in this header needs to be public
/// So, we are not checking if STFU_IMPL is defined

#include <exception>
#include <string>
#include <utility>

/// The function stfu uses to convert your object into
/// string for debugging to display errors.
template<class T>
inline std::string stfu_debug_string(T custom_object) {
    return std::to_string(custom_object);
}

/// These are functions provided for users. They are obviously
/// not used by us
template<>
inline std::string stfu_debug_string(std::string custom_object) {
    return custom_object;
}

template<>
inline std::string stfu_debug_string(const char *custom_object) {
    return custom_object;
}

/// By default, std::to_string seems to print 0 for false and 1 for true
/// So overloaded that to print false and true
template<>
inline std::string stfu_debug_string(bool custom_object) {
    return custom_object ? "true" : "false";
}

namespace stfu {
    namespace impl {

        /// The exception thrown by expect when assertion fails
        struct AssertionFailed : public std::exception {
            /// Error needs to be a member variable initialized during
            /// construction because what needs a c str. If we declare
            /// error in what and call c_str, it will crash because
            /// by the time the pointer will be used, the string would
            /// be destroyed causing a crash
            const std::string expected, actual, error;

            AssertionFailed(std::string expected, std::string actual,
                            std::string file, int line)
                    : expected(std::move(expected)),
                    actual(std::move(actual)),
                    error("Assertion Failed.\n"
                            "Expected: " + this->expected + '\n' +
                            "Actual: " + this->actual + '\n'
                            + std::move(file) + ':' + std::to_string(line) + '\n') {}

            const char *what() const noexcept override {
                return error.c_str();
            }
        };

        /// The class the takes in an lhs and defines the comparison
        /// operators for a given type T. If comparison fails, it
        /// throws AssertionError otherwise does nothing
        template<class T>
        class Expression {
            T lhs;
            std::string expression, file;
            int line;
        public:
            Expression(T t, std::string actualExpression, std::string file, int line)
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
            void operator!=(U rhs) {
                if (lhs != rhs) {

                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " == " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            void operator<(U rhs) {
                if (lhs < rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " >= " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            void operator<=(U rhs) {
                if (lhs <= rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " > " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            void operator>(U rhs) {
                if (lhs > rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " <= " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            void operator>=(U rhs) {
                if (lhs < rhs) {
                } else {
                    throw AssertionFailed(expression, stfu_debug_string(lhs) + " < " + stfu_debug_string(rhs), file, line);
                }
            }
        };

        /// Specialization for bool values because we want to be able
        /// to use `expect(false)` or `expect(true)`. That is not possible
        /// with normal expression because comparison is not used in those
        /// cases
        template<>
        class Expression<bool> {
            bool lhs;
            std::string expression, file;
            int line;

            /// have we used == or !=
            bool used = false;
        public:
            Expression(bool t, std::string actualExpression, std::string file, int line)
                    : lhs(t), expression(std::move(actualExpression)), file(std::move(file)), line(line) {}

            /// Only defined == and != because thats all you can do with booleans


            template<class U>
            void operator==(U rhs) {
                used = true;
                if (lhs == rhs) {
                } else {
                    /// We could have implemented == in terms of != but that would
                    /// mean custom types would have to implement both
                    throw AssertionFailed("true", stfu_debug_string(lhs) + " != " + stfu_debug_string(rhs), file, line);
                }
            }

            template<class U>
            void operator!=(U rhs) {
                used = true;
                if (lhs != rhs) {
                } else {
                    throw AssertionFailed("true", stfu_debug_string(lhs) + " == " + stfu_debug_string(rhs), file, line);
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
            ~Expression() noexcept(false) {
                if (!used && !lhs) {
                    throw AssertionFailed("true", stfu_debug_string(lhs), file, line);
                }
            }
        };

        /// Captures left hand side of the expression
        /// along with file and line of expect for debugging
        struct CaptureLHSAndDebugInfo {
            std::string actualExpression, file;
            int line;

            explicit CaptureLHSAndDebugInfo(std::string actualText, std::string file, int line)
                    : actualExpression(std::move(actualText)), file(std::move(file)), line(line) {}

            template<class T>
            Expression<T> operator<<(T other) {
                return Expression<T>(other, actualExpression, file, line);
            }
        };

        inline std::ostream &operator<<(std::ostream &os, const CaptureLHSAndDebugInfo &e) {
            os << e.actualExpression << '\n';
            return os;
        }
    } /// namespace impl

#define expect(condition) (stfu::impl::CaptureLHSAndDebugInfo(#condition, __FILE__, __LINE__) << condition) // NOLINT(bugprone-macro-parentheses)
#define expectThrows(type, func) stfu::impl::expectThrowsFunc<type>(func, #type, __FILE__, __LINE__)

namespace impl {
    template <typename T>
    void expectThrowsFunc(const std::function<void()>& func, const char* stringified_type, const char* file, int line) {
            try {
                func();
                throw AssertionFailed(stringified_type, "no exception thrown", file, line);
            } catch (T& t) {

            } catch (AssertionFailed& e) {
                throw;
            }
            catch (std::exception& e) {
                throw AssertionFailed(stringified_type, e.what(), file, line);
            } catch (...) {
                throw AssertionFailed(stringified_type, "unknown", file, line);
            }
    }
}

} /// namespace stfu


#endif //STFU_EXPECT_H
