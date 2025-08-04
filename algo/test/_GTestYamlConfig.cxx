/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "gtest/gtest.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

using namespace cbm::algo;

//
// TODO: These tests are nowhere near exhaustive. Extend after the DC.
//

TEST(Config, CanDeserializeStdMap)
{
  using Map_t = std::map<i32, i32>;

  std::stringstream ss;
  ss << "1: 2\n";
  ss << "3: 4\n";
  ss << "5: 6\n";

  YAML::Node node = YAML::Load(ss.str());
  auto map        = yaml::Read<Map_t>(node);

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.at(1), 2);
  EXPECT_EQ(map.at(3), 4);
  EXPECT_EQ(map.at(5), 6);
}

TEST(Config, CanSerializeStdMap)
{
  using Map_t = std::map<i32, i32>;

  Map_t map;
  map[1] = 2;
  map[3] = 4;
  map[5] = 6;

  YAML::Node node = YAML::Load(yaml::Dump{}(map));

  EXPECT_EQ(node.size(), 3);
  EXPECT_EQ(node[1].as<i32>(), 2);
  EXPECT_EQ(node[3].as<i32>(), 4);
  EXPECT_EQ(node[5].as<i32>(), 6);
}

class Foo {
 private:
  i32 fBar = 42;
  i32 fBaz = 43;

 public:
  i32 GetBar() const { return fBar; }
  i32 GetBaz() const { return fBaz; }

  CBM_YAML_PROPERTIES(yaml::Property(&Foo::fBar, "bar", ""), yaml::Property(&Foo::fBaz, "baz", ""));
};

TEST(Config, CanAccessPrivateMembers)
{
  std::stringstream ss;
  ss << "bar: 1\n";
  ss << "baz: 2\n";

  YAML::Node node = YAML::Load(ss.str());
  auto foo        = yaml::Read<Foo>(node);

  EXPECT_EQ(foo.GetBar(), 1);
  EXPECT_EQ(foo.GetBaz(), 2);
}
