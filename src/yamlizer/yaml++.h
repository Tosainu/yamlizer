#ifndef YAMLIZER_YAMLXX_H
#define YAMLIZER_YAMLXX_H

#include <string>
#include <utility>
#include <yaml.h>

namespace yamlizer {

class token {
  ::yaml_token_t token_;

public:
  token(::yaml_token_t t) : token_{std::move(t)} {}
  ~token();

  token(const token&) = delete;
  token& operator=(const token&) = delete;

  token(token&&) = default;
  token& operator=(token&&) = default;

  ::yaml_token_type_t type() const;
  decltype(std::declval<::yaml_token_t>().data) data() const;
};

class parser final {
  std::string buffer_;
  ::yaml_parser_t parser_;

public:
  parser(std::string buffer);
  ~parser();

  parser(const parser&) = delete;
  parser& operator=(const parser&) = delete;

  parser(parser&&) = default;
  parser& operator=(parser&&) = default;

  token scan();
};

} // namespace yamlizer

#endif // YAMLIZER_YAMLXX_H
