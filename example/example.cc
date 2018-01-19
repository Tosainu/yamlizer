#include <iostream>
#include "yamlizer/from_yaml.h"

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
