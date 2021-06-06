#include "src/string.h"
#include "gtest/gtest.h"

namespace String {
TEST(StringTest, ChainingAssign) {
  String str("abc");
  String a, b, c;
  a = b = c = str;
  EXPECT_TRUE(share_c_str(a, b));
  EXPECT_TRUE(share_c_str(a, c));
}

TEST(StringTest, ModifyString) {
  String a, b, c;
  a = b = c = String("abc");
  EXPECT_TRUE(share_c_str(a, b));
  // c should point to another c string
  c = String("cde");
  EXPECT_NE(a, c);
  // a and b are still equal
  EXPECT_EQ(a, b);
  EXPECT_TRUE(share_c_str(a, b));
  a[0] = '3';
  EXPECT_FALSE(share_c_str(a, b));
  String e("3bc");
  EXPECT_EQ(a, e);
}

TEST(StringTest, ConstString) {
  const String a("abc");
  const String b(a);
  EXPECT_TRUE(share_c_str(a, b));
  // c should point to another c string
  EXPECT_TRUE(share_c_str(a, b));
  const char &ch = a[2];
  EXPECT_TRUE(share_c_str(a, b));
}

TEST(StringTest, MoveString) {
  String a("abc");
  String c = a;
  String b(std::move(a));
  // b and c should point to the same string
  EXPECT_TRUE(share_c_str(b, c));
  // a should be an empty string
  EXPECT_TRUE(a.empty());
}

} // namespace String