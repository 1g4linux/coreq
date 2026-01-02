#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>

#include "coreqTk/percentage.h"

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

static std::string capture_stdout_start(int* old_stdout_fd, FILE** tmp) {
    *tmp = tmpfile();
    if (*tmp == NULL) return "";
    fflush(stdout);
    *old_stdout_fd = dup(fileno(stdout));
    if (*old_stdout_fd < 0) return "";
    if (dup2(fileno(*tmp), fileno(stdout)) < 0) return "";
    return "ok";
}

static std::string capture_stdout_stop(int old_stdout_fd, FILE* tmp) {
    fflush(stdout);
    if (dup2(old_stdout_fd, fileno(stdout)) < 0) return "";
    close(old_stdout_fd);
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

int test_verbose_progress() {
    int old_fd = -1;
    FILE* tmp = NULL;
    ASSERT_TRUE(capture_stdout_start(&old_fd, &tmp) == "ok");

    PercentStatus p;
    p.init("%s/%s %s%%", 4);
    p.next(" a");
    p.next(" b");
    p.finish("done");

    const std::string out = capture_stdout_stop(old_fd, tmp);
    ASSERT_TRUE(out.find("0/4   0%") != std::string::npos);
    ASSERT_TRUE(out.find("\r1/4  25% a") != std::string::npos);
    ASSERT_TRUE(out.find("\r2/4  50% b") != std::string::npos);
    ASSERT_TRUE(out.find("\r4/4 100%") != std::string::npos);
    ASSERT_TRUE(out.find("done\n") != std::string::npos);
    return 0;
}

int test_header_mode() {
    int old_fd = -1;
    FILE* tmp = NULL;
    ASSERT_TRUE(capture_stdout_start(&old_fd, &tmp) == "ok");

    PercentStatus p;
    p.init("Scan");
    p.interprint_start();
    p.interprint_end();
    p.finish("ok");

    const std::string out = capture_stdout_stop(old_fd, tmp);
    ASSERT_TRUE(out.find("Scan") != std::string::npos);
    ASSERT_TRUE(out.find("ok\n") != std::string::npos);
    return 0;
}

int main() {
    int failed = 0;
    if (test_verbose_progress() != 0) failed++;
    if (test_header_mode() != 0) failed++;

    if (failed == 0) {
        std::cout << "All percentage tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
