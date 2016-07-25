#include <cstdint>

namespace tcp_messenger {
  //cyclic shift bitwise operations
  std::uint32_t rotl(std::uint32_t v, std::int32_t shift);
  std::uint32_t rotr(std::uint32_t v, std::int32_t shift);
}
