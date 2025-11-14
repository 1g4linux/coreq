#include <iostream>
#include <string>
#include <vector>
#include "corepkg/basicversion.h"

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

int test_version_compare() {
    BasicVersion v1, v2;
    std::string err;
    
    ASSERT_EQ(v1.parseVersion("1.2.3", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.4", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 < v2);
    ASSERT_TRUE(v2 > v1);
    ASSERT_TRUE(v1 != v2);

    ASSERT_EQ(v1.parseVersion("1.2.3_rc1", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.3", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 < v2); // rc is usually less than final

    ASSERT_EQ(v1.parseVersion("1.2.3_alpha1", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.3_beta1", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 < v2);

    ASSERT_EQ(v1.parseVersion("1.2.3_beta2", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.3_rc1", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 < v2);

    ASSERT_EQ(v1.parseVersion("1.2.3_pre1", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.3_rc1", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 < v2);

    ASSERT_EQ(v1.parseVersion("1.2.3-r1", &err), BasicVersion::parsedOK);
    ASSERT_EQ(v2.parseVersion("1.2.3", &err), BasicVersion::parsedOK);
    ASSERT_TRUE(v1 > v2);

    return 0;
}

int main() {
    int failed = 0;
    if (test_version_compare() != 0) failed++;
    
    if (failed == 0) {
        std::cout << "All version tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
