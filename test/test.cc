#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE yamlizer

#include <array>
#include <iostream>
#include <boost/hana.hpp>
#include <boost/hana/ext/std/array.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/test/unit_test.hpp>
#include "yamlizer/from_yaml.h"
#include "yamlizer/yaml++.h"

struct book {
  BOOST_HANA_DEFINE_STRUCT(book, (std::string, name), (int, price));
};

struct string3 {
  BOOST_HANA_DEFINE_STRUCT(string3, (std::array<std::string, 3>, strings));
};

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
  const auto b1 = yamlizer::from_yaml<book>(R"EOS(
name: Gochumon wa Usagi Desuka ? Vol.1
price: 819
)EOS");
  BOOST_TEST(b1.name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(b1.price == 819);

  BOOST_CHECK_THROW(yamlizer::from_yaml<book>("name: Gochumon wa Usagi Desuka ? Vol.1"),
                    std::exception);
}

BOOST_AUTO_TEST_CASE(deserialize_block_sequence_of_scalar) {
  const auto a = yamlizer::from_yaml<std::array<int, 3>>(R"EOS(
- 1
- 2
- 3
)EOS");
  BOOST_TEST(a.at(0) == 1);
  BOOST_TEST(a.at(1) == 2);
  BOOST_TEST(a.at(2) == 3);

  const auto t = yamlizer::from_yaml<std::tuple<int, float, std::string>>(R"EOS(
- 123
- 1.23
- Hello, World!
)EOS");
  BOOST_TEST(std::get<0>(t) == 123);
  BOOST_TEST(std::get<1>(t) == 1.23f);
  BOOST_TEST(std::get<2>(t) == "Hello, World!");
}

BOOST_AUTO_TEST_CASE(deserialize_block_sequence_of_mapping) {
  const auto m = yamlizer::from_yaml<std::array<book, 2>>(R"EOS(
- name: Gochumon wa Usagi Desuka ? Vol.1
  price: 819
- name: Anne Happy Vol.1
  price: 590
)EOS");

  BOOST_TEST(m.at(0).name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(m.at(0).price == 819);
  BOOST_TEST(m.at(1).name == "Anne Happy Vol.1");
  BOOST_TEST(m.at(1).price == 590);
}

BOOST_AUTO_TEST_CASE(deserialize_mapping_of_block_sequence) {
  const auto a = yamlizer::from_yaml<string3>(R"EOS(
strings:
  - foo
  - bar
  - baz
)EOS");
  BOOST_TEST(a.strings.at(0) == "foo");
  BOOST_TEST(a.strings.at(1) == "bar");
  BOOST_TEST(a.strings.at(2) == "baz");
}

BOOST_AUTO_TEST_CASE(deserialize_flow_sequence_of_scalar) {
  const auto a = yamlizer::from_yaml<std::array<int, 3>>(R"EOS([1, 2, 3])EOS");
  BOOST_TEST(a.at(0) == 1);
  BOOST_TEST(a.at(1) == 2);
  BOOST_TEST(a.at(2) == 3);
}

BOOST_AUTO_TEST_CASE(deserialize_mapping_of_flow_sequence) {
  const auto a = yamlizer::from_yaml<string3>(R"EOS(strings: [foo, bar, baz])EOS");
  BOOST_TEST(a.strings.at(0) == "foo");
  BOOST_TEST(a.strings.at(1) == "bar");
  BOOST_TEST(a.strings.at(2) == "baz");
}
