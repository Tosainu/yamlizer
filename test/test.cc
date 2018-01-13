#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE yamlizer

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "yamlizer/from_yaml.h"
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

BOOST_AUTO_TEST_CASE(deserialize_scalar) {
  const auto v1 = yamlizer::from_yaml<int>("123");
  BOOST_TEST(v1 == 123);

  const auto v2 = yamlizer::from_yaml<float>("1.23");
  BOOST_TEST(v2 == 1.23f);

  const auto v3 = yamlizer::from_yaml<std::string>("Hello, World!");
  BOOST_TEST(v3 == "Hello, World!");

  BOOST_CHECK_THROW(yamlizer::from_yaml<int>("Hello, World!"), std::exception);
}

BOOST_AUTO_TEST_CASE(deserialize_struct) {
  struct book {
    BOOST_HANA_DEFINE_STRUCT(book, (std::string, name), (int, price));
  };

  const auto b1 = yamlizer::from_yaml<book>(R"EOS(
name: Gochumon wa Usagi Desuka ? Vol.1
price: 819
)EOS");
  BOOST_TEST(b1.name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(b1.price == 819);

  BOOST_CHECK_THROW(yamlizer::from_yaml<book>("name: Gochumon wa Usagi Desuka ? Vol.1"),
                    std::exception);
}
