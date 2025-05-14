#pragma once

#include <variant>

namespace MR {
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;
template<class var_t, class... Func> auto match(var_t & variant, Func &&... funcs)
{
    return std::visit(overload{ std::forward<Func>(funcs)... }, variant);
}
}
