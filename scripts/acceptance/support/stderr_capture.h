#pragma once

// Sends stderr's descriptor to a file while it lives. The stderr stream is
// never reopened, so a failed start leaves it as it was; finish() restores the
// saved descriptor and fails if it cannot, and the destructor restores it
// quietly when the probe ends early. The file is written in text mode.
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <share.h>
#include <stdexcept>
#include <sys/stat.h>

#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

namespace acceptance
{
    class StderrCapture
    {
    public:
        explicit StderrCapture(const std::filesystem::path& path)
        {
            std::fflush(stderr);
            int file = -1;
            require(_wsopen_s(&file, path.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_TEXT, _SH_DENYNO,
                _S_IREAD | _S_IWRITE) == 0 && file >= 0, "Cannot open the stderr capture file " + utf8_path(path));
            this->saved_ = _dup(_fileno(stderr));
            const bool redirected = this->saved_ >= 0 && _dup2(file, _fileno(stderr)) == 0;
            _close(file);
            if (!redirected)
            {
                if (this->saved_ >= 0) { _close(this->saved_); this->saved_ = -1; }
                throw std::runtime_error("Cannot capture stderr");
            }
        }
        StderrCapture(const StderrCapture&) = delete;
        StderrCapture& operator=(const StderrCapture&) = delete;
        ~StderrCapture() { this->restore(); }

        void finish() { require(this->restore(), "Cannot restore the probe's stderr after capturing it"); }

    private:
        bool restore() noexcept
        {
            if (this->saved_ < 0) { return true; }
            std::fflush(stderr);
            const bool restored = _dup2(this->saved_, _fileno(stderr)) == 0;
            _close(this->saved_); this->saved_ = -1; std::clearerr(stderr);
            return restored;
        }

        int saved_ = -1;
    };
}
