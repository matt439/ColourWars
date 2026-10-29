#include "game/pch.h"
#include "game/display_awake.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace
{
    using NoThrowRequest = bool(bool) noexcept;
    using ThrowingRequest = bool(bool);
    static_assert(std::is_same_v<DisplayAwake::Request, NoThrowRequest>);
    static_assert(std::is_nothrow_default_constructible_v<DisplayAwake>);
    static_assert(std::is_nothrow_constructible_v<DisplayAwake, NoThrowRequest&>);
    static_assert(std::is_nothrow_destructible_v<DisplayAwake>);
    static_assert(!std::is_copy_constructible_v<DisplayAwake> && !std::is_copy_assignable_v<DisplayAwake>);
    static_assert(!std::is_move_constructible_v<DisplayAwake> && !std::is_move_assignable_v<DisplayAwake>);
    static_assert(!std::is_constructible_v<DisplayAwake, NoThrowRequest*>);
    static_assert(!std::is_constructible_v<DisplayAwake, std::nullptr_t>);
    static_assert(!std::is_constructible_v<DisplayAwake, ThrowingRequest&>);

    using acceptance::require;

    struct Requests
    {
        std::array<bool, 8> operations{};
        size_t count = 0;
        bool acquire_succeeds = true;
        bool clear_succeeds = true;
        bool overflow = false;
    };
    // Test-only callback context; the production guard has no global hook.
    Requests* recording = nullptr;
    bool record_request(bool keep) noexcept
    {
        if (recording == nullptr) { return false; }
        if (recording->count < recording->operations.size())
        {
            recording->operations[recording->count] = keep;
        }
        else { recording->overflow = true; }
        ++recording->count;
        return keep ? recording->acquire_succeeds : recording->clear_succeeds;
    }
    struct RecordScope
    {
        explicit RecordScope(Requests& requests) : previous(recording) { recording = &requests; }
        ~RecordScope() { recording = previous; }
        Requests* previous;
    };
    template<size_t N>
    void exact(const Requests& requests, const std::array<bool, N>& expected)
    {
        require(!requests.overflow && requests.count == N, "Display request count or recorder bound differs");
        for (size_t i = 0; i < N; ++i)
        {
            require(requests.operations[i] == expected[i], "Display request acquire/clear order differs");
        }
    }

    void success()
    {
        Requests requests; const RecordScope scope(requests);
        exact(requests, std::array<bool, 0>{});
        {
            const DisplayAwake awake(record_request);
            exact(requests, std::array<bool, 1>{ true });
            requests.acquire_succeeds = false;
        }
        // Ownership follows the original successful result, not a later response.
        exact(requests, std::array<bool, 2>{ true, false });
    }
    void acquire_failure()
    {
        Requests requests; requests.acquire_succeeds = false; const RecordScope scope(requests);
        {
            const DisplayAwake awake(record_request);
            exact(requests, std::array<bool, 1>{ true });
            requests.acquire_succeeds = true;
        }
        exact(requests, std::array<bool, 1>{ true });
    }
    void clear_failure()
    {
        Requests requests; requests.clear_succeeds = false; const RecordScope scope(requests);
        { const DisplayAwake awake(record_request); }
        // One best-effort attempt, with no retry or assertion that it succeeded.
        exact(requests, std::array<bool, 2>{ true, false });
    }

    struct ExpectedOwnerFailure {};
    struct PrecedingMember
    {
        explicit PrecedingMember(bool fail)
        {
            if (fail) { throw ExpectedOwnerFailure{}; }
        }
    };
    struct SyntheticOwner
    {
        SyntheticOwner(bool fail_before_guard, bool fail_after_guard) :
            preceding(fail_before_guard), awake(record_request)
        {
            if (fail_after_guard) { throw ExpectedOwnerFailure{}; }
        }
        PrecedingMember preceding;
        DisplayAwake awake;
    };
    void owner_construction_unwind()
    {
        // This is containing-owner C++ member-unwind proof, not a GameLevel
        // construction/init test and not an unsafe missing-stage injection.
        Requests requests; const RecordScope scope(requests); bool caught = false;
        try { const SyntheticOwner owner(true, false); }
        catch (const ExpectedOwnerFailure&) { caught = true; }
        require(caught, "Expected preceding-member failure was not observed");
        exact(requests, std::array<bool, 0>{});
        caught = false;
        try { const SyntheticOwner owner(false, true); }
        catch (const ExpectedOwnerFailure&) { caught = true; }
        require(caught, "Expected post-acquisition owner failure was not observed");
        exact(requests, std::array<bool, 2>{ true, false });
    }
    void clear_failure_during_unwind()
    {
        Requests requests; requests.clear_succeeds = false; const RecordScope scope(requests);
        bool caught = false;
        try
        {
            const DisplayAwake awake(record_request);
            throw ExpectedOwnerFailure{};
        }
        catch (const ExpectedOwnerFailure&) { caught = true; }
        require(caught, "Display clear failure replaced the owner's original exception");
        exact(requests, std::array<bool, 2>{ true, false });
    }
    void sequential_owners()
    {
        Requests requests; const RecordScope scope(requests);
        { const DisplayAwake first(record_request); }
        requests.acquire_succeeds = false;
        { const DisplayAwake failed(record_request); }
        requests.acquire_succeeds = true;
        { const DisplayAwake second(record_request); }
        exact(requests, std::array<bool, 5>{ true, false, true, true, false });
    }
}

int main()
{
    return acceptance::run_probe("display-awake helper", [&]
    {
        success(); acquire_failure(); clear_failure(); owner_construction_unwind();
        clear_failure_during_unwind(); sequential_owners();
        require(recording == nullptr, "Test callback context escaped its owning scope");
        std::cout << "PASS display-awake helper: success, failed acquire/clear, before/after containing-owner construction failure, "
            "clear failure during unwind and sequential ownership; static noncopy/move/non-null/noexcept checks. "
            "Injected boolean intent only: no GameLevel construction, SetThreadExecutionState call, window, device or power-setting change.\n";
        return 0;
    });
}