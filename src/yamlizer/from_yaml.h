#ifndef YAMLIZER_FROM_YAML_H
#define YAMLIZER_FROM_YAML_H

#include <stdexcept>
#include <type_traits>
#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <boost/hana.hpp>
#include "yaml++.h"

namespace yamlizer {

namespace detail {

template <class T>
auto read_value(parser& p)
    -> std::enable_if_t<std::is_scalar<T>::value || std::is_same<T, std::string>::value, T> {
  const auto t = p.scan();
  if (t.type() == ::YAML_SCALAR_TOKEN) {
    boost::cnv::lexical_cast cnv{};
    return boost::convert<T>(t.data().scalar.value, cnv).value();
  } else {
    throw std::runtime_error("t.type() != YAML_SCALAR_TOKEN");
  }
}

template <class T>
auto read_value(parser& p)
    -> std::enable_if_t<boost::hana::Foldable<T>::value && boost::hana::Struct<T>::value, T> {
  if (p.scan().type() != ::YAML_BLOCK_MAPPING_START_TOKEN) {
    throw std::runtime_error("t.type() != YAML_BLOCK_MAPPING_START_TOKEN");
  }

  const auto result =
      boost::hana::fold_left(boost::hana::keys(T{}), T{}, [&p](auto acc, auto key) {
        if (p.scan().type() != ::YAML_KEY_TOKEN) {
          throw std::runtime_error("t.type() != YAML_KEY_TOKEN");
        }

        const auto actual_key = detail::read_value<std::string>(p);
        if (actual_key != key.c_str()) {
          using namespace std::string_literals;
          throw std::runtime_error("key does not match: ["s + actual_key + " != "s +
                                   key.c_str() + "]"s);
        }

        if (p.scan().type() != ::YAML_VALUE_TOKEN) {
          throw std::runtime_error("t.type() != YAML_VALUE_TOKEN");
        }

        boost::hana::at_key(acc, key) =
            read_value<std::remove_reference_t<decltype(boost::hana::at_key(acc, key))>>(p);
        return acc;
      });

  if (p.scan().type() != ::YAML_BLOCK_END_TOKEN) {
    throw std::runtime_error("t.type() != YAML_BLOCK_END_TOKEN");
  }
  return result;
}

} // namespace detail

template <class T>
T from_yaml(std::string yaml) {
  parser p{std::move(yaml)};
  token t{{}};

  t = p.scan();
  if (t.type() != ::YAML_STREAM_START_TOKEN) {
    throw std::runtime_error("t.type() != YAML_STREAM_START_TOKEN");
  }

  const auto result = detail::read_value<T>(p);

  t = p.scan();
  if (t.type() != ::YAML_STREAM_END_TOKEN) {
    throw std::runtime_error("t.type() != YAML_STREAM_END_TOKEN");
  }

  return result;
}

} // namespace yamlizer

#endif // YAMLIZER_FROM_YAML_H
