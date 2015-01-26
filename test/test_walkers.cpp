#include <scope/test.h>

#include "walkers.h"

SCOPE_TEST(testMakeFileID) {
  SCOPE_ASSERT_EQUAL("0000", makeFileID(0, "", 0));
  SCOPE_ASSERT_EQUAL("0001", makeFileID(0, "", 1));
  SCOPE_ASSERT_EQUAL("010000", makeFileID(1, "00", 0));
}

SCOPE_TEST(testDirInfoNewChild) {
  DirInfo gpa,
          uncle = gpa.newChild("bob"),
          cousin = uncle.newChild("bob/joe"),
          dad = gpa.newChild("dad"),
          me = dad.newChild("me"),
          bro = dad.newChild("bro");

  SCOPE_ASSERT_EQUAL("0000", uncle.id());
  SCOPE_ASSERT_EQUAL("010000", cousin.id());
  SCOPE_ASSERT_EQUAL("0001", dad.id());
  SCOPE_ASSERT_EQUAL("010100", me.id());
  SCOPE_ASSERT_EQUAL("010101", bro.id());
}
