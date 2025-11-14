#include <iostream>
#include <string>
#include <vector>
#include "corepkg/package.h"
#include "corepkg/version.h"

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

int test_package_basic() {
    Package p("sys-apps", "coreutils");
    ASSERT_EQ(p.category, "sys-apps");
    ASSERT_EQ(p.name, "coreutils");
    
    Version* v1 = new Version();
    std::string err;
    v1->parseVersion("8.32", &err);
    v1->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
    v1->set_slotname("");
    v1->overlay_key = 1;
    p.addVersion(v1);
    
    ASSERT_EQ(p.size(), 1u);
    ASSERT_EQ(p.latest(), v1);

    Version* v2 = new Version();
    v2->parseVersion("8.33", &err);
    v2->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
    v2->set_slotname("");
    v2->overlay_key = 1;
    p.addVersion(v2);
    ASSERT_EQ(p.size(), 2u);
    ASSERT_EQ(p.latest(), v2);

    Version* best = p.best_slot("");
    ASSERT_EQ(best, v2);

    Version* v3 = new Version();
    v3->parseVersion("9.0", &err);
    v3->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
    v3->set_slotname("9");
    v3->overlay_key = 1;
    p.addVersion(v3);
    
    ASSERT_EQ(p.best_slot("9"), v3);
    ASSERT_EQ(p.best_slot(""), v2);
    
    return 0;
}

int main() {
    int failed = 0;
    if (test_package_basic() != 0) failed++;
    
    if (failed == 0) {
        std::cout << "All package tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
