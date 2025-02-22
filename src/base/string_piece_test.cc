// Copyright 2010-2014, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <string>

#include "base/string_piece.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"

namespace mozc {

#define ASSERT_SIZE_EQ(a, b) ASSERT_EQ((a), StringPiece::size_type(b))

TEST(StringPieceTest, CheckComparisonOperators) {
#define CMP_Y(op, x, y)                                               \
  ASSERT_TRUE((StringPiece((x)) op StringPiece((y))));                \
  ASSERT_TRUE((StringPiece((x)).compare(StringPiece((y))) op 0))

#define CMP_N(op, x, y)                                               \
  ASSERT_FALSE(StringPiece((x)) op StringPiece((y)));                 \
  ASSERT_FALSE(StringPiece((x)).compare(StringPiece((y))) op 0)

  CMP_Y(==, "",   "");
  CMP_Y(==, "a",  "a");
  CMP_Y(==, "aa", "aa");
  CMP_N(==, "a",  "");
  CMP_N(==, "",   "a");
  CMP_N(==, "a",  "b");
  CMP_N(==, "a",  "aa");
  CMP_N(==, "aa", "a");

  CMP_N(!=, "",   "");
  CMP_N(!=, "a",  "a");
  CMP_N(!=, "aa", "aa");
  CMP_Y(!=, "a",  "");
  CMP_Y(!=, "",   "a");
  CMP_Y(!=, "a",  "b");
  CMP_Y(!=, "a",  "aa");
  CMP_Y(!=, "aa", "a");

  CMP_Y(<, "a",  "b");
  CMP_Y(<, "a",  "aa");
  CMP_Y(<, "aa", "b");
  CMP_Y(<, "aa", "bb");
  CMP_N(<, "a",  "a");
  CMP_N(<, "b",  "a");
  CMP_N(<, "aa", "a");
  CMP_N(<, "b",  "aa");
  CMP_N(<, "bb", "aa");

  CMP_Y(<=, "a",  "a");
  CMP_Y(<=, "a",  "b");
  CMP_Y(<=, "a",  "aa");
  CMP_Y(<=, "aa", "b");
  CMP_Y(<=, "aa", "bb");
  CMP_N(<=, "b",  "a");
  CMP_N(<=, "aa", "a");
  CMP_N(<=, "b",  "aa");
  CMP_N(<=, "bb", "aa");

  CMP_N(>=, "a",  "b");
  CMP_N(>=, "a",  "aa");
  CMP_N(>=, "aa", "b");
  CMP_N(>=, "aa", "bb");
  CMP_Y(>=, "a",  "a");
  CMP_Y(>=, "b",  "a");
  CMP_Y(>=, "aa", "a");
  CMP_Y(>=, "b",  "aa");
  CMP_Y(>=, "bb", "aa");

  CMP_N(>, "a",  "a");
  CMP_N(>, "a",  "b");
  CMP_N(>, "a",  "aa");
  CMP_N(>, "aa", "b");
  CMP_N(>, "aa", "bb");
  CMP_Y(>, "b",  "a");
  CMP_Y(>, "aa", "a");
  CMP_Y(>, "b",  "aa");
  CMP_Y(>, "bb", "aa");

  string x;
  for (int i = 0; i < 256; i++) {
    x += 'a';
    string y = x;
    CMP_Y(==, x, y);
    for (int j = 0; j < i; j++) {
      string z = x;
      z[j] = 'b';       // Differs in position 'j'
      CMP_N(==, x, z);
    }
  }

#undef CMP_Y
#undef CMP_N
}

TEST(StringPieceTest, CheckSTL) {
  StringPiece a("abcdefghijklmnopqrstuvwxyz");
  StringPiece b("abc");
  StringPiece c("xyz");
  StringPiece d("foobar");
  StringPiece e;
  string temp("123");
  temp += '\0';
  temp += "456";
  StringPiece f(temp);

  ASSERT_EQ(a[6], 'g');
  ASSERT_EQ(b[0], 'a');
  ASSERT_EQ(c[2], 'z');
  ASSERT_EQ(f[3], '\0');
  ASSERT_EQ(f[5], '5');

  ASSERT_EQ(*d.data(), 'f');
  ASSERT_EQ(d.data()[5], 'r');
  ASSERT_TRUE(e.data() == NULL);

  ASSERT_EQ(*a.begin(), 'a');
  ASSERT_EQ(*(b.begin() + 2), 'c');
  ASSERT_EQ(*(c.end() - 1), 'z');

  ASSERT_EQ(*a.rbegin(), 'z');
  ASSERT_EQ(*(b.rbegin() + 2), 'a');
  ASSERT_EQ(*(c.rend() - 1), 'x');
  ASSERT_TRUE(a.rbegin() + 26 == a.rend());

  ASSERT_SIZE_EQ(a.size(), 26);
  ASSERT_SIZE_EQ(b.size(), 3);
  ASSERT_SIZE_EQ(c.size(), 3);
  ASSERT_SIZE_EQ(d.size(), 6);
  ASSERT_SIZE_EQ(e.size(), 0);
  ASSERT_SIZE_EQ(f.size(), 7);

  ASSERT_TRUE(!d.empty());
  ASSERT_TRUE(d.begin() != d.end());
  ASSERT_TRUE(d.begin() + 6 == d.end());

  ASSERT_TRUE(e.empty());
  ASSERT_TRUE(e.begin() == e.end());

  d.clear();
  ASSERT_SIZE_EQ(d.size(), 0);
  ASSERT_TRUE(d.empty());
  ASSERT_TRUE(d.data() == NULL);
  ASSERT_TRUE(d.begin() == d.end());

  ASSERT_GE(a.max_size(), a.capacity());
  ASSERT_GE(a.capacity(), a.size());

  char buf[4] = { '%', '%', '%', '%' };
  ASSERT_SIZE_EQ(a.copy(buf, 4), 4);
  ASSERT_EQ(buf[0], a[0]);
  ASSERT_EQ(buf[1], a[1]);
  ASSERT_EQ(buf[2], a[2]);
  ASSERT_EQ(buf[3], a[3]);
  ASSERT_SIZE_EQ(a.copy(buf, 3, 7), 3);
  ASSERT_EQ(buf[0], a[7]);
  ASSERT_EQ(buf[1], a[8]);
  ASSERT_EQ(buf[2], a[9]);
  ASSERT_EQ(buf[3], a[3]);
  ASSERT_SIZE_EQ(c.copy(buf, 99), 3);
  ASSERT_EQ(buf[0], c[0]);
  ASSERT_EQ(buf[1], c[1]);
  ASSERT_EQ(buf[2], c[2]);
  ASSERT_EQ(buf[3], a[3]);

  ASSERT_EQ(StringPiece::npos, string::npos);

  ASSERT_SIZE_EQ(a.find(b), 0);
  ASSERT_SIZE_EQ(a.find(b, 1), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find(c), 23);
  ASSERT_SIZE_EQ(a.find(c, 9), 23);
  ASSERT_SIZE_EQ(a.find(c, StringPiece::npos), StringPiece::npos);
  ASSERT_SIZE_EQ(b.find(c), StringPiece::npos);
  ASSERT_SIZE_EQ(b.find(c, StringPiece::npos), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find(d), 0);
  ASSERT_SIZE_EQ(a.find(e), 0);
  ASSERT_SIZE_EQ(a.find(d, 12), 12);
  ASSERT_SIZE_EQ(a.find(e, 17), 17);
  StringPiece g("xx not found bb");
  ASSERT_SIZE_EQ(a.find(g), StringPiece::npos);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.find(b), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find(b), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find(b, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find(b, 7), StringPiece::npos);

  size_t empty_search_pos = string().find(string());
  ASSERT_SIZE_EQ(d.find(d), empty_search_pos);
  ASSERT_SIZE_EQ(d.find(e), empty_search_pos);
  ASSERT_SIZE_EQ(e.find(d), empty_search_pos);
  ASSERT_SIZE_EQ(e.find(e), empty_search_pos);
  ASSERT_SIZE_EQ(d.find(d, 4), string().find(string(), 4));
  ASSERT_SIZE_EQ(d.find(e, 4), string().find(string(), 4));
  ASSERT_SIZE_EQ(e.find(d, 4), string().find(string(), 4));
  ASSERT_SIZE_EQ(e.find(e, 4), string().find(string(), 4));

  ASSERT_SIZE_EQ(a.find('a'), 0);
  ASSERT_SIZE_EQ(a.find('c'), 2);
  ASSERT_SIZE_EQ(a.find('z'), 25);
  ASSERT_SIZE_EQ(a.find('$'), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find('\0'), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find('\0'), 3);
  ASSERT_SIZE_EQ(f.find('3'), 2);
  ASSERT_SIZE_EQ(f.find('5'), 5);
  ASSERT_SIZE_EQ(g.find('o'), 4);
  ASSERT_SIZE_EQ(g.find('o', 4), 4);
  ASSERT_SIZE_EQ(g.find('o', 5), 8);
  ASSERT_SIZE_EQ(a.find('b', 5), StringPiece::npos);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.find('\0'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find('\0'), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find('\0', 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find('\0', 7), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find('x', 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find('x', 7), StringPiece::npos);

  ASSERT_SIZE_EQ(a.rfind(b), 0);
  ASSERT_SIZE_EQ(a.rfind(b, 1), 0);
  ASSERT_SIZE_EQ(a.rfind(c), 23);
  ASSERT_SIZE_EQ(a.rfind(c, 22), StringPiece::npos);
  ASSERT_SIZE_EQ(a.rfind(c, 1), StringPiece::npos);
  ASSERT_SIZE_EQ(a.rfind(c, 0), StringPiece::npos);
  ASSERT_SIZE_EQ(b.rfind(c), StringPiece::npos);
  ASSERT_SIZE_EQ(b.rfind(c, 0), StringPiece::npos);
  ASSERT_EQ(a.rfind(d), a.as_string().rfind(string()));
  ASSERT_EQ(a.rfind(e), a.as_string().rfind(string()));
  ASSERT_SIZE_EQ(a.rfind(d, 12), 12);
  ASSERT_SIZE_EQ(a.rfind(e, 17), 17);
  ASSERT_SIZE_EQ(a.rfind(g), StringPiece::npos);
  ASSERT_SIZE_EQ(d.rfind(b), StringPiece::npos);
  ASSERT_SIZE_EQ(e.rfind(b), StringPiece::npos);
  ASSERT_SIZE_EQ(d.rfind(b, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.rfind(b, 7), StringPiece::npos);
  // empty string nonsense
  ASSERT_EQ(d.rfind(d, 4), string().rfind(string()));
  ASSERT_EQ(e.rfind(d, 7), string().rfind(string()));
  ASSERT_EQ(d.rfind(e, 4), string().rfind(string()));
  ASSERT_EQ(e.rfind(e, 7), string().rfind(string()));
  ASSERT_EQ(d.rfind(d), string().rfind(string()));
  ASSERT_EQ(e.rfind(d), string().rfind(string()));
  ASSERT_EQ(d.rfind(e), string().rfind(string()));
  ASSERT_EQ(e.rfind(e), string().rfind(string()));

  ASSERT_SIZE_EQ(g.rfind('o'), 8);
  ASSERT_SIZE_EQ(g.rfind('q'), StringPiece::npos);
  ASSERT_SIZE_EQ(g.rfind('o', 8), 8);
  ASSERT_SIZE_EQ(g.rfind('o', 7), 4);
  ASSERT_SIZE_EQ(g.rfind('o', 3), StringPiece::npos);
  ASSERT_SIZE_EQ(f.rfind('\0'), 3);
  ASSERT_SIZE_EQ(f.rfind('\0', 12), 3);
  ASSERT_SIZE_EQ(f.rfind('3'), 2);
  ASSERT_SIZE_EQ(f.rfind('5'), 5);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.rfind('o'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.rfind('o'), StringPiece::npos);
  ASSERT_SIZE_EQ(d.rfind('o', 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.rfind('o', 7), StringPiece::npos);

  ASSERT_SIZE_EQ(a.find_first_of(b), 0);
  ASSERT_SIZE_EQ(a.find_first_of(b, 0), 0);
  ASSERT_SIZE_EQ(a.find_first_of(b, 1), 1);
  ASSERT_SIZE_EQ(a.find_first_of(b, 2), 2);
  ASSERT_SIZE_EQ(a.find_first_of(b, 3), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find_first_of(c), 23);
  ASSERT_SIZE_EQ(a.find_first_of(c, 23), 23);
  ASSERT_SIZE_EQ(a.find_first_of(c, 24), 24);
  ASSERT_SIZE_EQ(a.find_first_of(c, 25), 25);
  ASSERT_SIZE_EQ(a.find_first_of(c, 26), StringPiece::npos);
  ASSERT_SIZE_EQ(g.find_first_of(b), 13);
  ASSERT_SIZE_EQ(g.find_first_of(c), 0);
  ASSERT_SIZE_EQ(a.find_first_of(f), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_first_of(a), StringPiece::npos);
  // empty string nonsense
  ASSERT_SIZE_EQ(a.find_first_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find_first_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_of(b), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_of(b), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_of(e), StringPiece::npos);

  ASSERT_SIZE_EQ(a.find_first_not_of(b), 3);
  ASSERT_SIZE_EQ(a.find_first_not_of(c), 0);
  ASSERT_SIZE_EQ(b.find_first_not_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(c.find_first_not_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_first_not_of(a), 0);
  ASSERT_SIZE_EQ(a.find_first_not_of(f), 0);
  ASSERT_SIZE_EQ(a.find_first_not_of(d), 0);
  ASSERT_SIZE_EQ(a.find_first_not_of(e), 0);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.find_first_not_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_not_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_not_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_not_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_not_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_not_of(e), StringPiece::npos);

  StringPiece h("====");
  ASSERT_SIZE_EQ(h.find_first_not_of('='), StringPiece::npos);
  ASSERT_SIZE_EQ(h.find_first_not_of('=', 3), StringPiece::npos);
  ASSERT_SIZE_EQ(h.find_first_not_of('\0'), 0);
  ASSERT_SIZE_EQ(g.find_first_not_of('x'), 2);
  ASSERT_SIZE_EQ(f.find_first_not_of('\0'), 0);
  ASSERT_SIZE_EQ(f.find_first_not_of('\0', 3), 4);
  ASSERT_SIZE_EQ(f.find_first_not_of('\0', 2), 2);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.find_first_not_of('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_not_of('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_first_not_of('\0'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_first_not_of('\0'), StringPiece::npos);

  //  StringPiece g("xx not found bb");
  StringPiece i("56");
  ASSERT_SIZE_EQ(h.find_last_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(g.find_last_of(a), g.size()-1);
  ASSERT_SIZE_EQ(a.find_last_of(b), 2);
  ASSERT_SIZE_EQ(a.find_last_of(c), a.size()-1);
  ASSERT_SIZE_EQ(f.find_last_of(i), 6);
  ASSERT_SIZE_EQ(a.find_last_of('a'), 0);
  ASSERT_SIZE_EQ(a.find_last_of('b'), 1);
  ASSERT_SIZE_EQ(a.find_last_of('z'), 25);
  ASSERT_SIZE_EQ(a.find_last_of('a', 5), 0);
  ASSERT_SIZE_EQ(a.find_last_of('b', 5), 1);
  ASSERT_SIZE_EQ(a.find_last_of('b', 0), StringPiece::npos);
  ASSERT_SIZE_EQ(a.find_last_of('z', 25), 25);
  ASSERT_SIZE_EQ(a.find_last_of('z', 24), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_last_of(i, 5), 5);
  ASSERT_SIZE_EQ(f.find_last_of(i, 6), 6);
  ASSERT_SIZE_EQ(f.find_last_of(a, 4), StringPiece::npos);
  // empty string nonsense
  ASSERT_SIZE_EQ(f.find_last_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_last_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_last_of(d, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_last_of(e, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(f), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(f), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(d, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(e, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(d, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(e, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_of(f, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_of(f, 4), StringPiece::npos);

  ASSERT_SIZE_EQ(a.find_last_not_of(b), a.size()-1);
  ASSERT_SIZE_EQ(a.find_last_not_of(c), 22);
  ASSERT_SIZE_EQ(b.find_last_not_of(a), StringPiece::npos);
  ASSERT_SIZE_EQ(b.find_last_not_of(b), StringPiece::npos);
  ASSERT_SIZE_EQ(f.find_last_not_of(i), 4);
  ASSERT_SIZE_EQ(a.find_last_not_of(c, 24), 22);
  ASSERT_SIZE_EQ(a.find_last_not_of(b, 3), 3);
  ASSERT_SIZE_EQ(a.find_last_not_of(b, 2), StringPiece::npos);
  // empty string nonsense
  ASSERT_SIZE_EQ(f.find_last_not_of(d), f.size()-1);
  ASSERT_SIZE_EQ(f.find_last_not_of(e), f.size()-1);
  ASSERT_SIZE_EQ(f.find_last_not_of(d, 4), 4);
  ASSERT_SIZE_EQ(f.find_last_not_of(e, 4), 4);
  ASSERT_SIZE_EQ(d.find_last_not_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(d), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(e), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of(f), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(f), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of(d, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of(e, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(d, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(e, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of(f, 4), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of(f, 4), StringPiece::npos);

  ASSERT_SIZE_EQ(h.find_last_not_of('x'), h.size() - 1);
  ASSERT_SIZE_EQ(h.find_last_not_of('='), StringPiece::npos);
  ASSERT_SIZE_EQ(b.find_last_not_of('c'), 1);
  ASSERT_SIZE_EQ(h.find_last_not_of('x', 2), 2);
  ASSERT_SIZE_EQ(h.find_last_not_of('=', 2), StringPiece::npos);
  ASSERT_SIZE_EQ(b.find_last_not_of('b', 1), 0);
  // empty string nonsense
  ASSERT_SIZE_EQ(d.find_last_not_of('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of('x'), StringPiece::npos);
  ASSERT_SIZE_EQ(d.find_last_not_of('\0'), StringPiece::npos);
  ASSERT_SIZE_EQ(e.find_last_not_of('\0'), StringPiece::npos);

  ASSERT_EQ(a.substr(0, 3), b);
  ASSERT_EQ(a.substr(23), c);
  ASSERT_EQ(a.substr(23, 3), c);
  ASSERT_EQ(a.substr(23, 99), c);
  ASSERT_EQ(a.substr(0), a);
  ASSERT_EQ(a.substr(3, 2), "de");
  // empty string nonsense
  ASSERT_EQ(a.substr(99, 2), e);
  ASSERT_EQ(d.substr(99), e);
  ASSERT_EQ(d.substr(0, 99), e);
  ASSERT_EQ(d.substr(99, 99), e);
}

TEST(StringPieceTest, CheckCustom) {
  StringPiece a("foobar");
  string s1("123");
  s1 += '\0';
  s1 += "456";
  StringPiece b(s1);
  StringPiece e;
  string s2;

  // CopyToString
  a.CopyToString(&s2);
  ASSERT_SIZE_EQ(s2.size(), 6);
  ASSERT_EQ(s2, "foobar");
  b.CopyToString(&s2);
  ASSERT_SIZE_EQ(s2.size(), 7);
  ASSERT_EQ(s1, s2);
  e.CopyToString(&s2);
  ASSERT_TRUE(s2.empty());

  // AppendToString
  s2.erase();
  a.AppendToString(&s2);
  ASSERT_SIZE_EQ(s2.size(), 6);
  ASSERT_EQ(s2, "foobar");
  a.AppendToString(&s2);
  ASSERT_SIZE_EQ(s2.size(), 12);
  ASSERT_EQ(s2, "foobarfoobar");

  // starts_with
  ASSERT_TRUE(a.starts_with(a));
  ASSERT_TRUE(a.starts_with("foo"));
  ASSERT_TRUE(a.starts_with(e));
  ASSERT_TRUE(b.starts_with(s1));
  ASSERT_TRUE(b.starts_with(b));
  ASSERT_TRUE(b.starts_with(e));
  ASSERT_TRUE(e.starts_with(""));
  ASSERT_TRUE(!a.starts_with(b));
  ASSERT_TRUE(!b.starts_with(a));
  ASSERT_TRUE(!e.starts_with(a));

  // ends with
  ASSERT_TRUE(a.ends_with(a));
  ASSERT_TRUE(a.ends_with("bar"));
  ASSERT_TRUE(a.ends_with(e));
  ASSERT_TRUE(b.ends_with(s1));
  ASSERT_TRUE(b.ends_with(b));
  ASSERT_TRUE(b.ends_with(e));
  ASSERT_TRUE(e.ends_with(""));
  ASSERT_TRUE(!a.ends_with(b));
  ASSERT_TRUE(!b.ends_with(a));
  ASSERT_TRUE(!e.ends_with(a));

  // remove_prefix
  StringPiece c(a);
  c.remove_prefix(3);
  ASSERT_EQ(c, "bar");
  c = a;
  c.remove_prefix(0);
  ASSERT_EQ(c, a);
  c.remove_prefix(c.size());
  ASSERT_EQ(c, e);

  // remove_suffix
  c = a;
  c.remove_suffix(3);
  ASSERT_EQ(c, "foo");
  c = a;
  c.remove_suffix(0);
  ASSERT_EQ(c, a);
  c.remove_suffix(c.size());
  ASSERT_EQ(c, e);

  // set
  c.set("foobar", 6);
  ASSERT_EQ(c, a);
  c.set("foobar", 0);
  ASSERT_EQ(c, e);
  c.set("foobar", 7);
  ASSERT_NE(c, a);

  c.set("foobar");
  ASSERT_EQ(c, a);

  c.set(static_cast<const void *>("foobar"), 6);
  ASSERT_EQ(c, a);
  c.set(static_cast<const void *>("foobar"), 0);
  ASSERT_EQ(c, e);
  c.set(static_cast<const void *>("foobar"), 7);
  ASSERT_NE(c, a);

  // as_string
  string s3(a.as_string().c_str(), 7);
  ASSERT_EQ(c, s3);
  string s4(e.as_string());
  ASSERT_TRUE(s4.empty());
}

TEST(StringPieceTest, CheckNULL) {
  // we used to crash here, but now we don't.
  StringPiece s(NULL);
  ASSERT_EQ(static_cast<const char *>(NULL), s.data());
  ASSERT_SIZE_EQ(s.size(), 0);

  s.set(NULL);
  ASSERT_EQ(static_cast<const char *>(NULL), s.data());
  ASSERT_SIZE_EQ(s.size(), 0);
}

TEST(StringPieceTest, CheckComparisons2) {
  StringPiece abc("abcdefghijklmnopqrstuvwxyz");

  // check comparison operations on strings longer than 4 bytes.
  ASSERT_TRUE(abc == StringPiece("abcdefghijklmnopqrstuvwxyz"));
  ASSERT_TRUE(abc.compare(StringPiece("abcdefghijklmnopqrstuvwxyz")) == 0);

  ASSERT_TRUE(abc < StringPiece("abcdefghijklmnopqrstuvwxzz"));
  ASSERT_TRUE(abc.compare(StringPiece("abcdefghijklmnopqrstuvwxzz")) < 0);

  ASSERT_TRUE(abc > StringPiece("abcdefghijklmnopqrstuvwxyy"));
  ASSERT_TRUE(abc.compare(StringPiece("abcdefghijklmnopqrstuvwxyy")) > 0);

  // starts_with
  ASSERT_TRUE(abc.starts_with(abc));
  ASSERT_TRUE(abc.starts_with("abcdefghijklm"));
  ASSERT_TRUE(!abc.starts_with("abcdefguvwxyz"));

  // ends_with
  ASSERT_TRUE(abc.ends_with(abc));
  ASSERT_TRUE(!abc.ends_with("abcdefguvwxyz"));
  ASSERT_TRUE(abc.ends_with("nopqrstuvwxyz"));
}

TEST(StringPieceTest, HeterogenousStringPieceEquals) {
  ASSERT_TRUE(StringPiece("hello") == string("hello"));
  ASSERT_TRUE("hello" == StringPiece("hello"));
}

TEST(StringPieceTest, Constructors) {
  // Tests for StringPiece(const char*, size_type)
  ASSERT_EQ("123", StringPiece("12345", 3));
  ASSERT_EQ("", StringPiece("12345", 0));
  ASSERT_EQ("12345", StringPiece("12345", 5));

  // Tests for StringPiece(const StringPiece, size_type)
  ASSERT_EQ("45", StringPiece(StringPiece("12345"), 3));
  ASSERT_EQ("12345", StringPiece(StringPiece("12345"), 0));
  ASSERT_EQ("", StringPiece(StringPiece("12345"), 5));

  // Tests for StringPiece(const StringPiece, size_type, size_type)
  ASSERT_EQ("234", StringPiece("12345", 1, 3));
  ASSERT_EQ("2345", StringPiece("12345", 1, 300));
  ASSERT_EQ("", StringPiece("12345", 1, 0));
}

}  // namespace mozc
