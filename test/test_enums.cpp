#include <scope/test.h>

#include "enums.h"

SCOPE_TEST(testMetaTypeLookup) {
  SCOPE_ASSERT_EQUAL("Undefined", metaType(0));
  SCOPE_ASSERT_EQUAL("File", metaType(1));
  SCOPE_ASSERT_EQUAL("Folder", metaType(2));
  SCOPE_ASSERT_EQUAL("Named Pipe", metaType(3));
  SCOPE_ASSERT_EQUAL("Character Device", metaType(4));
  SCOPE_ASSERT_EQUAL("Block Device", metaType(5));
  SCOPE_ASSERT_EQUAL("Symbolic Link", metaType(6));
  SCOPE_ASSERT_EQUAL("Shadow Inode", metaType(7));
  SCOPE_ASSERT_EQUAL("Domain Socket", metaType(8));
  SCOPE_ASSERT_EQUAL("Whiteout Inode", metaType(9));
  SCOPE_ASSERT_EQUAL("Virtual", metaType(10));
  SCOPE_ASSERT_EQUAL("Undefined", metaType(11));
}

// Seriously, same shit, different order
SCOPE_TEST(testNameTypeLookup) {
  SCOPE_ASSERT_EQUAL("Undefined", nameType(0));
  SCOPE_ASSERT_EQUAL("Named Pipe", nameType(1));
  SCOPE_ASSERT_EQUAL("Character Device", nameType(2));
  SCOPE_ASSERT_EQUAL("Folder", nameType(3));
  SCOPE_ASSERT_EQUAL("Block Device", nameType(4));
  SCOPE_ASSERT_EQUAL("File", nameType(5));
  SCOPE_ASSERT_EQUAL("Symbolic Link", nameType(6));
  SCOPE_ASSERT_EQUAL("Domain Socket", nameType(7));
  SCOPE_ASSERT_EQUAL("Shadow Inode", nameType(8));
  SCOPE_ASSERT_EQUAL("Whiteout Inode", nameType(9));
  SCOPE_ASSERT_EQUAL("Virtual", nameType(10));
  SCOPE_ASSERT_EQUAL("Undefined", nameType(11));
}

SCOPE_TEST(testMetaFlags) {
  SCOPE_ASSERT_EQUAL("Allocated", metaFlags(1));
  SCOPE_ASSERT_EQUAL("Deleted", metaFlags(2));
  SCOPE_ASSERT_EQUAL("Used", metaFlags(4));
  SCOPE_ASSERT_EQUAL("Unused", metaFlags(8));
  SCOPE_ASSERT_EQUAL("Compressed", metaFlags(16));
  SCOPE_ASSERT_EQUAL("Orphan", metaFlags(32));
  SCOPE_ASSERT_EQUAL("Deleted, Used", metaFlags(6));
}

SCOPE_TEST(testNameFlags) {
  SCOPE_ASSERT_EQUAL("Allocated", nameFlags(1));
  SCOPE_ASSERT_EQUAL("Deleted", nameFlags(2));
}

SCOPE_TEST(testAttrFlags) {
  SCOPE_ASSERT_EQUAL("", attrFlags(0));
  SCOPE_ASSERT_EQUAL("In Use", attrFlags(1));
  SCOPE_ASSERT_EQUAL("Non-resident", attrFlags(2));
  SCOPE_ASSERT_EQUAL("Resident", attrFlags(4));
  SCOPE_ASSERT_EQUAL("Encrypted", attrFlags(16));
  SCOPE_ASSERT_EQUAL("Compressed", attrFlags(32));
  SCOPE_ASSERT_EQUAL("Sparse", attrFlags(64));
  SCOPE_ASSERT_EQUAL("Recovered", attrFlags(128));
  SCOPE_ASSERT_EQUAL("In Use, Non-resident", attrFlags(3));
}
