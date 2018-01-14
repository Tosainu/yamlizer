#ifndef YAMLIZER_FROM_YAML_H
#define YAMLIZER_FROM_YAML_H

#include "detail/read_value.h"
#include "yaml++.h"

namespace yamlizer {

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
