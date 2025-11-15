#pragma once

#if __has_include(<mdspan>)
    #include <mdspan>
    namespace stdx = std;
#elif __has_include(<experimental/mdspan>)
    #include <experimental/mdspan>
    namespace stdx = std::experimental;
#else
    #error "mdspan not found — upgrade compiler."
#endif

template <typename T>
using Matrix = stdx::mdspan<T, stdx::dextents<size_t, 2>>;

template <typename T>
using Vector = stdx::mdspan<T, stdx::dextents<size_t, 1>>;
