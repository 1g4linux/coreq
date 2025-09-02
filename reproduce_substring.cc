#include <iostream>
#include <string>
#include <vector>
#include <cstring>

// Mock classes to satisfy SubstringAlgorithm
class Package {};
class BaseAlgorithm {
protected:
    std::string search_string;
public:
    virtual void setString(const std::string& s) { search_string = s; }
    virtual bool operator()(const char* s, Package* p) const = 0;
};

#define FINAL
#define OVERRIDE override
#define ATTRIBUTE_NONNULL(x)
#define NULLPTR nullptr

class SubstringAlgorithm FINAL : public BaseAlgorithm {
 public:
  ATTRIBUTE_NONNULL((2)) bool operator()(const char* s, Package* /* p */) const OVERRIDE { 
      return (std::strstr(s, search_string.c_str()) != NULLPTR); 
  }
};

void test(const char* pattern, const char* text, bool expected) {
    SubstringAlgorithm algo;
    algo.setString(pattern);
    bool result = algo(text, nullptr);
    if (result == expected) {
        std::cout << "PASS: pattern='" << pattern << "', text='" << text << "', expected=" << expected << std::endl;
    } else {
        std::cerr << "FAIL: pattern='" << pattern << "', text='" << text << "', expected=" << expected << ", got=" << result << std::endl;
        exit(1);
    }
}

int main() {
    test("foo", "foobar", true);
    test("bar", "foobar", true);
    test("baz", "foobar", false);
    test("oob", "foobar", true);
    test("", "foobar", true); // strstr(s, "") returns s, so not null
    test("foobar", "foo", false);
    return 0;
}
