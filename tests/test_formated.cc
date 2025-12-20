#include <cstdio>

#include <iostream>
#include <string>

#include "coreqTk/formated.h"

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

static std::string read_all(FILE* fp) {
    if (fflush(fp) != 0) return "";
    if (fseek(fp, 0, SEEK_SET) != 0) return "";

    std::string out;
    char buf[128];
    while (true) {
        const std::size_t n = fread(buf, 1, sizeof(buf), fp);
        out.append(buf, n);
        if (n < sizeof(buf)) break;
    }
    return out;
}

int test_string_formatting() {
    ASSERT_EQ(static_cast<std::string>(coreq::format("A%sB") % "x"), "AxB");
    ASSERT_EQ(static_cast<std::string>(coreq::format("%2$s/%1$d") % 7 % "pkg"), "pkg/7");
    ASSERT_EQ(static_cast<std::string>(coreq::format("x%%y")), "x%y");
    ASSERT_EQ(static_cast<std::string>(coreq::format(true) % "line"), "line\n");

    std::string::size_type pos = std::string::npos;
    ASSERT_EQ(static_cast<std::string>(coreq::format("%d") % pos), "<string::npos>");
    return 0;
}

int test_stream_output() {
    FILE* fp = tmpfile();
    if (fp == NULL) return 1;

    {
        coreq::format out(fp, "v=%s", true, false);
        out % "ok";
    }
    ASSERT_EQ(read_all(fp), "v=ok\n");

    if (fclose(fp) != 0) return 1;
    return 0;
}

int main() {
    int failed = 0;
    if (test_string_formatting() != 0) failed++;
    if (test_stream_output() != 0) failed++;

    if (failed == 0) {
        std::cout << "All formated tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
