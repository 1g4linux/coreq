#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "coreqTk/filenames.h"
#include "coreqTk/stringtypes.h"

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

static int make_dir(const std::string& path) {
    if (mkdir(path.c_str(), 0700) != 0) {
        std::cerr << "mkdir failed for " << path << ": " << std::strerror(errno) << std::endl;
        return 1;
    }
    return 0;
}

int test_normalize_path() {
    ASSERT_EQ(normalize_path("", false, false), "");
    ASSERT_EQ(normalize_path("///tmp////coreq///", false, false), "/tmp/coreq");
    ASSERT_EQ(normalize_path("///tmp////coreq", false, true), "/tmp/coreq/");
    ASSERT_EQ(normalize_path("/", false, false), "/");
    ASSERT_EQ(normalize_path("/", false, true), "/");

    char dir_template[] = "/tmp/coreq-filenames-test-XXXXXX";
    char* base = mkdtemp(dir_template);
    if (base == NULL) {
        std::cerr << "mkdtemp failed: " << std::strerror(errno) << std::endl;
        return 1;
    }

    const std::string root(base);
    const std::string real_dir = root + "/real";
    if (make_dir(real_dir) != 0) return 1;

    const std::string tricky = root + "/real/../real/.";
    ASSERT_EQ(normalize_path(tricky.c_str(), true, false), real_dir);

    if (rmdir(real_dir.c_str()) != 0) return 1;
    if (rmdir(root.c_str()) != 0) return 1;
    return 0;
}

int test_same_and_prefix() {
    ASSERT_TRUE(same_filenames("/tmp//a///", "/tmp/a", false, false));
    ASSERT_TRUE(!same_filenames("/tmp/a", "/tmp/b", false, false));
    ASSERT_TRUE(same_filenames("/tmp/*/pkg", "/tmp/overlay/pkg", true, false));

    ASSERT_TRUE(filename_starts_with("/tmp/overlay", "/tmp/overlay/pkg", false));
    ASSERT_TRUE(!filename_starts_with("/tmp/overlay", "/tmp/overlay2/pkg", false));
    return 0;
}

int test_find_filenames() {
    WordVec names;
    names.push_back("/tmp/nope");
    names.push_back("/tmp/overlay-*");
    names.push_back("/tmp/overlay-main");

    WordVec::const_iterator it = find_filenames(names.begin(), names.end(), "/tmp/overlay-main", true, false);
    ASSERT_TRUE(it != names.end());
    ASSERT_EQ(*it, "/tmp/overlay-*");

    it = find_filenames(names.begin(), names.end(), "/tmp/missing", true, false);
    ASSERT_TRUE(it == names.end());
    return 0;
}

int test_is_virtual() {
    ASSERT_TRUE(is_virtual("relative-overlay"));

    char dir_template[] = "/tmp/coreq-filenames-virtual-XXXXXX";
    char* base = mkdtemp(dir_template);
    if (base == NULL) {
        std::cerr << "mkdtemp failed: " << std::strerror(errno) << std::endl;
        return 1;
    }

    const std::string root(base);
    const std::string file_path = root + "/entry";
    FILE* fp = fopen(file_path.c_str(), "w");
    if (fp == NULL) return 1;
    fclose(fp);

    ASSERT_TRUE(!is_virtual(root.c_str()));
    ASSERT_TRUE(is_virtual(file_path.c_str()));
    ASSERT_TRUE(is_virtual((root + "/missing").c_str()));

    if (unlink(file_path.c_str()) != 0) return 1;
    if (rmdir(root.c_str()) != 0) return 1;
    return 0;
}

int main() {
    int failed = 0;
    if (test_normalize_path() != 0) failed++;
    if (test_same_and_prefix() != 0) failed++;
    if (test_find_filenames() != 0) failed++;
    if (test_is_virtual() != 0) failed++;

    if (failed == 0) {
        std::cout << "All filenames tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
