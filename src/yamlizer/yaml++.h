#ifndef YAMLIZER_YAMLXX_H
#define YAMLIZER_YAMLXX_H

#include <iterator>
#include <string>
#include <string_view>
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

  token(token&& t) noexcept;
  token& operator=(token&& t) noexcept;

  ::yaml_token_type_t type() const;
  decltype(std::declval<::yaml_token_t>().data) data() const;
};

class parser final {
  std::string_view buffer_;
  ::yaml_parser_t parser_;

public:
  parser(std::string_view buffer);
  ~parser();

  parser(const parser&) = delete;
  parser& operator=(const parser&) = delete;

  parser(parser&&) noexcept;
  parser& operator=(parser&&) noexcept;

  token scan();
};

static constexpr std::string_view token_type_to_string(::yaml_token_type_t type) noexcept {
  using namespace std::literals::string_view_literals;

#define ENUM_ITEM(NAME) [::NAME] = #NAME##sv
  constexpr std::string_view t[]{
      ENUM_ITEM(YAML_NO_TOKEN),
      ENUM_ITEM(YAML_STREAM_START_TOKEN),
      ENUM_ITEM(YAML_STREAM_END_TOKEN),
      ENUM_ITEM(YAML_VERSION_DIRECTIVE_TOKEN),
      ENUM_ITEM(YAML_TAG_DIRECTIVE_TOKEN),
      ENUM_ITEM(YAML_DOCUMENT_START_TOKEN),
      ENUM_ITEM(YAML_DOCUMENT_END_TOKEN),
      ENUM_ITEM(YAML_BLOCK_SEQUENCE_START_TOKEN),
      ENUM_ITEM(YAML_BLOCK_MAPPING_START_TOKEN),
      ENUM_ITEM(YAML_BLOCK_END_TOKEN),
      ENUM_ITEM(YAML_FLOW_SEQUENCE_START_TOKEN),
      ENUM_ITEM(YAML_FLOW_SEQUENCE_END_TOKEN),
      ENUM_ITEM(YAML_FLOW_MAPPING_START_TOKEN),
      ENUM_ITEM(YAML_FLOW_MAPPING_END_TOKEN),
      ENUM_ITEM(YAML_BLOCK_ENTRY_TOKEN),
      ENUM_ITEM(YAML_FLOW_ENTRY_TOKEN),
      ENUM_ITEM(YAML_KEY_TOKEN),
      ENUM_ITEM(YAML_VALUE_TOKEN),
      ENUM_ITEM(YAML_ALIAS_TOKEN),
      ENUM_ITEM(YAML_ANCHOR_TOKEN),
      ENUM_ITEM(YAML_TAG_TOKEN),
      ENUM_ITEM(YAML_SCALAR_TOKEN),
  };
#undef ENNUM_ITEM

  if (static_cast<std::size_t>(type) < std::size(t)) {
    return t[type];
  } else {
    return std::string_view{};
  }
}

} // namespace yamlizer

#endif // YAMLIZER_YAMLXX_H
