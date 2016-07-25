#include <cstdint>

#include "bits.hpp"

namespace tcp_messenger {
  
  std::uint32_t rotl(std::uint32_t v, std::int32_t shift) {
    std::int32_t s =  shift>=0? shift%32 : -((-shift)%32);
    return (v<<s) | (v>>(32-s));
  }

  std::uint32_t rotr(std::uint32_t v, std::int32_t shift) {
    std::int32_t s =  shift>=0? shift%32 : -((-shift)%32);
    return (v>>s) | (v<<(32-s));
  }

}
