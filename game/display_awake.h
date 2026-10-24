#pragma once

// One GameLevel owns one successful display-idle request for its lifetime.
// The injected function is non-null and nonthrowing; production supplies the
// Win32 adapter, while policy probes supply only a recording function.
class DisplayAwake final
{
public:
    using Request = bool(bool keep) noexcept;

    DisplayAwake() noexcept;
    explicit DisplayAwake(Request& request) noexcept :
        request_(request), acquired_(request_(true))
    {
    }
    ~DisplayAwake() noexcept
    {
        if (this->acquired_) { (void)this->request_(false); }
    }

    DisplayAwake(const DisplayAwake&) = delete;
    DisplayAwake& operator=(const DisplayAwake&) = delete;
    DisplayAwake(DisplayAwake&&) = delete;
    DisplayAwake& operator=(DisplayAwake&&) = delete;

private:
    Request& request_;
    const bool acquired_;
};