/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <array>

#include "DexClass.h"
#include "DexInstruction.h"
#include "DexLoader.h"
#include "Match.h"
#include "ProguardConfiguration.h"
#include "ProguardMap.h"
#include "ProguardMatcher.h"
#include "ProguardObfuscationTest.h"
#include "ProguardParser.h"
#include "ReachableClasses.h"
#include "RedexContext.h"

template<std::size_t SIZE>
void testClass(
    ProguardObfuscationTest* tester,
    const std::string& class_name,
    const std::array<std::string, SIZE>& fields) {
  auto clazz = tester->find_class_named(class_name);
  ASSERT_NE(nullptr, clazz) << class_name << " not found.";

  for (const std::string &fieldName : fields) {
    ASSERT_FALSE(tester->field_found(
        clazz->get_ifields(),
        class_name + fieldName)) << class_name + fieldName << " not obfuscated";
  }
}

/**
 * Check renaming has been properly applied.
 */
TEST(ProguardTest, obfuscation) {
  g_redex = new RedexContext();

  const char* dexfile = std::getenv("pg_config_e2e_dexfile");
  const char* mapping_file = std::getenv("pg_config_e2e_mapping");
  const char* configuration_file = std::getenv("pg_config_e2e_pgconfig");
  ASSERT_NE(nullptr, dexfile);
  ASSERT_NE(nullptr, configuration_file);

  ProguardObfuscationTest tester(dexfile, mapping_file);
  ASSERT_TRUE(tester.configure_proguard(configuration_file))
    << "Proguard configuration failed";

  // Make sure the fields class Alpha are renamed.
  const std::array<std::string, 4> alphaNames = {
    ".wombat:I",
    ".numbat:I",
    ".omega:Ljava/lang/String;",
    ".theta:Ljava/util/List;"};
  const std::array<std::string, 1> helloNames = {
    ".hello:Ljava/lang/String;" };
  const std::array<std::string, 1> worldNames = {
    ".world:Ljava/lang/String;" };
  testClass(&tester,
    "Lcom/facebook/redex/test/proguard/Alpha;",
    alphaNames);
  testClass(&tester,
    "Lcom/facebook/redex/test/proguard/Hello;",
    helloNames);
  testClass(&tester,
    "Lcom/facebook/redex/test/proguard/World;",
    worldNames);

  // Because of the all() call in Beta, there should be refs created in the
  // bytecode of all() to All.hello and All.world which should be updated
  // to Hello.[renamed] and World.[renamed]
  ASSERT_FALSE(tester.refs_to_field_found(helloNames[0]))
    << "Refs to " << helloNames[0] << " not properly modified";
  ASSERT_FALSE(tester.refs_to_field_found(worldNames[0]))
    << "Refs to " << worldNames[0] << " not properly modified";

  // Make sure the fields in the class Beta are not renamed.
  auto beta = tester.find_class_named(
    "Lcom/facebook/redex/test/proguard/Beta;");
  ASSERT_NE(nullptr, beta);
  ASSERT_TRUE(tester.field_found(
      beta->get_ifields(),
      "Lcom/facebook/redex/test/proguard/Beta;.wombatBeta:I"));

  delete g_redex;
}
