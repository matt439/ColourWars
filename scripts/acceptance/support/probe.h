#pragma once

// What every acceptance probe shares: a failed check throws, and the probe's
// main hands its body to run_probe, which turns an escaping exception into a
// "FAIL <name>: <reason>" line on stderr and exit code 1. The runner decides
// pass or fail by that exit code alone (scripts/run-acceptance.ps1).

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace acceptance
{
    inline void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    inline void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    // Runs the probe's body, which returns its exit code. An exception that
    // escapes it is reported under the probe's name and exits 1.
    template <typename Body>
    int run_probe(const char* name, Body&& body)
    {
        static_assert(std::is_same_v<std::invoke_result_t<Body&>, int>,
            "A probe body returns its exit code");
        try
        {
            return body();
        }
        catch (const std::exception& error)
        {
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
            return 1;
        }
        catch (...)
        {
            std::cerr << "FAIL " << name << ": an unknown exception escaped the probe\n";
            return 1;
        }
    }
}
