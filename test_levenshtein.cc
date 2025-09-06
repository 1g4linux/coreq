#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

typedef size_t Levenshtein;

#define likely(x) (x)

using std::vector;
using std::min;
using std::swap;

Levenshtein get_levenshtein_distance(const char* str_a, const char* str_b) {
  size_t n(std::strlen(str_a));
  size_t m(std::strlen(str_b));
  if (n > m) {
    swap(n, m);
    swap(str_a, str_b);
  }
  if (n == 0) {
    return m;
  }

  vector<Levenshtein> arr(n + 1);
  for (Levenshtein i(0); likely(i <= n); ++i) {
    arr[i] = i;
  }
  for (; m > 0; ++str_b, --m) {
    vector<Levenshtein>::iterator it(arr.begin());
    Levenshtein sub((*it)++);
    for (size_t i(0); i < n; ++i) {
      Levenshtein c(*it);
      c = min(c, *(++it)) + 1;
      if (str_a[i] != *str_b) {
        ++sub;
      }
      if (c > sub) {
        c = sub;
      }
      sub = *it;
      *it = c;
    }
  }
  return arr[n];
}

void test_lev(const char* a, const char* b, Levenshtein expected) {
    Levenshtein res = get_levenshtein_distance(a, b);
    if (res == expected) {
        std::cout << "PASS: '" << a << "', '" << b << "', expected=" << expected << std::endl;
    } else {
        std::cerr << "FAIL: '" << a << "', '" << b << "', expected=" << expected << ", got=" << res << std::endl;
        exit(1);
    }
}

int main() {
    test_lev("kitten", "sitting", 3);
    test_lev("flaw", "lawn", 2);
    test_lev("gumbo", "gambol", 2);
    test_lev("book", "back", 2);
    test_lev("abc", "abcdef", 3);
    test_lev("a", "abc", 2);
    test_lev("abc", "a", 2);
    test_lev("", "abc", 3);
    test_lev("abc", "", 3);
    return 0;
}
