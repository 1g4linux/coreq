#include <iostream>
#include <string>

#include "coreqTk/outputstring.h"

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

int test_basic_and_columns() {
    OutputString out("ab");
    out.append_column(5);
    out.append_smart('x');

    std::string printed;
    WordSize width = 0;
    out.print(&printed, &width);

    ASSERT_EQ(printed, "ab   x");
    ASSERT_EQ(width, 6u);
    return 0;
}

int test_absolute_mode() {
    OutputString out;
    out.append_smart('a');
    out.append_smart('\n');
    out.append_smart('b');
    out.append_column(4);
    out.append_smart('c');

    std::string printed;
    WordSize width = 0;
    out.print(&printed, &width);

    ASSERT_EQ(printed, "a\nb   c");
    ASSERT_EQ(width, 5u);
    return 0;
}

int test_append_escape() {
    OutputString out("ab");

    const char* esc = "\\C<6>";
    const char* pos = esc;
    out.append_escape(&pos);
    ASSERT_TRUE(*pos == '>');

    out.append_smart('x');
    std::string printed;
    WordSize width = 0;
    out.print(&printed, &width);
    ASSERT_EQ(printed, "ab    x");
    ASSERT_EQ(width, 7u);

    const char* esc2 = "\\n";
    const char* pos2 = esc2;
    OutputString out2("A");
    out2.append_escape(&pos2);
    out2.append_smart('B');
    printed.clear();
    width = 0;
    out2.print(&printed, &width);
    ASSERT_EQ(printed, "A\nB");
    ASSERT_EQ(width, 1u);
    return 0;
}

int test_append_outputstring() {
    OutputString base("foo");
    OutputString rhs;
    rhs.append_smart('\n');
    rhs.append_smart('x');

    base.append(rhs);

    std::string printed;
    WordSize width = 99;
    base.print(&printed, &width);
    ASSERT_EQ(printed, "foo\nx");
    ASSERT_EQ(width, 1u);
    return 0;
}

int main() {
    int failed = 0;
    if (test_basic_and_columns() != 0) failed++;
    if (test_absolute_mode() != 0) failed++;
    if (test_append_escape() != 0) failed++;
    if (test_append_outputstring() != 0) failed++;

    if (failed == 0) {
        std::cout << "All outputstring tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
