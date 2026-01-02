#include <iostream>
#include <string>

#include "coreqTk/outputstring.h"
#include "coreqTk/stringlist.h"

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

static std::string to_printed(const OutputString& s) {
    std::string out;
    WordSize width = 0;
    s.print(&out, &width);
    return out;
}

int test_finalize_and_join() {
    StringList l;
    l.push_back("");
    l.push_back("a");
    l.push_back("");
    l.push_back("b");
    l.push_back("");
    l.finalize();

    const WordVec* vec = l.asWordVecPtr();
    ASSERT_TRUE(vec != NULL);
    ASSERT_EQ(vec->size(), 3u);
    ASSERT_EQ((*vec)[0], "a");
    ASSERT_EQ((*vec)[1], "");
    ASSERT_EQ((*vec)[2], "b");

    OutputString out;
    l.append_to_string(&out, OutputString(","));
    ASSERT_EQ(to_printed(out), "a,,b");
    return 0;
}

int test_all_empty_becomes_empty() {
    StringList l;
    l.push_back("");
    l.push_back("");
    l.finalize();
    ASSERT_TRUE(l.empty());
    ASSERT_TRUE(l.asWordVecPtr() == NULL);
    return 0;
}

int test_copy_shares_content() {
    StringList a;
    a.push_back("x");
    StringList b = a;
    a.push_back("y");

    const WordVec* avec = a.asWordVecPtr();
    const WordVec* bvec = b.asWordVecPtr();
    ASSERT_TRUE(avec != NULL);
    ASSERT_TRUE(bvec != NULL);
    ASSERT_EQ(avec->size(), 2u);
    ASSERT_EQ(bvec->size(), 2u);
    ASSERT_EQ((*bvec)[1], "y");
    ASSERT_TRUE(a == b);
    return 0;
}

int main() {
    int failed = 0;
    if (test_finalize_and_join() != 0) failed++;
    if (test_all_empty_becomes_empty() != 0) failed++;
    if (test_copy_shares_content() != 0) failed++;

    if (failed == 0) {
        std::cout << "All stringlist tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
