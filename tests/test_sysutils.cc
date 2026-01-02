#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

#include "coreqTk/sysutils.h"

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

int test_uid_gid_lookup() {
    uid_t uid = getuid();
    gid_t gid = getgid();

    struct passwd* pw = getpwuid(uid);
    ASSERT_TRUE(pw != NULL);

    uid_t looked_uid = static_cast<uid_t>(-1);
    gid_t looked_gid = 0;
    ASSERT_TRUE(get_uid_of(pw->pw_name, &looked_uid));
    ASSERT_EQ(looked_uid, uid);
    ASSERT_TRUE(!get_uid_of("coreq-user-does-not-exist-xyz", &looked_uid));

    struct group* gr = getgrgid(gid);
    if (gr != NULL) {
        ASSERT_TRUE(get_gid_of(gr->gr_name, &looked_gid));
        ASSERT_EQ(looked_gid, gid);
    } else {
        ASSERT_TRUE(!get_gid_of("coreq-group-does-not-exist-xyz", &looked_gid));
    }
    return 0;
}

int test_file_and_dir_helpers() {
    char dir_template[] = "/tmp/coreq-sysutils-test-XXXXXX";
    char* base = mkdtemp(dir_template);
    if (base == NULL) {
        std::cerr << "mkdtemp failed: " << std::strerror(errno) << std::endl;
        return 1;
    }
    const std::string root(base);
    const std::string file_path = root + "/file.txt";
    const std::string symlink_path = root + "/file-link";

    FILE* fp = fopen(file_path.c_str(), "w");
    ASSERT_TRUE(fp != NULL);
    fputs("x", fp);
    fclose(fp);

    ASSERT_TRUE(is_dir(root.c_str()));
    ASSERT_TRUE(!is_file(root.c_str()));
    ASSERT_TRUE(is_file(file_path.c_str()));
    ASSERT_TRUE(is_pure_file(file_path.c_str()));

    ASSERT_TRUE(symlink(file_path.c_str(), symlink_path.c_str()) == 0);
    ASSERT_TRUE(is_file(symlink_path.c_str()));
    ASSERT_TRUE(!is_pure_file(symlink_path.c_str()));

    std::time_t mtime = 0;
    ASSERT_TRUE(get_mtime(&mtime, file_path.c_str()));
    ASSERT_TRUE(mtime > 0);

    ASSERT_TRUE(unlink(symlink_path.c_str()) == 0);
    ASSERT_TRUE(unlink(file_path.c_str()) == 0);
    ASSERT_TRUE(rmdir(root.c_str()) == 0);
    return 0;
}

int test_date_and_geometry() {
    const char* date = date_conv("%Y", static_cast<std::time_t>(0));
    ASSERT_TRUE(date != NULL);
    ASSERT_TRUE(std::strlen(date) > 0);

    unsigned int lines = 0;
    unsigned int columns = 0;
    const bool has_geom = get_geometry(&lines, &columns);
    if (has_geom) {
        ASSERT_TRUE(lines > 0);
        ASSERT_TRUE(columns > 0);
    }
    return 0;
}

int main() {
    int failed = 0;
    if (test_uid_gid_lookup() != 0) failed++;
    if (test_file_and_dir_helpers() != 0) failed++;
    if (test_date_and_geometry() != 0) failed++;

    if (failed == 0) {
        std::cout << "All sysutils tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failed << " tests failed!" << std::endl;
    return 1;
}
