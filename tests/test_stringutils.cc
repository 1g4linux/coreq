#include <iostream>
#include <string>
#include <vector>
#include "coreqTk/stringutils.h"
#include "coreqTk/coreqint.h"

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

int test_trim() {
    std::string s = "  hello  ";
    trim(&s);
    ASSERT_EQ(s, "hello");
    
    s = " \t\n hello \r ";
    trim(&s);
    ASSERT_EQ(s, "hello");
    return 0;
}

int test_to_lower() {
    ASSERT_EQ(to_lower("Hello WORLD"), "hello world");
    ASSERT_EQ(to_lower("123!@#"), "123!@#");
    return 0;
}

int test_is_numeric() {
    ASSERT_TRUE(is_numeric("12345"));
    ASSERT_TRUE(!is_numeric("123a45"));
    ASSERT_TRUE(is_numeric(""));
    return 0;
}

int test_natcmp() {
    // 0: equal, -1: a < b, 1: a > b
    ASSERT_EQ(natcmp("abc", "abc"), 0);
    ASSERT_EQ(natcmp("abc10", "abc2"), 1);
    ASSERT_EQ(natcmp("abc2", "abc10"), -1);
    ASSERT_EQ(natcmp("abc01", "abc1"), 1); // "Leading zeroes are ignored, but if numbers differ only by leading zeroes, the one with more leading zeroes is considered to be larger."
    ASSERT_EQ(natcmp("a1", "a01"), -1);
    return 0;
}

int test_split_string() {
    WordVec v = split_string("foo bar  baz", false, " ", true);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "bar");
    ASSERT_EQ(v[2], "baz");

    v = split_string("foo:bar:baz", false, ":", true);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "bar");
    ASSERT_EQ(v[2], "baz");

    v = split_string("foo::bar", false, ":", false);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "foo");
    ASSERT_EQ(v[1], "");
    ASSERT_EQ(v[2], "bar");

    return 0;
}

int test_unescape_string() {
    std::string s = "hello\\nworld";
    unescape_string(&s);
    ASSERT_EQ(s, "hello\nworld");

    s = "tab\\tchar";
    unescape_string(&s);
    ASSERT_EQ(s, "tab\tchar");

    s = "escaped\\\\backslash";
    unescape_string(&s);
    ASSERT_EQ(s, "escaped\\backslash");

    return 0;
}

int test_trimall_and_optional_append() {
    std::string s = "  alpha   beta\tgamma  ";
    trimall(&s);
    ASSERT_EQ(s, "alpha beta gamma");

    s = "path";
    optional_append(&s, '/');
    optional_append(&s, '/');
    ASSERT_EQ(s, "path/");
    return 0;
}

int test_slot_subslot_helpers() {
    std::string slot = "0/1";
    std::string subslot;
    ASSERT_TRUE(slot_subslot(&slot, &subslot));
    ASSERT_EQ(slot, "");
    ASSERT_EQ(subslot, "1");

    slot = "2/0";
    ASSERT_TRUE(!slot_subslot(&slot, &subslot));
    ASSERT_EQ(slot, "2");
    ASSERT_EQ(subslot, "");

    std::string out_slot;
    ASSERT_TRUE(slot_subslot("3/4", &out_slot, &subslot));
    ASSERT_EQ(out_slot, "3");
    ASSERT_EQ(subslot, "4");

    ASSERT_TRUE(!slot_subslot("0", &out_slot, &subslot));
    ASSERT_EQ(out_slot, "");
    ASSERT_EQ(subslot, "");
    return 0;
}

int test_explode_atom_splitters() {
    std::string name;
    std::string version;
    ASSERT_TRUE(ExplodeAtom::split(&name, &version, "coreq-1.2.3-r1"));
    ASSERT_EQ(name, "coreq");
    ASSERT_EQ(version, "1.2.3-r1");

    ASSERT_TRUE(ExplodeAtom::split_name(&name, "pkg-10"));
    ASSERT_EQ(name, "pkg");
    ASSERT_TRUE(ExplodeAtom::split_version(&version, "pkg-10"));
    ASSERT_EQ(version, "10");

    ASSERT_TRUE(!ExplodeAtom::split_name(&name, "pkg"));
    return 0;
}

int test_escape_split_and_join() {
    std::string s = "a b\\ c d\\\\e";
    WordVec v = split_string(s, true, " ", true);
    ASSERT_EQ(v.size(), 3u);
    ASSERT_EQ(v[0], "a");
    ASSERT_EQ(v[1], "b c");
    ASSERT_EQ(v[2], "d\\e");

    std::string escaped = "a b";
    escape_string(&escaped, " ");
    ASSERT_EQ(escaped, "a\\ b");

    std::string joined;
    split_and_join(&joined, "a::b", ",", false, ":", false);
    ASSERT_EQ(joined, "a,,b");
    return 0;
}

int test_resolve_plus_minus_and_pkgpath_helpers() {
    WordSet resolved;
    ASSERT_TRUE(resolve_plus_minus(&resolved, "foo bar -foo -baz", NULLPTR));
    ASSERT_TRUE(resolved.find("bar") != resolved.end());
    ASSERT_TRUE(resolved.find("foo") == resolved.end());

    ASSERT_TRUE(is_valid_pkgpath('_'));
    ASSERT_TRUE(is_valid_pkgpath('/'));
    ASSERT_TRUE(!is_valid_pkgpath('!'));

    ASSERT_EQ(std::string(first_alnum("...x")), "x");
    ASSERT_EQ(std::string(first_not_alnum_or_ok("abc-_.", "-_")), ".");
    return 0;
}

int main() {
    int failed = 0;
    if (test_trim() != 0) failed++;
    if (test_to_lower() != 0) failed++;
    if (test_is_numeric() != 0) failed++;
    if (test_natcmp() != 0) failed++;
    if (test_split_string() != 0) failed++;
    if (test_unescape_string() != 0) failed++;
    if (test_trimall_and_optional_append() != 0) failed++;
    if (test_slot_subslot_helpers() != 0) failed++;
    if (test_explode_atom_splitters() != 0) failed++;
    if (test_escape_split_and_join() != 0) failed++;
    if (test_resolve_plus_minus_and_pkgpath_helpers() != 0) failed++;
    
    if (failed == 0) {
        std::cout << "All stringutils tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " tests failed!" << std::endl;
        return 1;
    }
}
