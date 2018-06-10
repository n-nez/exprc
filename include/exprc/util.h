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
