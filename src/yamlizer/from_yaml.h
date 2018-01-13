#ifndef YAMLIZER_FROM_YAML_H
#define YAMLIZER_FROM_YAML_H

#include <stdexcept>
#include <type_traits>
#include <boost/convert.hpp>
#include <boost/convert/lexical_cast.hpp>
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
