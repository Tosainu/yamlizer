#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE yamlizer

#include <array>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <boost/hana.hpp>
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

  BOOST_CHECK_THROW(yamlizer::from_yaml<int>("Hello, World!"), std::exception);
}

BOOST_AUTO_TEST_CASE(deserialize_string) {
  const auto s1 = yamlizer::from_yaml<std::string>("Hello, World!");
  BOOST_TEST(s1.compare("Hello, World!") == 0);

  const auto s2 = yamlizer::from_yaml<std::wstring>("Hello, World!");
  BOOST_TEST(s2.compare(L"Hello, World!") == 0);
}

BOOST_AUTO_TEST_CASE(deserialize_pair) {
  const auto p1 = yamlizer::from_yaml<std::pair<std::string, int>>("foo: 123");
  BOOST_TEST(std::get<0>(p1) == "foo");
  BOOST_TEST(std::get<1>(p1) == 123);

  const auto p2 = yamlizer::from_yaml<std::pair<std::string, int>>(R"EOS(
{
  foo: 123
}
)EOS");
  BOOST_TEST(std::get<0>(p2) == "foo");
  BOOST_TEST(std::get<1>(p2) == 123);
}

BOOST_AUTO_TEST_CASE(deserialize_struct) {
  const auto b1 = yamlizer::from_yaml<book>(R"EOS(
name: Gochumon wa Usagi Desuka ? Vol.1
price: 819
)EOS");
  BOOST_TEST(b1.name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(b1.price == 819);

  const auto b2 = yamlizer::from_yaml<book>(R"EOS(
{
  name: 'Gochumon wa Usagi Desuka ? Vol.1',
  price: 819
}
)EOS");
  BOOST_TEST(b2.name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(b2.price == 819);

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

  BOOST_CHECK_NO_THROW(yamlizer::from_yaml<std::tuple<>>("[]"));
}

BOOST_AUTO_TEST_CASE(deserialize_mapping_of_flow_sequence) {
  const auto a = yamlizer::from_yaml<string3>(R"EOS(strings: [foo, bar, baz])EOS");
  BOOST_TEST(a.strings.at(0) == "foo");
  BOOST_TEST(a.strings.at(1) == "bar");
  BOOST_TEST(a.strings.at(2) == "baz");
}

BOOST_AUTO_TEST_CASE(deserialize_block_sequence_of_scalar_to_std_vector) {
  const auto v = yamlizer::from_yaml<std::vector<int>>(R"EOS(
- 0
- 1
- 2
- 3
- 4
- 5
)EOS");
  BOOST_TEST(v.size() == 6u);
  BOOST_TEST(v[0] == 0);
  BOOST_TEST(v[1] == 1);
  BOOST_TEST(v[2] == 2);
  BOOST_TEST(v[3] == 3);
  BOOST_TEST(v[4] == 4);
  BOOST_TEST(v[5] == 5);
}

BOOST_AUTO_TEST_CASE(deserialize_flow_sequence_of_scalar_to_std_vector) {
  const auto v1 = yamlizer::from_yaml<std::vector<int>>("[0, 1, 2, 3, 4, 5]");
  BOOST_TEST(v1.size() == 6u);
  BOOST_TEST(v1[0] == 0);
  BOOST_TEST(v1[1] == 1);
  BOOST_TEST(v1[2] == 2);
  BOOST_TEST(v1[3] == 3);
  BOOST_TEST(v1[4] == 4);
  BOOST_TEST(v1[5] == 5);

  const auto v2 = yamlizer::from_yaml<std::vector<int>>("[]");
  BOOST_TEST(v2.size() == 0u);
}

BOOST_AUTO_TEST_CASE(deserialize_key_value_container) {
  const auto m1 = yamlizer::from_yaml<std::map<std::string, int>>(R"EOS(
foo: 123
bar: 456
)EOS");
  BOOST_TEST(m1.size() == 2u);
  BOOST_TEST(m1.at("foo") == 123);
  BOOST_TEST(m1.at("bar") == 456);

  using msi = std::map<std::string, int>;
  BOOST_CHECK_THROW(yamlizer::from_yaml<msi>("foo: 123\nfoo: 456"), std::exception);

  const auto m2 = yamlizer::from_yaml<std::unordered_map<std::string, int>>(R"EOS(
foo: 123
bar: 456
)EOS");
  BOOST_TEST(m2.size() == 2u);
  BOOST_TEST(m2.at("foo") == 123);
  BOOST_TEST(m2.at("bar") == 456);

  const auto m3 = yamlizer::from_yaml<std::map<std::string, book>>(R"EOS(
book1:
  name: Gochumon wa Usagi Desuka ? Vol.1
  price: 819
book2:
  name: Anne Happy Vol.1
  price: 590
)EOS");
  BOOST_TEST(m3.size() == 2u);
  BOOST_TEST(m3.at("book1").name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(m3.at("book1").price == 819);
  BOOST_TEST(m3.at("book2").name == "Anne Happy Vol.1");
  BOOST_TEST(m3.at("book2").price == 590);

  const auto m4 = yamlizer::from_yaml<std::unordered_map<std::string, int>>(R"EOS(
{
  foo: 123,
  bar: 456
}
)EOS");
  BOOST_TEST(m4.size() == 2u);
  BOOST_TEST(m4.at("foo") == 123);
  BOOST_TEST(m4.at("bar") == 456);

  const auto m5 = yamlizer::from_yaml<std::map<std::string, book>>(R"EOS(
book1: {
    name: 'Gochumon wa Usagi Desuka ? Vol.1',
    price: 819
  }
book2: {
    name: 'Anne Happy Vol.1',
    price: 590
  }
)EOS");
  BOOST_TEST(m5.size() == 2u);
  BOOST_TEST(m5.at("book1").name == "Gochumon wa Usagi Desuka ? Vol.1");
  BOOST_TEST(m5.at("book1").price == 819);
  BOOST_TEST(m5.at("book2").name == "Anne Happy Vol.1");
  BOOST_TEST(m5.at("book2").price == 590);
}

BOOST_AUTO_TEST_CASE(deserialize_optional1) {
  const auto o1 = yamlizer::from_yaml<std::optional<int>>("");
  BOOST_TEST(!o1);
  const auto o2 = yamlizer::from_yaml<std::optional<int>>("123");
  BOOST_TEST(*o2 == 123);
}

BOOST_AUTO_TEST_CASE(deserialize_optional2) {
  struct optional_struct {
    BOOST_HANA_DEFINE_STRUCT(optional_struct, (std::optional<int>, v1), (std::string, v2));
  };

  const auto o1 = yamlizer::from_yaml<optional_struct>(R"eos(
v1: 123
v2: poe
)eos");
  BOOST_TEST(o1.v1.value() == 123);
  BOOST_TEST(o1.v2 == "poe");

  const auto o2 = yamlizer::from_yaml<optional_struct>(R"eos(
v2: poe
)eos");
  BOOST_TEST(!o2.v1);
  BOOST_TEST(o2.v2 == "poe");

  BOOST_CHECK_THROW(yamlizer::from_yaml<optional_struct>("v1: fee\nv2: poe"), std::exception);
}
