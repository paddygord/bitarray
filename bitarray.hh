#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <iostream>
#include <immintrin.h>
#include <cassert>

namespace {
template<typename T>
T pext(T data, T mask) {
    if constexpr (sizeof(T) <= 4) {
        return _pext_u32(data, mask);
    } else if (sizeof(T) <= 8) {
        return _pext_u64(data, mask);
    }
}

template<typename T>
T pdep(T data, T mask) {
    if constexpr (sizeof(T) <= 4) {
        return _pdep_u32(data, mask);
    } else if (sizeof(T) <= 8) {
        return _pdep_u64(data, mask);
    }
}
}

namespace bitarray {
template <size_t Bits, typename WordType = size_t>
class bitarray {
    static_assert(std::numeric_limits<WordType>::is_integer, "storage type must be an integer type");
    static_assert(std::numeric_limits<WordType>::digits <= 64, "storage type must be <= 64 bits wide");

    static constexpr size_t WordBits = std::numeric_limits<WordType>::digits;
public:
    std::array<WordType, 1 + (Bits - 1) / WordBits> data {};
    bitarray() : data {} {}
    bitarray(std::initializer_list<WordType> list) {
        std::uninitialized_copy(list.begin(), list.begin() + std::min(data.size(), list.size()), data.begin());
        sanitize();
    }
    template<typename T>
    bitarray(T other) {
        std::copy(other.data.begin(), other.data.begin() + std::min(other.data.size(), data.size()), data.begin());
        sanitize();
    }

private:
    static constexpr WordType zero();
    static constexpr WordType one();
    static constexpr WordType ones();
    void sanitize();
public:
    static constexpr size_t size();
    bool all() const;
    bool any() const;
    bool none() const;
    size_t count() const;
    size_t count_trailing_zeros() const;
    size_t count_leading_zeros() const;
    void set();
    constexpr void set(size_t pos, bool value = true);
    void reset();
    void reset(size_t pos);
    void flip();
    void flip(size_t pos);
    void insert_at_pos(WordType x, size_t pos);
    WordType extract_at_pos(size_t pos) const;
    bool operator==(const bitarray<Bits, WordType>& rhs) const;
    bool operator!=(const bitarray<Bits, WordType>& rhs) const;
    constexpr bool operator[](size_t pos) const;
    void operator~() const;
    void operator&=(const bitarray<Bits, WordType>& rhs);
    void operator|=(const bitarray<Bits, WordType>& rhs);
    void operator^=(const bitarray<Bits, WordType>& rhs);
    bitarray<Bits, WordType> operator&(const bitarray<Bits, WordType>& rhs) const;
    bitarray<Bits, WordType> operator|(const bitarray<Bits, WordType>& rhs) const;
    bitarray<Bits, WordType> operator^(const bitarray<Bits, WordType>& rhs) const;
    bitarray<Bits, WordType> operator<<(size_t _shift) const;
    bitarray<Bits, WordType> operator<<=(size_t _shift);
    bitarray<Bits, WordType> operator>>(size_t _shift) const;
    bitarray<Bits, WordType> operator>>=(size_t _shift);
    template<size_t M, size_t O = M>
    bitarray<O, WordType> gather(bitarray<M, WordType> mask);
    template<size_t M>
    bitarray<M, WordType> scatter(bitarray<M, WordType> mask);
    template <size_t Step, size_t Start>
    static constexpr bitarray<Bits, WordType> mask();
    template<size_t Len, size_t Num>
    static bitarray<Len * Num> interleave(std::array<bitarray<Len>, Num> input);
    template<size_t Len, size_t Num>
    static std::array<bitarray<Len>, Num> deinterleave(bitarray<Len * Num> input);
    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const bitarray<Bits, WordType>& x) {
        for (ssize_t i = x.size(); i--;) {
            bool bit = x[i];
            os << (bit ? '1' : '0');
        }
        return os;
    }
};

template <size_t Bits, typename WordType>
constexpr WordType bitarray<Bits, WordType>::zero() {
    return static_cast<WordType>(0);
}
template <size_t Bits, typename WordType>
constexpr WordType bitarray<Bits, WordType>::one() {
    return static_cast<WordType>(1);
}
template <size_t Bits, typename WordType>
constexpr WordType bitarray<Bits, WordType>::ones() {
    return ~static_cast<WordType>(0);
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::sanitize() {
    if (size() % WordBits != 0) {
        data.back() &= ~(ones() << (size() % WordBits));
    }
}
template <size_t Bits, typename WordType>
constexpr size_t bitarray<Bits, WordType>::size() {
    return Bits;
}
template <size_t Bits, typename WordType>
bool bitarray<Bits, WordType>::all() const {
    if ((data.back() | (ones() << (size() % WordBits))) != ones())
        return false;
    for (size_t i = 0; i + 1 < data.size(); i++)
        if (data[i] != ones())
            return false;
    return true;
}
template <size_t Bits, typename WordType>
bool bitarray<Bits, WordType>::any() const {
    for (auto& x: data)
        if (x != 0)
            return true;
    return false;
}
template <size_t Bits, typename WordType>
bool bitarray<Bits, WordType>::none() const {
    for (auto& x: data)
        if (x != 0)
            return false;
    return true;
}
template <size_t Bits, typename WordType>
size_t bitarray<Bits, WordType>::count() const {
    size_t count = 0;
    for (auto& x: data)
        count += __builtin_popcountll(x);
    return count;
}
template <size_t Bits, typename WordType>
size_t bitarray<Bits, WordType>::count_trailing_zeros() const {
    for (size_t i = 0; i < data.size(); i++)
        if (data[i] != 0)
            return i * WordBits + __builtin_ctzll(data[i]);
    return size();
}
template <size_t Bits, typename WordType>
size_t bitarray<Bits, WordType>::count_leading_zeros() const {
    for (size_t i = data.size(); i--;)
        if (data[i] != 0)
            return i * WordBits + __builtin_clzll(data[i]) - (WordBits - size() % WordBits);
    return size();
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::set() {
    for (auto& x: data)
        x = ones();
    sanitize();
}
template <size_t Bits, typename WordType>
constexpr void bitarray<Bits, WordType>::set(size_t pos, bool value) {
    assert(pos < size());
    if (value) {
        data[pos / WordBits] |= one() << (pos % WordBits);
    } else {
        data[pos / WordBits] &= ~(one() << (pos % WordBits));
    }
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::reset() {
    for (auto& x: data)
        x = zero();
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::reset(size_t pos) {
    assert(pos < size());
    data[pos / WordBits] &= ~(one() << (pos % WordBits));
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::flip() {
    for (auto& x: data)
        x ^= ones();
    sanitize();
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::flip(size_t pos) {
    assert(pos < size());
    data[pos / WordBits] ^= one() << (pos % WordBits);
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::insert_at_pos(WordType x, size_t pos) {
    assert(pos < size());
    size_t offset = pos % WordBits;
    data[pos / WordBits] |= x << offset;
    if (offset != 0 && pos / WordBits + 1 < data.size()) {
        data[pos / WordBits + 1] |= x >> (WordBits - offset);
    }
}
template <size_t Bits, typename WordType>
WordType bitarray<Bits, WordType>::extract_at_pos(size_t pos) const {
    assert(pos < size());
    size_t offset = pos % WordBits;
    WordType out = data[pos / WordBits] >> offset;
    if (offset != 0 && pos / WordBits + 1 < data.size()) {
        out |= data[pos / WordBits + 1] << (WordBits - offset);
    }
    return out;
}
template <size_t Bits, typename WordType>
bool bitarray<Bits, WordType>::operator==(const bitarray<Bits, WordType>& rhs) const {
    for (size_t i = 0; i < data.size(); i++)
        if (data[i] != rhs.data[i])
            return false;
    return true;
}
template <size_t Bits, typename WordType>
bool bitarray<Bits, WordType>::operator!=(const bitarray<Bits, WordType>& rhs) const {
    return !(*this == rhs);
}
template <size_t Bits, typename WordType>
constexpr bool bitarray<Bits, WordType>::operator[](size_t pos) const {
    assert(pos < size());
    return static_cast<bool>((data[pos / WordBits] >> (pos % WordBits)) & 1);
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::operator~() const {
    for (auto& x: data)
        x = ~x;
    sanitize();
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::operator&=(const bitarray<Bits, WordType>& rhs) {
    for (size_t i = 0; i < data.size(); i++)
        data[i] &= rhs.data[i];
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::operator|=(const bitarray<Bits, WordType>& rhs) {
    for (size_t i = 0; i < data.size(); i++)
        data[i] |= rhs.data[i];
}
template <size_t Bits, typename WordType>
void bitarray<Bits, WordType>::operator^=(const bitarray<Bits, WordType>& rhs) {
    for (size_t i = 0; i < data.size(); i++)
        data[i] ^= rhs.data[i];
    sanitize();
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator&(const bitarray<Bits, WordType>& rhs) const {
    bitarray<Bits, WordType> x = *this;
    x &= rhs;
    return x;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator|(const bitarray<Bits, WordType>& rhs) const {
    bitarray<Bits, WordType> x = *this;
    x |= rhs;
    return x;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator^(const bitarray<Bits, WordType>& rhs) const {
    bitarray<Bits, WordType> x = *this;
    x ^= rhs;
    return x;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator<<(size_t _shift) const {
    bitarray<Bits, WordType> x{};
    for (size_t pos = _shift, i = 0; pos < size() && i < data.size(); pos += WordBits, i++) {
        x.insert_at_pos(data[i], pos);
    }
    return x;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator<<=(size_t _shift) {
    bitarray<Bits, WordType> x{};
    for (size_t pos = _shift, i = 0; pos < size() && i < data.size(); pos += WordBits, i++) {
        x.insert_at_pos(data[i], pos);
    }
    *this = x;
    return *this;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator>>(size_t _shift) const {
    bitarray<Bits, WordType> x = *this;
    x >>= _shift;
    return x;
}
template <size_t Bits, typename WordType>
bitarray<Bits, WordType> bitarray<Bits, WordType>::operator>>=(size_t _shift) {
    size_t i = 0;
    for (size_t pos = _shift; pos < size() && i < data.size(); pos += WordBits, i++) {
        data[i] = extract_at_pos(pos);
    }
    for (; i < data.size(); i++) {
        data[i] = 0;
    }
    return *this;
}
template <size_t Bits, typename WordType>
template<size_t M, size_t O>
bitarray<O, WordType> bitarray<Bits, WordType>::gather(bitarray<M, WordType> mask) {
    static_assert(M <= size(), "gather operation mask length must be <= input length");
    bitarray<O, WordType> output {};
    for (size_t i = 0, pos = 0; i < mask.data.size(); i++) {
        output.insert_at_pos(pext(
            data[i],
            mask.data[i]
        ), pos);
        pos += __builtin_popcountll(mask.data[i]);
    }
    return output;
}
template <size_t Bits, typename WordType>
template <size_t Step, size_t Start>
constexpr bitarray<Bits, WordType> bitarray<Bits, WordType>::mask() {
    bitarray<Bits, WordType> x{};
    for (size_t i = Start; i < size(); i += Step) {
        x.set(i);
    }
    return x;
}
template<size_t Len, size_t... Is>
std::array<bitarray<Len>, sizeof...(Is)> all_masks_inner(std::index_sequence<Is...>) {
    std::array<bitarray<Len>, sizeof...(Is)> masks;
    size_t i = 0;
    (void(masks[i++] = (bitarray<Len>::template mask<sizeof...(Is), Is>())) , ...);
    return masks;
};
template<size_t Len, size_t Num>
std::array<bitarray<Len>, Num> all_masks() {
    using seq = std::make_index_sequence<Num>;
    return all_masks_inner<Len>(seq());
};
template <size_t Bits, typename WordType>
template<size_t M>
bitarray<M, WordType> bitarray<Bits, WordType>::scatter(bitarray<M, WordType> mask) {
    static_assert(M >= size(), "scatter operation mask length must be >= input length");
    bitarray<M, WordType> output {};
    for (size_t i = 0, pos = 0; i < output.data.size(); i++) {
        output.data[i] = pdep(
            extract_at_pos(pos),
            mask.data[i]
        );
        pos += __builtin_popcountll(mask.data[i]);
    }
    return output;
}
template <size_t Bits, typename WordType>
template<size_t Len, size_t Num>
bitarray<Len * Num> bitarray<Bits, WordType>::interleave(std::array<bitarray<Len>, Num> input) {
    bitarray<Len * Num> output {};
    std::array<bitarray<Len * Num>, Num> masks = all_masks<Len * Num, Num>();
    for (size_t j = 0; j < input.size(); j++) {
        output |= input[j].template scatter<Len * Num>(masks[j]);
    }
    return output;
}
template <size_t Bits, typename WordType>
template<size_t Len, size_t Num>
std::array<bitarray<Len>, Num> bitarray<Bits, WordType>::deinterleave(bitarray<Len * Num> input) {
    std::array<bitarray<Len>, Num> output {};
    std::array<bitarray<Len * Num>, Num> masks = all_masks<Len * Num, Num>();
    for (size_t j = 0; j < output.size(); j++) {
        output[j] = input.template gather<Len * Num, Len>(masks[j]);
    }
    return output;
}
}