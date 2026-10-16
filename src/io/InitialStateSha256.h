#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

class InitialStateSha256 {
public:
  void update(const unsigned char* data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
      m_block[m_blockSize++] = data[i];
      if (m_blockSize == 64) {
        transform(m_block.data());
        m_bitLength += 512;
        m_blockSize = 0;
      }
    }
  }

  std::string finish() {
    m_bitLength += static_cast<std::uint64_t>(m_blockSize) * 8;
    m_block[m_blockSize++] = 0x80;
    if (m_blockSize > 56) {
      while (m_blockSize < 64) m_block[m_blockSize++] = 0;
      transform(m_block.data());
      m_blockSize = 0;
    }
    while (m_blockSize < 56) m_block[m_blockSize++] = 0;
    for (int i = 7; i >= 0; --i)
      m_block[m_blockSize++] = static_cast<unsigned char>(m_bitLength >> (i * 8));
    transform(m_block.data());

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::hex << std::setfill('0');
    for (std::uint32_t word : m_state)
      output << std::setw(8) << word;
    return output.str();
  }

private:
  static std::uint32_t rotateRight(std::uint32_t value, unsigned amount) {
    return (value >> amount) | (value << (32 - amount));
  }

  void transform(const unsigned char* block) {
    static constexpr std::array<std::uint32_t, 64> constants = {{
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    }};
    std::array<std::uint32_t, 64> words{};
    for (int i = 0; i < 16; ++i)
      words[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
                 (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                 (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                 static_cast<std::uint32_t>(block[i * 4 + 3]);
    for (int i = 16; i < 64; ++i) {
      const std::uint32_t s0 = rotateRight(words[i - 15], 7) ^ rotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
      const std::uint32_t s1 = rotateRight(words[i - 2], 17) ^ rotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
      words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }
    std::uint32_t a=m_state[0], b=m_state[1], c=m_state[2], d=m_state[3];
    std::uint32_t e=m_state[4], f=m_state[5], g=m_state[6], h=m_state[7];
    for (int i = 0; i < 64; ++i) {
      const std::uint32_t s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
      const std::uint32_t choice = (e & f) ^ (~e & g);
      const std::uint32_t temp1 = h + s1 + choice + constants[i] + words[i];
      const std::uint32_t s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
      const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = s0 + majority;
      h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
    }
    m_state[0]+=a; m_state[1]+=b; m_state[2]+=c; m_state[3]+=d;
    m_state[4]+=e; m_state[5]+=f; m_state[6]+=g; m_state[7]+=h;
  }

  std::array<std::uint32_t, 8> m_state = {{
    0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
    0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
  }};
  std::array<unsigned char, 64> m_block{};
  std::size_t m_blockSize = 0;
  std::uint64_t m_bitLength = 0;
};

inline std::string InitialStateSha256Hex(const std::string& bytes) {
  InitialStateSha256 hash;
  hash.update(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
  return hash.finish();
}

inline std::string InitialStateSha256File(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) throw std::runtime_error("Cannot open BDF to calculate SHA-256: " + path);
  InitialStateSha256 hash;
  std::array<char, 8192> buffer{};
  while (input.good()) {
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize count = input.gcount();
    if (count > 0)
      hash.update(reinterpret_cast<const unsigned char*>(buffer.data()), static_cast<std::size_t>(count));
  }
  if (!input.eof()) throw std::runtime_error("Failed while reading BDF for SHA-256: " + path);
  return hash.finish();
}
