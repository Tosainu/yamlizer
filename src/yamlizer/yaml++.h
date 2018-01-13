#ifndef YAMLIZER_YAMLXX_H
#define YAMLIZER_YAMLXX_H

#include <string>
#include <utility>
#include <yaml.h>

namespace yamlizer {

class token final {
  ::yaml_token_t token_;

public:
  token(::yaml_token_t t);
  ~token();

  token(const token&) = delete;
  token& operator=(const token&) = delete;

  token(token&& t);
  token& operator=(token&& t);

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

  parser(parser&&);
  parser& operator=(parser&&);

  token scan();
};

} // namespace yamlizer

#endif // YAMLIZER_YAMLXX_H
