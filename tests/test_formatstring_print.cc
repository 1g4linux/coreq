#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "output/formatstring.h"
#include "corepkg/package.h"
#include "corepkg/version.h"

#define ASSERT_TRUE(condition)                                                   \
  if (!(condition)) {                                                            \
    std::cerr << "Assertion failed: " << #condition << " at " << __FILE__       \
              << ":" << __LINE__ << std::endl;                                   \
    return 1;                                                                    \
  }

#define ASSERT_EQ(val1, val2)                                                    \
  if (!((val1) == (val2))) {                                                     \
    std::cerr << "Assertion failed: " << #val1 << " == " << #val2 << " ("       \
              << (val1) << " != " << (val2) << ") at " << __FILE__ << ":"       \
              << __LINE__ << std::endl;                                          \
    return 1;                                                                    \
  }

struct CallbackCtx {
  std::map<std::string, std::string> values;
  std::vector<std::string> calls;
};

static void callback_get_property(OutputString* s, const PrintFormat* /*fmt*/,
                                  void* entity, const std::string& name) {
  CallbackCtx* ctx = static_cast<CallbackCtx*>(entity);
  ctx->calls.push_back(name);
  const std::map<std::string, std::string>::const_iterator it = ctx->values.find(name);
  if (it == ctx->values.end()) {
    s->clear();
    return;
  }
  s->assign_smart(it->second);
}

static int test_parse_and_print_set_variable() {
  PrintFormat fmt(callback_get_property);
  std::string err;
  ASSERT_TRUE(fmt.parseFormat("{*tmp='hello'}<$tmp>", &err));

  CallbackCtx ctx;
  ASSERT_TRUE(fmt.print(&ctx,
                        reinterpret_cast<const DBHeader*>(0x1),
                        reinterpret_cast<VarDbPkg*>(0x1),
                        reinterpret_cast<const CorePkgSettings*>(0x1),
                        reinterpret_cast<const SetStability*>(0x1),
                        true));
  ASSERT_TRUE(ctx.calls.empty());
  return 0;
}

static int test_if_else_branch_uses_expected_property() {
  PrintFormat fmt(callback_get_property);
  std::string err;
  ASSERT_TRUE(fmt.parseFormat("{mode='on'}<true_prop>{else}<false_prop>{}", &err));

  CallbackCtx ctx;
  ctx.values["mode"] = "on";
  ctx.values["true_prop"] = "yes";
  ctx.values["false_prop"] = "no";

  ASSERT_TRUE(fmt.print(&ctx,
                        reinterpret_cast<const DBHeader*>(0x1),
                        reinterpret_cast<VarDbPkg*>(0x1),
                        reinterpret_cast<const CorePkgSettings*>(0x1),
                        reinterpret_cast<const SetStability*>(0x1),
                        true));

  ASSERT_EQ(ctx.calls.size(), static_cast<std::vector<std::string>::size_type>(2));
  ASSERT_EQ(ctx.calls[0], "mode");
  ASSERT_EQ(ctx.calls[1], "true_prop");
  return 0;
}

static int test_virtual_overlay_helpers() {
  PrintFormat fmt;
  fmt.clear_virtual(4);
  fmt.set_as_virtual(1, true);
  fmt.set_as_virtual(3, true);

  ASSERT_TRUE(!fmt.is_virtual(0));
  ASSERT_TRUE(fmt.is_virtual(1));
  ASSERT_TRUE(!fmt.is_virtual(2));
  ASSERT_TRUE(fmt.is_virtual(3));
  ASSERT_TRUE(!fmt.is_virtual(4));
  return 0;
}

static int test_have_virtual_mixed_versions() {
  Package pkg("sys-apps", "coreq-test");
  std::string err;

  Version* v1 = new Version();
  ASSERT_EQ(v1->parseVersion("1.0", &err), BasicVersion::parsedOK);
  v1->set_slotname("");
  v1->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
  v1->overlay_key = 1;
  pkg.addVersion(v1);

  Version* v2 = new Version();
  ASSERT_EQ(v2->parseVersion("2.0", &err), BasicVersion::parsedOK);
  v2->set_slotname("");
  v2->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
  v2->overlay_key = 2;
  pkg.addVersion(v2);

  PrintFormat fmt;
  fmt.clear_virtual(3);
  fmt.set_as_virtual(2, true);

  ASSERT_TRUE(fmt.have_virtual(&pkg, false));
  ASSERT_TRUE(fmt.have_virtual(&pkg, true));
  return 0;
}

static int test_overlay_keytext_translation_and_usage_tracking() {
  PrintFormat fmt;
  PrintFormat::OverlayTranslations translations(3, 0);
  PrintFormat::OverlayUsed used(3, false);
  bool some_used = false;

  fmt.color_overlaykey = "<O>";
  fmt.color_virtualkey = "<V>";
  fmt.color_keyend = "</>";
  fmt.set_overlay_translations(&translations);
  fmt.set_overlay_used(&used, &some_used);
  fmt.clear_virtual(4);
  fmt.set_as_virtual(2, true);

  OutputString s2;
  fmt.overlay_keytext(&s2, 2, false);
  ASSERT_EQ(s2.as_string(), std::string("<V>[1]</>"));
  ASSERT_TRUE(used[1]);
  ASSERT_TRUE(some_used);

  OutputString s3;
  fmt.overlay_keytext(&s3, 3, false);
  ASSERT_EQ(s3.as_string(), std::string("<O>[2]</>"));

  OutputString plain;
  fmt.overlay_keytext(&plain, 2, true);
  ASSERT_EQ(plain.as_string(), std::string("1"));
  return 0;
}

int main() {
  ExtendedVersion::init_static();

  int failed = 0;
  if (test_parse_and_print_set_variable() != 0) {
    failed++;
  }
  if (test_if_else_branch_uses_expected_property() != 0) {
    failed++;
  }
  if (test_virtual_overlay_helpers() != 0) {
    failed++;
  }
  if (test_have_virtual_mixed_versions() != 0) {
    failed++;
  }
  if (test_overlay_keytext_translation_and_usage_tracking() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All formatstring-print tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
