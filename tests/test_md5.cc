#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "coreqTk/md5.h"

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

static int write_file(const std::string& content, std::string* out_path) {
    char tmp_template[] = "/tmp/coreq-md5-test-XXXXXX";
    const int fd = mkstemp(tmp_template);
    if (fd < 0) {
        std::cerr << "mkstemp failed: " << std::strerror(errno) << std::endl;
        return 1;
    }
    *out_path = tmp_template;
    if (!content.empty()) {
        const ssize_t written = write(fd, content.data(), content.size());
        if (written != static_cast<ssize_t>(content.size())) {
            close(fd);
            return 1;
        }
    }
    if (close(fd) != 0) return 1;
    return 0;
}

int test_md5_vectors() {
    std::string path;

    if (write_file("", &path) != 0) return 1;
    ASSERT_TRUE(verify_md5sum(path.c_str(), "d41d8cd98f00b204e9800998ecf8427e"));
    ASSERT_TRUE(verify_md5sum(path.c_str(), "D41D8CD98F00B204E9800998ECF8427E"));
    ASSERT_TRUE(!verify_md5sum(path.c_str(), "d41d8cd98f00b204e9800998ecf8427f"));
    if (unlink(path.c_str()) != 0) return 1;

    if (write_file("abc", &path) != 0) return 1;
    ASSERT_TRUE(verify_md5sum(path.c_str(), "900150983cd24fb0d6963f7d28e17f72"));
    ASSERT_TRUE(!verify_md5sum(path.c_str(), "900150983cd24fb0d6963f7d28e17f73"));
    if (unlink(path.c_str()) != 0) return 1;

    if (write_file("The quick brown fox jumps over the lazy dog", &path) != 0) return 1;
    ASSERT_TRUE(verify_md5sum(path.c_str(), "9e107d9d372bb6826bd81d3542a419d6"));
    if (unlink(path.c_str()) != 0) return 1;

    if (write_file(std::string(80, 'a'), &path) != 0) return 1;
    ASSERT_TRUE(verify_md5sum(path.c_str(), "b15af9cdabbaea0516866a33d8fd0f98"));
    if (unlink(path.c_str()) != 0) return 1;

    return 0;
}

int test_invalid_input() {
    ASSERT_TRUE(!verify_md5sum("/tmp/does-not-exist-coreq-md5", "d41d8cd98f00b204e9800998ecf8427e"));
    ASSERT_TRUE(!verify_md5sum("/tmp/does-not-exist-coreq-md5", "short"));
    ASSERT_TRUE(!verify_md5sum("/tmp/does-not-exist-coreq-md5", "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"));
    return 0;
}

int main() {
    int failed = 0;
    if (test_md5_vectors() != 0) failed++;
    if (test_invalid_input() != 0) failed++;

    if (failed == 0) {
        std::cout << "All md5 tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
