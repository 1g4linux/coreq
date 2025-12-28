#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>

#include "coreqTk/parseerror.h"

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

static std::string capture_output_start(int* old_stderr_fd, FILE** tmp) {
    *tmp = tmpfile();
    if (*tmp == NULL) return "";
    fflush(stderr);
    *old_stderr_fd = dup(fileno(stderr));
    if (*old_stderr_fd < 0) return "";
    if (dup2(fileno(*tmp), fileno(stderr)) < 0) return "";
    return "ok";
}

static std::string capture_output_stop(int old_stderr_fd, FILE* tmp) {
    fflush(stderr);
    if (dup2(old_stderr_fd, fileno(stderr)) < 0) return "";
    close(old_stderr_fd);
    if (fseek(tmp, 0, SEEK_SET) != 0) return "";

    std::string out;
    char buf[256];
    while (true) {
        const std::size_t n = fread(buf, 1, sizeof(buf), tmp);
        out.append(buf, n);
        if (n < sizeof(buf)) break;
    }
    fclose(tmp);
    return out;
}

static std::size_t count_substring(const std::string& haystack, const std::string& needle) {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

int test_output_format_and_dedup() {
    ParseError pe(false);

    int old_fd = -1;
    FILE* tmp = NULL;
    ASSERT_TRUE(capture_output_start(&old_fd, &tmp) == "ok");

    pe.output("parseerror-format-1", 3, "badline", "first line\nsecond line");
    pe.output("parseerror-format-1", 3, "badline", "first line\nsecond line");

    const std::string out = capture_output_stop(old_fd, tmp);
    ASSERT_TRUE(out.find("invalid line 3") != std::string::npos);
    ASSERT_TRUE(out.find("parseerror-format-1") != std::string::npos);
    ASSERT_TRUE(out.find("\"badline\"") != std::string::npos);
    ASSERT_TRUE(out.find("    first line") != std::string::npos);
    ASSERT_TRUE(out.find("    second line") != std::string::npos);
    ASSERT_TRUE(count_substring(out, "invalid line 3") == 1u);
    return 0;
}

int test_tacit_mode() {
    ParseError pe(true);
    int old_fd = -1;
    FILE* tmp = NULL;
    ASSERT_TRUE(capture_output_start(&old_fd, &tmp) == "ok");

    pe.output("parseerror-tacit-1", 9, "line", "err");
    const std::string out = capture_output_stop(old_fd, tmp);
    ASSERT_TRUE(out.empty());
    return 0;
}

int test_iterator_overload() {
    ParseError pe(false);
    LineVec lines;
    lines.push_back("line1");
    lines.push_back("line2");
    lines.push_back("line3");

    int old_fd = -1;
    FILE* tmp = NULL;
    ASSERT_TRUE(capture_output_start(&old_fd, &tmp) == "ok");

    pe.output("parseerror-iter-1", lines.begin(), lines.begin() + 1, "bad token");
    const std::string out = capture_output_stop(old_fd, tmp);
    ASSERT_TRUE(out.find("invalid line 2") != std::string::npos);
    ASSERT_TRUE(out.find("\"line2\"") != std::string::npos);
    return 0;
}

int main() {
    int failed = 0;
    if (test_output_format_and_dedup() != 0) failed++;
    if (test_tacit_mode() != 0) failed++;
    if (test_iterator_overload() != 0) failed++;

    if (failed == 0) {
        std::cout << "All parseerror tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
