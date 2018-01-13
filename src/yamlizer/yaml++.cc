#include <stdexcept>
#include "yaml++.h"

namespace yamlizer {

token::~token() {
  ::yaml_token_delete(&token_);
}

::yaml_token_type_t token::type() const {
  return token_.type;
}

decltype(std::declval<::yaml_token_t>().data) token::data() const {
  return token_.data;
}

parser::parser(std::string buffer) : buffer_(std::move(buffer)) {
  if (!::yaml_parser_initialize(&parser_)) {
    throw std::runtime_error("Failed to initialize YAML parser");
  }
  ::yaml_parser_set_input_string(
      &parser_, reinterpret_cast<const unsigned char*>(buffer_.c_str()), buffer_.length());
}

parser::~parser() {
  ::yaml_parser_delete(&parser_);
}

token parser::scan() {
  ::yaml_token_t t{};
  ::yaml_parser_scan(&parser_, &t);
  return {std::move(t)};
}

} // namespace yamlizer
