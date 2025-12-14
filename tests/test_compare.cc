#include <iostream>
#include <string>

#include "coreqTk/compare.h"

#define ASSERT_EQ(val1, val2) \
    if (!((val1) == (val2))) { \
        std::cerr << "Assertion failed: " << #val1 << " == " << #val2 \
                  << " (" << (val1) << " != " << (val2) << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

int test_default_compare() {
    ASSERT_EQ(coreq::default_compare(1, 1), 0);
    ASSERT_EQ(coreq::default_compare(1, 2), -1);
    ASSERT_EQ(coreq::default_compare(2, 1), 1);

    const std::string a("alpha");
    const std::string b("beta");
    ASSERT_EQ(coreq::default_compare(a, a), 0);
    ASSERT_EQ(coreq::default_compare(a, b), -1);
    ASSERT_EQ(coreq::default_compare(b, a), 1);
    return 0;
}

int test_numeric_compare() {
    ASSERT_EQ(coreq::numeric_compare("", ""), 0);
    ASSERT_EQ(coreq::numeric_compare("", "0"), 0);
    ASSERT_EQ(coreq::numeric_compare("0", ""), 0);
    ASSERT_EQ(coreq::numeric_compare("0", "000"), 0);
    ASSERT_EQ(coreq::numeric_compare("0001", "1"), 0);
    ASSERT_EQ(coreq::numeric_compare("0010", "9"), 1);
    ASSERT_EQ(coreq::numeric_compare("9", "0010"), -1);
    ASSERT_EQ(coreq::numeric_compare("123", "45"), 1);
    ASSERT_EQ(coreq::numeric_compare("45", "123"), -1);
    ASSERT_EQ(coreq::numeric_compare("2147483648", "2147483647"), 1);
    ASSERT_EQ(coreq::numeric_compare("18446744073709551616", "18446744073709551615"), 1);
    ASSERT_EQ(coreq::numeric_compare("100", "099"), 1);
    ASSERT_EQ(coreq::numeric_compare("0099", "100"), -1);
    return 0;
}

int main() {
    int failed = 0;
    if (test_default_compare() != 0) failed++;
    if (test_numeric_compare() != 0) failed++;

    if (failed == 0) {
        std::cout << "All compare tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
