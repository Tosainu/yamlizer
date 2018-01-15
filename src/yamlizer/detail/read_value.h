#ifndef YAMLIZER_DETAIL_READ_VALUE_H
#define YAMLIZER_DETAIL_READ_VALUE_H

#include <stdexcept>
#include <type_traits>
#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <boost/hana.hpp>
#include "yamlizer/yaml++.h"

namespace yamlizer {
namespace detail {

struct read_value_impl {
  template <class T>
  static auto apply(parser& p)
      -> std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<T, std::string>::value,
                          T> {
    const auto t = p.scan();
    if (t.type() == ::YAML_SCALAR_TOKEN) {
      boost::cnv::lexical_cast cnv{};
      return boost::convert<T>(t.data().scalar.value, cnv).value();
    } else {
      throw std::runtime_error("t.type() != YAML_SCALAR_TOKEN");
    }
  }

  template <class T>
  static auto apply(parser& p)
      -> std::enable_if_t<boost::hana::Foldable<T>::value && boost::hana::Struct<T>::value, T> {
    if (p.scan().type() != ::YAML_BLOCK_MAPPING_START_TOKEN) {
      throw std::runtime_error("t.type() != YAML_BLOCK_MAPPING_START_TOKEN");
    }

    const auto result =
        boost::hana::fold_left(boost::hana::keys(T{}), T{}, [&p](auto acc, auto key) {
          if (p.scan().type() != ::YAML_KEY_TOKEN) {
            throw std::runtime_error("t.type() != YAML_KEY_TOKEN");
          }

          const auto actual_key = read_value_impl::apply<std::string>(p);
          if (actual_key != key.c_str()) {
            using namespace std::string_literals;
            throw std::runtime_error("key does not match: ["s + actual_key + " != "s +
                                     key.c_str() + "]"s);
          }

          if (p.scan().type() != ::YAML_VALUE_TOKEN) {
            throw std::runtime_error("t.type() != YAML_VALUE_TOKEN");
          }

          boost::hana::at_key(acc, key) = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at_key(acc, key))>>(p);
          return acc;
        });

    if (p.scan().type() != ::YAML_BLOCK_END_TOKEN) {
      throw std::runtime_error("t.type() != YAML_BLOCK_END_TOKEN");
    }
    return result;
  }

  template <class T>
  static auto apply(parser& p)
      -> std::enable_if_t<boost::hana::Foldable<T>::value && !boost::hana::Struct<T>::value,
                          T> {
    switch (p.scan().type()) {
      case ::YAML_BLOCK_SEQUENCE_START_TOKEN:
        return read_value_impl::read_block_sequence<T>(p);

      case ::YAML_FLOW_SEQUENCE_START_TOKEN:
        return read_value_impl::read_flow_sequence<T>(p);

      default:
        throw std::runtime_error(
            "t.type() != YAML_BLOCK_SEQUENCE_START_TOKEN || YAML_FLOW_SEQUENCE_START_TOKEN");
    }
  }

  template <class T>
  static auto read_block_sequence(parser& p) {
    using length = decltype(boost::hana::length(std::declval<T>()));
    const auto result = boost::hana::fold_left(
        boost::hana::range_c<std::size_t, 0u, length::value>, T{}, [&p](auto acc, auto key) {
          if (p.scan().type() != ::YAML_BLOCK_ENTRY_TOKEN) {
            throw std::runtime_error("t.type() != YAML_BLOCK_ENTRY_TOKEN");
          }

          boost::hana::at(acc, key) = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at(acc, key))>>(p);
          return acc;
        });

    if (p.scan().type() != ::YAML_BLOCK_END_TOKEN) {
      throw std::runtime_error("YAML_BLOCK_END_TOKEN");
    }
    return result;
  }

  template <class T>
  static auto read_flow_sequence(parser& p) {
    using length = decltype(boost::hana::length(std::declval<T>()));
    return boost::hana::fold_left(
        boost::hana::range_c<std::size_t, 0u, length::value>, T{}, [&p](auto acc, auto key) {
          boost::hana::at(acc, key) = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at(acc, key))>>(p);

          const auto t = p.scan();
          if (t.type() != ::YAML_FLOW_ENTRY_TOKEN &&
              t.type() != ::YAML_FLOW_SEQUENCE_END_TOKEN) {
            throw std::runtime_error(
                "t.type() != YAML_FLOW_ENTRY_TOKEN || YAML_FLOW_SEQUENCE_END_TOKEN");
          }

          return acc;
        });
  }
};

template <class T>
T read_value(parser& p) {
  return read_value_impl::apply<T>(p);
}

} // namespace detail
} // namespace yamlizer

#endif // YAMLIZER_DETAIL_READ_VALUE_H
