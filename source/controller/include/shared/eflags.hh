#ifndef __EFLAGS_H__
#define __EFLAGS_H__

#include <type_traits>

template <typename EnumType>
class EFlags {
  public:
    static_assert(std::is_enum<EnumType>::value, "EnumType must be an enum class");

    EFlags()
        : value_(static_cast<std::underlying_type_t<EnumType>>(0)) {}

    explicit EFlags(EnumType flag)
        : value_(static_cast<std::underlying_type_t<EnumType>>(flag)) {}

    EFlags operator|(EFlags other) const {
        return EFlags(static_cast<EnumType>(value_ | other.value_));
    }

    EFlags operator&(EFlags other) const {
        return EFlags(static_cast<EnumType>(value_ & other.value_));
    }

    EFlags &operator|=(EFlags other) {
        value_ |= other.value_;
        return *this;
    }

    template <typename... Flags>
    bool contains(Flags... flags) const {
        return (has(EFlags(flags)) || ...);
    }

    explicit operator std::underlying_type_t<EnumType>() const { return value_; }

  private:
    bool has(EFlags flag) const { return (value_ & flag.value_) != 0; }

    std::underlying_type_t<EnumType> value_;
};

#endif  // __EFLAGS_H__