#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "coreqTk/regexp.h"

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

int test_regex_basic() {
    Regex r;
    std::string::size_type b = 99;
    std::string::size_type e = 99;
    ASSERT_TRUE(r.match("anything"));
    ASSERT_TRUE(r.match("anything", &b, &e));
    ASSERT_EQ(b, 0u);
    ASSERT_EQ(e, std::string::npos);

    r.compile("^ab+c$");
    ASSERT_TRUE(r.compiled());
    ASSERT_TRUE(r.match("abbbc"));
    ASSERT_TRUE(!r.match("ac"));
    ASSERT_TRUE(r.match("abbbc", &b, &e));
    ASSERT_EQ(b, 0u);
    ASSERT_EQ(e, 5u);

    ASSERT_TRUE(!r.match("xyz", &b, &e));
    ASSERT_EQ(b, std::string::npos);
    ASSERT_EQ(e, std::string::npos);

    r.clear();
    ASSERT_TRUE(!r.compiled());
    ASSERT_TRUE(r.match("xyz"));
    return 0;
}

int test_regex_list() {
    RegexList l("^foo$ ^bar$");
    ASSERT_TRUE(l.match("foo"));
    ASSERT_TRUE(l.match("bar"));
    ASSERT_TRUE(!l.match("baz"));
    return 0;
}

int test_invalid_regex_exits() {
    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid == 0) {
        Regex r;
        r.compile("[");
        _exit(0);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return 1;
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_TRUE(WEXITSTATUS(status) != 0);
    return 0;
}

int main() {
    int failed = 0;
    if (test_regex_basic() != 0) failed++;
    if (test_regex_list() != 0) failed++;
    if (test_invalid_regex_exits() != 0) failed++;

    if (failed == 0) {
        std::cout << "All regexp tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
