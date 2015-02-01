#include <scope/test.h>

#include "walkers.h"

SCOPE_TEST(testDirInfoNewChild) {
  DirInfo gpa;

  gpa.incCount();
  DirInfo uncle = gpa.newChild("bob");

  SCOPE_ASSERT_EQUAL("", gpa.path());
  SCOPE_ASSERT_EQUAL("bob", uncle.path());
  SCOPE_ASSERT_EQUAL(0, gpa.level());
  SCOPE_ASSERT_EQUAL(0, uncle.level());
  SCOPE_ASSERT_EQUAL(1, gpa.count());
  SCOPE_ASSERT_EQUAL(0, uncle.count());

  uncle.incCount();
  DirInfo cousin = uncle.newChild("bob/joe");

  SCOPE_ASSERT_EQUAL(1, uncle.count());

  gpa.incCount();
  DirInfo dad = gpa.newChild("dad");

  SCOPE_ASSERT_EQUAL("dad", dad.path());
  SCOPE_ASSERT_EQUAL(0, dad.level());
  SCOPE_ASSERT_EQUAL(0, dad.count());
  SCOPE_ASSERT_EQUAL(2, gpa.count());

  dad.incCount();
  DirInfo me = dad.newChild("me");

  SCOPE_ASSERT_EQUAL(1, dad.count());

  dad.incCount();
  DirInfo bro = dad.newChild("bro");

  SCOPE_ASSERT_EQUAL(2, dad.count());

  SCOPE_ASSERT_EQUAL("000000", uncle.id());

  SCOPE_ASSERT_EQUAL("00010000", cousin.id());

  SCOPE_ASSERT_EQUAL("000001", dad.id());

  SCOPE_ASSERT_EQUAL("00010100", me.id());

  SCOPE_ASSERT_EQUAL("00010101", bro.id());

  SCOPE_ASSERT_EQUAL("000001", gpa.lastChild());
  SCOPE_ASSERT_EQUAL("00010000", uncle.lastChild());
  SCOPE_ASSERT_EQUAL("00010101", dad.lastChild());
}

SCOPE_TEST(testMakeUnallocatedDataRun) {
  TSK_FS_ATTR_RUN         extent;
  TSK_DADDR_T             start(20),
                          end(30);

  SCOPE_ASSERT(MetadataWriter::makeUnallocatedDataRun(start, end, extent));

  SCOPE_ASSERT_EQUAL(start, extent.addr);
  SCOPE_ASSERT_EQUAL(10, extent.len);
  SCOPE_ASSERT_EQUAL(0, extent.offset);
}
