#ifndef YAMLIZER_DETAIL_READ_VALUE_H
#define YAMLIZER_DETAIL_READ_VALUE_H

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <boost/hana.hpp>
#include <boost/hana/ext/std/array.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include "yamlizer/yaml++.h"

namespace yamlizer {
namespace detail {

template <class T, std::enable_if_t<boost::hana::Foldable<T>::value, std::nullptr_t> = nullptr>
constexpr auto make_index_range() {
  using length_type = decltype(boost::hana::length(std::declval<T>()));
  return boost::hana::make_range(boost::hana::size_c<0>, length_type{});
}

template <class T, class = void>
struct has_push_back : std::false_type {};
template <class T>
struct has_push_back<T, decltype(static_cast<void>(std::declval<T>().push_back(
                            std::declval<typename T::value_type>())))> : std::true_type {};

template <class T, class = void>
struct has_emplace : std::false_type {};
template <class T>
struct has_emplace<
    T, std::enable_if_t<std::is_same<
           decltype(std::declval<T>().emplace(std::declval<typename T::value_type>())),
           std::pair<typename T::iterator, bool>>::value>>
    : std::true_type {};

template <class T, class = void>
struct is_key_value_container : std::false_type {};
template <class T>
struct is_key_value_container<
    T, std::enable_if_t<std::is_same<
           typename T::value_type,
           std::pair<std::add_const_t<typename T::key_type>, typename T::mapped_type>>::value>>
    : std::true_type {};

template <class Iterator>
bool check_token_type(::yaml_token_type_t type, Iterator begin, Iterator end) {
  if (begin >= end) {
    throw std::runtime_error("it >= end");
  }
  return begin->type() == type;
}

struct read_value_impl {
  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<T, std::string>::value,
                          std::tuple<T, Iterator>> {
    if (check_token_type(::YAML_SCALAR_TOKEN, begin, end)) {
      boost::cnv::lexical_cast cnv{};
      return std::forward_as_tuple(boost::convert<T>(begin->data().scalar.value, cnv).value(),
                                   std::next(begin));
    } else {
      throw std::runtime_error("token type != YAML_SCALAR_TOKEN");
    }
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<has_emplace<T>::value && is_key_value_container<T>::value,
                          std::tuple<T, Iterator>> {
    if (!check_token_type(::YAML_BLOCK_MAPPING_START_TOKEN, begin, end)) {
      throw std::runtime_error("token type != YAML_BLOCK_MAPPING_START_TOKEN");
    }

    T result{};
    for (auto it = std::next(begin);;) {
      if (check_token_type(::YAML_BLOCK_END_TOKEN, it, end)) {
        return std::forward_as_tuple(result, std::next(it));
      }

      if (!check_token_type(::YAML_KEY_TOKEN, it, end)) {
        throw std::runtime_error("token type != YAML_KEY_TOKEN");
      }
      const auto r1 = read_value_impl::apply<typename T::key_type>(std::next(it), end);

      if (!check_token_type(::YAML_VALUE_TOKEN, std::get<1>(r1), end)) {
        throw std::runtime_error("token type != YAML_VALUE_TOKEN");
      }
      const auto r2 =
          read_value_impl::apply<typename T::mapped_type>(std::next(std::get<1>(r1)), end);

      if (!std::get<1>(result.emplace(std::get<0>(r1), std::get<0>(r2)))) {
        throw std::runtime_error("failed to insert an object");
      }
      it = std::get<1>(r2);
    }
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Foldable<T>::value && boost::hana::Struct<T>::value,
                          std::tuple<T, Iterator>> {
    if (!check_token_type(::YAML_BLOCK_MAPPING_START_TOKEN, begin, end)) {
      throw std::runtime_error("token type != YAML_BLOCK_MAPPING_START_TOKEN");
    }

    const auto r2 = boost::hana::fold_left(
        boost::hana::keys(T{}), std::forward_as_tuple(T{}, std::next(begin)),
        [end](auto acc, auto key) {
          if (!check_token_type(::YAML_KEY_TOKEN, std::get<1>(acc), end)) {
            throw std::runtime_error("token type != YAML_KEY_TOKEN");
          }

          constexpr auto key_cstr = boost::hana::to<const char*>(key);
          const auto r21 =
              read_value_impl::apply<std::string>(std::next(std::get<1>(acc)), end);
          if (std::get<0>(r21) != key_cstr) {
            using namespace std::string_literals;
            throw std::runtime_error("key does not match: ["s + std::get<0>(r21) + " != "s +
                                     key_cstr + "]"s);
          }

          if (!check_token_type(::YAML_VALUE_TOKEN, std::get<1>(r21), end)) {
            throw std::runtime_error("token type != YAML_VALUE_TOKEN");
          }

          auto acc0      = std::get<0>(acc);
          const auto r22 = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at_key(acc0, key))>>(
              std::next(std::get<1>(r21)), end);
          boost::hana::at_key(acc0, key) = std::get<0>(r22);

          return std::make_tuple(acc0, std::get<1>(r22));
        });

    if (!check_token_type(::YAML_BLOCK_END_TOKEN, std::get<1>(r2), end)) {
      throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
    }
    return std::forward_as_tuple(std::get<0>(r2), std::next(std::get<1>(r2)));
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Foldable<T>::value && !boost::hana::Struct<T>::value,
                          std::tuple<T, Iterator>> {
    switch (begin->type()) {
      case ::YAML_BLOCK_SEQUENCE_START_TOKEN:
        return read_value_impl::read_block_sequence<T>(std::next(begin), end);

      case ::YAML_FLOW_SEQUENCE_START_TOKEN:
        return read_value_impl::read_flow_sequence<T>(std::next(begin), end);

      default:
        throw std::runtime_error(
            "t.type() != YAML_BLOCK_SEQUENCE_START_TOKEN || YAML_FLOW_SEQUENCE_START_TOKEN");
    }
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<has_push_back<T>::value && !std::is_same<T, std::string>::value,
                          std::tuple<T, Iterator>> {
    switch (begin->type()) {
      case ::YAML_BLOCK_SEQUENCE_START_TOKEN:
        return read_value_impl::read_block_sequence<T>(std::next(begin), end);

      case ::YAML_FLOW_SEQUENCE_START_TOKEN:
        return read_value_impl::read_flow_sequence<T>(std::next(begin), end);

      default:
        throw std::runtime_error(
            "t.type() != YAML_BLOCK_SEQUENCE_START_TOKEN || YAML_FLOW_SEQUENCE_START_TOKEN");
    }
  }

  template <class T, class Iterator>
  static auto read_block_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Foldable<T>::value, std::tuple<T, Iterator>> {
    const auto r1 = boost::hana::fold_left(
        make_index_range<T>(), std::forward_as_tuple(T{}, begin), [end](auto acc, auto key) {
          if (!check_token_type(::YAML_BLOCK_ENTRY_TOKEN, std::get<1>(acc), end)) {
            throw std::runtime_error("token type != YAML_BLOCK_ENTRY_TOKEN");
          }

          auto acc0      = std::get<0>(acc);
          const auto r11 = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at(acc0, key))>>(
              std::next(std::get<1>(acc)), end);
          boost::hana::at(acc0, key) = std::get<0>(r11);
          return std::make_tuple(acc0, std::get<1>(r11));
        });

    if (!check_token_type(::YAML_BLOCK_END_TOKEN, std::get<1>(r1), end)) {
      throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
    }
    return std::forward_as_tuple(std::get<0>(r1), std::next(std::get<1>(r1)));
  }

  template <class T, class Iterator>
  static auto read_block_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<has_push_back<T>::value, std::tuple<T, Iterator>> {
    T result{};
    for (auto it = begin;;) {
      if (it->type() == ::YAML_BLOCK_ENTRY_TOKEN) {
        const auto r = read_value_impl::apply<std::remove_reference_t<typename T::value_type>>(
            std::next(it), end);
        result.push_back(std::get<0>(r));
        it = std::get<1>(r);
      } else if (it->type() == ::YAML_BLOCK_END_TOKEN) {
        return std::forward_as_tuple(result, std::next(it));
      } else {
        throw std::runtime_error("invalid token type");
      }
    }
  }

  template <class T, class Iterator>
  static auto read_flow_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Foldable<T>::value, std::tuple<T, Iterator>> {
    return boost::hana::fold_left(
        make_index_range<T>(), std::forward_as_tuple(T{}, begin), [end](auto acc, auto key) {
          auto acc0 = std::get<0>(acc);
          auto r    = read_value_impl::apply<
              std::remove_reference_t<decltype(boost::hana::at(acc0, key))>>(std::get<1>(acc),
                                                                             end);
          boost::hana::at(acc0, key) = std::get<0>(r);

          if (std::get<1>(r)->type() != ::YAML_FLOW_ENTRY_TOKEN &&
              std::get<1>(r)->type() != ::YAML_FLOW_SEQUENCE_END_TOKEN) {
            throw std::runtime_error(
                "t.type() != YAML_FLOW_ENTRY_TOKEN || YAML_FLOW_SEQUENCE_END_TOKEN");
          }

          return std::make_tuple(acc0, std::next(std::get<1>(r)));
        });
  }

  template <class T, class Iterator>
  static auto read_flow_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<has_push_back<T>::value, std::tuple<T, Iterator>> {
    T result{};
    for (auto it = begin;;) {
      if (it->type() == ::YAML_FLOW_SEQUENCE_END_TOKEN) {
        return std::forward_as_tuple(result, std::next(it));
      }

      if (it->type() == ::YAML_FLOW_ENTRY_TOKEN) {
        it = std::next(it);
      }

      const auto r =
          read_value_impl::apply<std::remove_reference_t<typename T::value_type>>(it, end);
      result.push_back(std::get<0>(r));
      it = std::get<1>(r);
    }
  }
};

template <class T, class Iterator>
std::tuple<T, Iterator> read_value(Iterator begin, Iterator end) {
  if (!check_token_type(::YAML_STREAM_START_TOKEN, begin, end)) {
    throw std::runtime_error("token type != YAML_STREAM_START_TOKEN");
  }
  const auto r = read_value_impl::apply<T>(std::next(begin), end);
  if (!check_token_type(::YAML_STREAM_END_TOKEN, std::get<1>(r), end)) {
    throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
  }
  return std::forward_as_tuple(std::get<0>(r), std::next(std::get<1>(r)));
}

} // namespace detail
} // namespace yamlizer

#endif // YAMLIZER_DETAIL_READ_VALUE_H
