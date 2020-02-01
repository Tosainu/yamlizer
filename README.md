# yamlizer ![](https://github.com/Tosainu/yamlizer/workflows/CI/badge.svg)

YAML deserializer for C++17

## Requirements

- GCC 7.3.1+ or Clang 5.0.1+
- CMake 3.8+
- Boost 1.61.0+
  - Boost.Convert
  - Boost.Hana
  - Boost.Test
- libyaml 0.1.7+

## Example

### scalar

```cpp
const auto v1 = yamlizer::from_yaml<int>("123");
// => 123

const auto v2 = yamlizer::from_yaml<float>("1.23");
// => 1.23f
```

### string

```cpp
const auto s1 = yamlizer::from_yaml<std::string>("Hello, World!");
// => std::string{"Hello, World!"}

const auto s2 = yamlizer::from_yaml<std::wstring>("Hello, World!");
// => std::wstring{L"Hello, World!"}
```

### struct

```cpp
struct book {
  std::string name;
  int price;
};

BOOST_HANA_ADAPT_STRUCT(book, name, price);

const auto b = yamlizer::from_yaml<book>(R"EOS(
name: Gochumon wa Usagi Desuka ? Vol.1
price: 819
)EOS");
std::cout << b.name << std::endl;
// => Gochumon wa Usagi Desuka ? Vol.1
std::cout << b.price << " yen" << std::endl;
// => 819 yen
```

### containers

```cpp
const auto v1 = yamlizer::from_yaml<std::vector<int>>("[0, 1, 2, 3, 4, 5]");
// => std::vector<int>{0, 1, 2, 3, 4 ,5}

const auto m1 = yamlizer::from_yaml<std::map<std::string, int>>(R"EOS(
foo: 123
bar: 456
)EOS");
std::cout << m1.at("foo") << ' ' << m1.at("bar") << std::endl;
// => 123 456
```

### tuple

```cpp
const auto t = yamlizer::from_yaml<std::tuple<int, float, std::string>>(R"EOS(
- 123
- 1.23
- Hello, World!
)EOS");
// => std::tuple<int, float, std::string>{123, 1.23f, std::string{"Hello, World!"}}
```

## License

[MIT](https://github.com/Tosainu/yamlizer/blob/master/LICENSE)
