#ifndef POAC_OPTS_VERSION_HPP
#define POAC_OPTS_VERSION_HPP

#include <iostream>
#include <optional>

#include <poac/util/termcolor2.hpp>

namespace poac::opts::version {
    constexpr auto summary = termcolor2::make_string("Show the current poac version");
    constexpr auto options = termcolor2::make_string("<Nothing>");

    [[nodiscard]] std::optional<core::except::Error>
    exec(std::optional<io::yaml::Config>&&, std::vector<std::string>&&) noexcept {
        std::cout << POAC_VERSION << std::endl;
        return std::nullopt;
    }
} // end namespace
#endif // !POAC_OPTS_VERSION_HPP
