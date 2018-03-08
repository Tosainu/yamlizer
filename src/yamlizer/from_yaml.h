#ifndef YAMLIZER_FROM_YAML_H
#define YAMLIZER_FROM_YAML_H

#include <string_view>
#include <vector>

#include "detail/read_value.h"
#include "yaml++.h"

namespace yamlizer {

template <class T>
T from_yaml(std::string_view yaml) {
  parser p{yaml};

  std::vector<token> ts{};
  for (auto prev_token = ::YAML_NO_TOKEN; prev_token != ::YAML_STREAM_END_TOKEN;) {
    auto t     = p.scan();
    prev_token = t.type();
    ts.emplace_back(std::move(t));
  }

  return std::get<0>(detail::read_value<T>(ts.cbegin(), ts.cend()));
}

} // namespace yamlizer

#endif // YAMLIZER_FROM_YAML_H
