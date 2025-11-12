#include <iostream>
#include <string>
#include <vector>
#include "coreqTk/stringutils.h"

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

#define ASSERT_EQ(val1, val2) \
    if (!((val1) == (val2))) { \
        std::cerr << "Assertion failed: " << #val1 << " == " << #val2 \
                  << " (" << (val1) << " != " << (val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

int test_trim() {
    std::string s = "  hello  ";
    trim(&s);
    ASSERT_EQ(s, "hello");
    
    s = " \t\n hello \r ";
    trim(&s);
    ASSERT_EQ(s, "hello");
    return 0;
}

int test_to_lower() {
    ASSERT_EQ(to_lower("Hello WORLD"), "hello world");
    ASSERT_EQ(to_lower("123!@#"), "123!@#");
    return 0;
}

int test_is_numeric() {
    ASSERT_TRUE(is_numeric("12345"));
    ASSERT_TRUE(!is_numeric("123a45"));
    ASSERT_TRUE(is_numeric(""));
    return 0;
}

int main() {
    int failed = 0;
    if (test_trim() != 0) failed++;
    if (test_to_lower() != 0) failed++;
    if (test_is_numeric() != 0) failed++;
    
    if (failed == 0) {
        std::cout << "All stringutils tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
