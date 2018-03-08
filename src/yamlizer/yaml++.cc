#include <stdexcept>
#include "yaml++.h"

namespace yamlizer {

token::token(::yaml_token_t t) : token_{std::move(t)} {}

token::~token() {
  ::yaml_token_delete(&token_);
}

token::token(token&& t) noexcept {
  std::swap(token_, t.token_);
}

token& token::operator=(token&& t) noexcept {
  std::swap(token_, t.token_);
  return *this;
}

::yaml_token_type_t token::type() const {
  return token_.type;
}

decltype(std::declval<::yaml_token_t>().data) token::data() const {
  return token_.data;
}

parser::parser(std::string_view buffer) : buffer_(buffer) {
  if (!::yaml_parser_initialize(&parser_)) {
    throw std::runtime_error("Failed to initialize YAML parser");
  }
  ::yaml_parser_set_input_string(
      &parser_, reinterpret_cast<const unsigned char*>(buffer_.data()), buffer_.length());
}

parser::~parser() {
  ::yaml_parser_delete(&parser_);
}

parser::parser(parser&& t) noexcept {
  std::swap(buffer_, t.buffer_);
  std::swap(parser_, t.parser_);
}

parser& parser::operator=(parser&& t) noexcept {
  std::swap(buffer_, t.buffer_);
  std::swap(parser_, t.parser_);
  return *this;
}

token parser::scan() {
  ::yaml_token_t t{};
  ::yaml_parser_scan(&parser_, &t);
  return {std::move(t)};
}

} // namespace yamlizer
