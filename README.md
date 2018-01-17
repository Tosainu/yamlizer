# yamlizer [![wercker status][wercker-badge]][wercker-runs]

YAML deserializer for C++14

## Example

```cpp
struct book {
  std::string name;
  int price;
};

BOOST_HANA_ADAPT_STRUCT(book, name, price);

auto main() -> int {
  const auto b = yamlizer::from_yaml<book>(R"EOS(
name: Gochumon wa Usagi Desuka ? Vol.1
price: 819
)EOS");

  std::cout << b.name << std::endl;
  // => Gochumon wa Usagi Desuka ? Vol.1
  std::cout << b.price << " yen" << std::endl;
  // => 819 yen
}
```

## License

[MIT](https://github.com/Tosainu/yamlizer/blob/master/LICENSE)

[wercker-badge]: https://app.wercker.com/status/16e2b290ac7a4de24e210c44fe57f3ff/s/master
[wercker-runs]:  https://app.wercker.com/project/byKey/16e2b290ac7a4de24e210c44fe57f3ff
