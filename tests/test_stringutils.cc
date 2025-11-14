#include <iostream>
#include <string>
#include <vector>
#include "coreqTk/stringutils.h"
#include "coreqTk/coreqint.h"

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

int test_natcmp() {
    // 0: equal, -1: a < b, 1: a > b
    ASSERT_EQ(natcmp("abc", "abc"), 0);
    ASSERT_EQ(natcmp("abc10", "abc2"), 1);
    ASSERT_EQ(natcmp("abc2", "abc10"), -1);
    ASSERT_EQ(natcmp("abc01", "abc1"), 1); // "Leading zeroes are ignored, but if numbers differ only by leading zeroes, the one with more leading zeroes is considered to be larger."
    ASSERT_EQ(natcmp("a1", "a01"), -1);
    return 0;
}

int test_split_string() {
    WordVec v = split_string("foo bar  baz", false, " ", true);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "bar");
    ASSERT_EQ(v[2], "baz");

    v = split_string("foo:bar:baz", false, ":", true);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "bar");
    ASSERT_EQ(v[2], "baz");

    v = split_string("foo::bar", false, ":", false);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "");
    ASSERT_EQ(v[2], "bar");

    return 0;
}

int test_unescape_string() {
    std::string s = "hello\\nworld";
    unescape_string(&s);
    ASSERT_EQ(s, "hello\nworld");

    s = "tab\\tchar";
    unescape_string(&s);
    ASSERT_EQ(s, "tab\tchar");

    s = "escaped\\\\backslash";
    unescape_string(&s);
    ASSERT_EQ(s, "escaped\\backslash");

    return 0;
}

int main() {
    int failed = 0;
    if (test_trim() != 0) failed++;
    if (test_to_lower() != 0) failed++;
    if (test_is_numeric() != 0) failed++;
    if (test_natcmp() != 0) failed++;
    if (test_split_string() != 0) failed++;
    if (test_unescape_string() != 0) failed++;
    
    if (failed == 0) {
        std::cout << "All stringutils tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
