#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "output/formatstring.h"
#include "coreqTk/ansicolor.h"

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

static int test_format_parser_builds_expected_node_chain() {
  FormatParser parser;
  std::string err;
  ASSERT_TRUE(parser.start("abc<foo>{*tmp='bar'}<$tmp>", true, false, &err));

  Node* node = parser.rootnode();
  ASSERT_TRUE(node != NULLPTR);

  ASSERT_EQ(node->type, Node::TEXT);
  Text* text = static_cast<Text*>(node);
  ASSERT_EQ(text->text.as_string(), std::string("abc"));

  node = node->next;
  ASSERT_TRUE(node != NULLPTR);
  ASSERT_EQ(node->type, Node::OUTPUT);
  Property* prop = static_cast<Property*>(node);
  ASSERT_EQ(prop->name, std::string("foo"));
  ASSERT_TRUE(!prop->user_variable);

  node = node->next;
  ASSERT_TRUE(node != NULLPTR);
  ASSERT_EQ(node->type, Node::SET);
  ConditionBlock* set = static_cast<ConditionBlock*>(node);
  ASSERT_EQ(set->variable.name, std::string("tmp"));
  ASSERT_EQ(set->text.text.as_string(), std::string("bar"));
  ASSERT_EQ(set->rhs, ConditionBlock::RHS_STRING);
  ASSERT_TRUE(set->user_variable);
  ASSERT_TRUE(!set->negation);

  node = node->next;
  ASSERT_TRUE(node != NULLPTR);
  ASSERT_EQ(node->type, Node::OUTPUT);
  prop = static_cast<Property*>(node);
  ASSERT_EQ(prop->name, std::string("tmp"));
  ASSERT_TRUE(prop->user_variable);

  ASSERT_TRUE(node->next == NULLPTR);

  delete parser.rootnode();
  return 0;
}

static int test_format_parser_reports_syntax_errors() {
  FormatParser parser;
  std::string err;

  ASSERT_TRUE(!parser.start("{", true, false, &err));
  ASSERT_TRUE(err.find("no name of property/variable after '{'") != std::string::npos);
  ASSERT_TRUE(err.find("line") != std::string::npos);
  ASSERT_TRUE(err.find("column") != std::string::npos);

  err.clear();
  ASSERT_TRUE(!parser.start("a(blue", true, false, &err));
  ASSERT_TRUE(err.find("without closing ')'") != std::string::npos);

  err.clear();
  ASSERT_TRUE(!parser.start("<foo", true, false, &err));
  ASSERT_TRUE(err.find("without closing '>'") != std::string::npos);

  return 0;
}

static int test_parse_only_colors_mode() {
  FormatParser parser_with_colors;
  std::string err;

  ASSERT_TRUE(parser_with_colors.start("(red)X", true, true, &err));
  Node* root = parser_with_colors.rootnode();
  ASSERT_TRUE(root != NULLPTR);
  ASSERT_EQ(root->type, Node::TEXT);
  ASSERT_TRUE(!static_cast<Text*>(root)->text.as_string().empty());

  Node* second = root->next;
  ASSERT_TRUE(second != NULLPTR);
  ASSERT_EQ(second->type, Node::TEXT);
  ASSERT_EQ(static_cast<Text*>(second)->text.as_string(), std::string("X"));
  ASSERT_TRUE(second->next == NULLPTR);
  delete parser_with_colors.rootnode();

  FormatParser parser_without_colors;
  ASSERT_TRUE(parser_without_colors.start("(red)X", false, true, &err));
  root = parser_without_colors.rootnode();
  ASSERT_TRUE(root != NULLPTR);
  ASSERT_EQ(root->type, Node::TEXT);
  ASSERT_EQ(static_cast<Text*>(root)->text.as_string(), std::string("X"));
  ASSERT_TRUE(root->next == NULLPTR);
  delete parser_without_colors.rootnode();

  return 0;
}

static int test_var_parser_cache_clear_use() {
  VarParserCache cache;
  cache["alpha"].in_use = true;
  cache["beta"].in_use = true;
  cache["gamma"].in_use = false;

  cache.clear_use();

  ASSERT_TRUE(!cache["alpha"].in_use);
  ASSERT_TRUE(!cache["beta"].in_use);
  ASSERT_TRUE(!cache["gamma"].in_use);
  return 0;
}

static int test_print_format_replaces_previous_root() {
  PrintFormat fmt(callback_get_property);
  std::string err;
  CallbackCtx ctx;

  ASSERT_TRUE(fmt.parseFormat("<left>", &err));
  ctx.values["left"] = "L";
  ASSERT_TRUE(fmt.print(&ctx,
                        reinterpret_cast<const DBHeader*>(0x1),
                        reinterpret_cast<VarDbPkg*>(0x1),
                        reinterpret_cast<const CorePkgSettings*>(0x1),
                        reinterpret_cast<const SetStability*>(0x1),
                        true));
  ASSERT_EQ(ctx.calls.size(), static_cast<std::vector<std::string>::size_type>(1));
  ASSERT_EQ(ctx.calls[0], std::string("left"));

  ctx.calls.clear();
  ctx.values.clear();
  ctx.values["right"] = "R";

  ASSERT_TRUE(fmt.parseFormat("<right>", &err));
  ASSERT_TRUE(fmt.print(&ctx,
                        reinterpret_cast<const DBHeader*>(0x1),
                        reinterpret_cast<VarDbPkg*>(0x1),
                        reinterpret_cast<const CorePkgSettings*>(0x1),
                        reinterpret_cast<const SetStability*>(0x1),
                        true));
  ASSERT_EQ(ctx.calls.size(), static_cast<std::vector<std::string>::size_type>(1));
  ASSERT_EQ(ctx.calls[0], std::string("right"));
  return 0;
}

int main() {
  AnsiColor::init_static();

  int failed = 0;
  if (test_format_parser_builds_expected_node_chain() != 0) {
    failed++;
  }
  if (test_format_parser_reports_syntax_errors() != 0) {
    failed++;
  }
  if (test_parse_only_colors_mode() != 0) {
    failed++;
  }
  if (test_var_parser_cache_clear_use() != 0) {
    failed++;
  }
  if (test_print_format_replaces_previous_root() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All formatstring tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
