#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE yamlizer

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "yamlizer/yaml++.h"

BOOST_AUTO_TEST_CASE(yamlxx) {
  yamlizer::parser p{R"EOS(
foo: bar
)EOS"};
  yamlizer::token t{{}};

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_STREAM_START_TOKEN);

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_BLOCK_MAPPING_START_TOKEN);

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_KEY_TOKEN);

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_SCALAR_TOKEN);
  BOOST_TEST(reinterpret_cast<const char*>(t.data().scalar.value) == "foo");

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_VALUE_TOKEN);

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_SCALAR_TOKEN);
  BOOST_TEST(reinterpret_cast<const char*>(t.data().scalar.value) == "bar");

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_BLOCK_END_TOKEN);

  t = p.scan();
  BOOST_TEST(t.type() == ::YAML_STREAM_END_TOKEN);
}
