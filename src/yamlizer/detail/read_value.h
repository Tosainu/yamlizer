#ifndef YAMLIZER_DETAIL_READ_VALUE_H
#define YAMLIZER_DETAIL_READ_VALUE_H

#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <boost/hana.hpp>
#include <boost/hana/ext/std/array.hpp>
#include <boost/hana/ext/std/pair.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include "yamlizer/yaml++.h"

// __has_include is c++17's feature but most compilers support it
#ifdef __has_include
#  if __has_include(<optional>) && __cplusplus > 201402L
#    define YAMLIZER_HAS_STD_OPTIONAL
#    include <optional>
#  endif
#  if __has_include(<experimental/optional>) && __cplusplus >= 201402L
#    define YAMLIZER_HAS_STD_EXPELIMENTAL_OPTIONAL
#    include <experimental/optional>
#  endif
#endif

namespace yamlizer {
namespace detail {

template <class T, std::enable_if_t<boost::hana::Foldable<T>::value, std::nullptr_t> = nullptr>
constexpr auto make_index_range() {
  using length_type = decltype(boost::hana::length(std::declval<T>()));
  return boost::hana::make_range(boost::hana::size_c<0>, length_type{});
}

template <class T, class = void>
struct has_emplace_back : std::false_type {};
template <class T>
struct has_emplace_back<T, decltype(static_cast<void>(std::declval<T>().emplace_back(
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

template <class T>
struct is_string : std::false_type {};
template <>
struct is_string<std::string> : std::true_type {};
template <>
struct is_string<std::wstring> : std::true_type {};

template <class T>
struct is_optional : std::false_type {};
#ifdef YAMLIZER_HAS_STD_OPTIONAL
template <class T>
struct is_optional<std::optional<T>> : std::true_type {};
#endif
#ifdef YAMLIZER_HAS_STD_EXPELIMENTAL_OPTIONAL
template <class T>
struct is_optional<std::experimental::optional<T>> : std::true_type {};
#endif

// http://en.cppreference.com/w/cpp/types/remove_cvref
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

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
      -> std::enable_if_t<std::is_arithmetic<T>::value || is_string<T>::value,
                          std::tuple<T, Iterator>> {
    if (check_token_type(::YAML_SCALAR_TOKEN, begin, end)) {
      boost::cnv::lexical_cast cnv{};
      return std::make_tuple(boost::convert<T>(begin->data().scalar.value, cnv).value(),
                             std::next(begin));
    } else {
      throw std::runtime_error("token type != YAML_SCALAR_TOKEN");
    }
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Product<T>::value, std::tuple<T, Iterator>> {
    if (!check_token_type(::YAML_BLOCK_MAPPING_START_TOKEN, begin, end)) {
      throw std::runtime_error("token type != YAML_BLOCK_MAPPING_START_TOKEN");
    }

    const auto r = read_value_impl::read_key_value<T>(std::next(begin), end);

    if (!check_token_type(::YAML_BLOCK_END_TOKEN, std::get<1>(r), end)) {
      throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
    }
    return std::make_tuple(std::get<0>(r), std::next(std::get<1>(r)));
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<is_optional<T>::value, std::tuple<T, Iterator>> {
    try {
      return read_value_impl::apply<typename T::value_type>(begin, end);
    } catch (...) {
      return std::make_tuple(T{}, begin);
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
        return std::make_tuple(result, std::next(it));
      }

      const auto r = read_value_impl::read_key_value<typename T::value_type>(it, end);

      if (!std::get<1>(result.emplace(std::get<0>(r)))) {
        throw std::runtime_error("failed to insert an object");
      }
      it = std::get<1>(r);
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
          auto acc0        = std::get<0>(acc);
          using value_type = remove_cvref_t<decltype(boost::hana::at_key(acc0, key))>;

          const auto r =
              read_value_impl::read_struct_member<value_type>(std::get<1>(acc), end, key);
          boost::hana::at_key(acc0, key) = std::get<0>(r);
          return std::make_tuple(acc0, std::get<1>(r));
        });

    if (!check_token_type(::YAML_BLOCK_END_TOKEN, std::get<1>(r2), end)) {
      throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
    }
    return std::make_tuple(std::get<0>(r2), std::next(std::get<1>(r2)));
  }

  template <class T, class Iterator>
  static auto apply(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Foldable<T>::value && !boost::hana::Product<T>::value &&
                              !boost::hana::Struct<T>::value,
                          std::tuple<T, Iterator>> {
    if (begin >= end) {
      throw std::runtime_error("it >= end");
    }

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
      -> std::enable_if_t<has_emplace_back<T>::value && !is_string<T>::value,
                          std::tuple<T, Iterator>> {
    if (begin >= end) {
      throw std::runtime_error("it >= end");
    }

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

          auto acc0        = std::get<0>(acc);
          using value_type = remove_cvref_t<decltype(boost::hana::at(acc0, key))>;
          const auto r11 = read_value_impl::apply<value_type>(std::next(std::get<1>(acc)), end);
          boost::hana::at(acc0, key) = std::get<0>(r11);
          return std::make_tuple(acc0, std::get<1>(r11));
        });

    if (!check_token_type(::YAML_BLOCK_END_TOKEN, std::get<1>(r1), end)) {
      throw std::runtime_error("token type != YAML_BLOCK_END_TOKEN");
    }
    return std::make_tuple(std::get<0>(r1), std::next(std::get<1>(r1)));
  }

  template <class T, class Iterator>
  static auto read_block_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<has_emplace_back<T>::value, std::tuple<T, Iterator>> {
    T result{};
    for (auto it = begin;;) {
      if (check_token_type(::YAML_BLOCK_ENTRY_TOKEN, it, end)) {
        const auto r = read_value_impl::apply<typename T::value_type>(std::next(it), end);
        result.emplace_back(std::get<0>(r));
        it = std::get<1>(r);
      } else if (check_token_type(::YAML_BLOCK_END_TOKEN, it, end)) {
        return std::make_tuple(result, std::next(it));
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
          auto acc0        = std::get<0>(acc);
          using value_type = remove_cvref_t<decltype(boost::hana::at(acc0, key))>;
          auto r           = read_value_impl::apply<value_type>(std::get<1>(acc), end);
          boost::hana::at(acc0, key) = std::get<0>(r);

          if (!(check_token_type(::YAML_FLOW_ENTRY_TOKEN, std::get<1>(r), end) ||
                check_token_type(::YAML_FLOW_SEQUENCE_END_TOKEN, std::get<1>(r), end))) {
            throw std::runtime_error(
                "t.type() != YAML_FLOW_ENTRY_TOKEN || YAML_FLOW_SEQUENCE_END_TOKEN");
          }

          return std::make_tuple(acc0, std::next(std::get<1>(r)));
        });
  }

  template <class T, class Iterator>
  static auto read_flow_sequence(Iterator begin, Iterator end)
      -> std::enable_if_t<has_emplace_back<T>::value, std::tuple<T, Iterator>> {
    T result{};
    for (auto it = begin;;) {
      if (check_token_type(::YAML_FLOW_SEQUENCE_END_TOKEN, it, end)) {
        return std::make_tuple(result, std::next(it));
      }

      if (check_token_type(::YAML_FLOW_ENTRY_TOKEN, it, end)) {
        it = std::next(it);
      }

      const auto r = read_value_impl::apply<typename T::value_type>(it, end);
      result.emplace_back(std::get<0>(r));
      it = std::get<1>(r);
    }
  }

  template <class T, class Iterator, class Key>
  static auto read_struct_member(Iterator begin, Iterator end, Key key)
      -> std::enable_if_t<is_optional<T>::value, std::tuple<T, Iterator>> {
    if (!check_token_type(::YAML_KEY_TOKEN, begin, end)) {
      throw std::runtime_error("token type != YAML_KEY_TOKEN");
    }
    const auto r = read_value_impl::apply<std::string>(std::next(begin), end);

    const auto actual_key   = std::get<0>(r);
    constexpr auto key_cstr = boost::hana::to<const char*>(key);
    if (actual_key != key_cstr) {
      return std::make_tuple(T{}, begin);
    }

    return read_value_impl::read_struct_member<typename T::value_type>(begin, end, key);
  }

  template <class T, class Iterator, class Key>
  static auto read_struct_member(Iterator begin, Iterator end, Key key)
      -> std::enable_if_t<!is_optional<T>::value, std::tuple<T, Iterator>> {
    const auto r =
        read_value_impl::read_key_value<boost::hana::pair<std::string, T>>(begin, end);

    const auto actual_key   = boost::hana::first(std::get<0>(r));
    constexpr auto key_cstr = boost::hana::to<const char*>(key);
    if (actual_key != key_cstr) {
      using namespace std::string_literals;
      throw std::runtime_error("key does not match: ["s + actual_key + " != "s + key_cstr +
                               "]"s);
    } else {
      return std::make_tuple(boost::hana::second(std::get<0>(r)), std::get<1>(r));
    }
  }

  template <class T, class Iterator>
  static auto read_key_value(Iterator begin, Iterator end)
      -> std::enable_if_t<boost::hana::Product<T>::value, std::tuple<T, Iterator>> {
    if (!check_token_type(::YAML_KEY_TOKEN, begin, end)) {
      throw std::runtime_error("token type != YAML_KEY_TOKEN");
    }
    using key_type = remove_cvref_t<decltype(boost::hana::first(std::declval<T>()))>;
    const auto key = read_value_impl::apply<key_type>(std::next(begin), end);

    if (!check_token_type(::YAML_VALUE_TOKEN, std::get<1>(key), end)) {
      throw std::runtime_error("token type != YAML_VALUE_TOKEN");
    }
    using value_type = remove_cvref_t<decltype(boost::hana::second(std::declval<T>()))>;
    const auto value = read_value_impl::apply<value_type>(std::next(std::get<1>(key)), end);

    return std::make_tuple(boost::hana::make<T>(std::get<0>(key), std::get<0>(value)),
                           std::get<1>(value));
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
  return std::make_tuple(std::get<0>(r), std::next(std::get<1>(r)));
}

} // namespace detail
} // namespace yamlizer

#endif // YAMLIZER_DETAIL_READ_VALUE_H
