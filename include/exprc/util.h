#ifndef EXPRC_UTIL_H
#define EXPRC_UTIL_H

#include <type_traits>
#include <tuple>

namespace exprc {

namespace util {

namespace details {

template <typename I>
using is_input_iterator = std::is_convertible<typename std::iterator_traits<I>::iterator_category*, typename std::input_iterator_tag*>;

} // namespace details

template <typename IdType>
constexpr auto asInt(IdType id) {
    return static_cast<std::underlying_type_t<IdType>>(id);
}

template <typename IdType>
class IdGen {
public:
    IdType operator()() {
        return static_cast<IdType>(m_next++);
    }

private:
    std::underlying_type_t<IdType> m_next{asInt(IdType::FIRST_VALID_ID)};
};

template <typename... Types>
class Context {
public:

    template <typename Type, typename... Args>
    auto make(Args&&... args) {
        using GenType = IdGen<typename Type::Id>;
        return Type{std::get<GenType>(m_next_id)(), std::forward<Args>(args)...};
    }

private:
    std::tuple<IdGen<typename Types::Id>...> m_next_id;
};

} // namespace util

} // namespace exprc

namespace std {

template <typename I>
constexpr std::enable_if_t<exprc::util::details::is_input_iterator<I>{}, I> begin(std::pair<I, I> pair) {
    return pair.first;
}

template <typename I>
constexpr std::enable_if_t<exprc::util::details::is_input_iterator<I>{}, I> end(std::pair<I, I> pair) {
    return pair.second;
}

} // namespace std

#endif // EXPRC_UTIL_H
