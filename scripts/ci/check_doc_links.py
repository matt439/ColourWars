"""Check that relative links in tracked Markdown reach a file and a heading."""

from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
from urllib.parse import unquote


# Inline links and images, then reference definitions. A target may sit in
# angle brackets and may carry a title after a space.
INLINE = re.compile(r"!?\[(?:[^\[\]]|\[[^\[\]]*\])*\]\(\s*(<[^>]*>|[^\s)]+)(?:\s+[\"'(][^)]*)?\)")
REFERENCE = re.compile(r"^ {0,3}\[[^\]]+\]:\s*(<[^>]*>|\S+)")
FENCE = re.compile(r"^ {0,3}(`{3,}|~{3,})")
HEADING = re.compile(r"^ {0,3}(#{1,6})\s+(.*?)\s*#*\s*$")
HTML_ANCHOR = re.compile(r"<a\s[^>]*(?:id|name)\s*=\s*[\"']([^\"']+)[\"']", re.IGNORECASE)
SCHEME = re.compile(r"^[a-zA-Z][a-zA-Z0-9+.-]*:")
# Evidence under out/ is generated locally and never tracked.
ALLOWED_ROOTS = ("out/",)
# A document with a line opening "Historical" or "Frozen" near its top is a
# record kept as written: it may name files since retired, but its anchors
# into documents that still exist are checked.
HISTORICAL = re.compile(r"^\W*(?:Historical|Frozen)\b")
HISTORICAL_LINES = 12


def slug(heading):
    """GitHub's heading anchor: the visible text, lowercased, punctuation dropped."""
    text = re.sub(r"!?\[([^\]]*)\]\([^)]*\)", r"\1", heading)
    text = re.sub(r"<[^>]+>", "", text)
    text = text.replace("`", "").strip().lower()
    text = "".join(ch for ch in text if ch.isalnum() or ch in " -_")
    return text.replace(" ", "-")


def strip_code(line):
    return re.sub(r"(`+)(?:(?!\1).)+\1", "", line)


def scan(text):
    """Return (anchors, links) for one document; links are (line, target)."""
    anchors, links, counts = set(), [], {}
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        match = FENCE.match(line)
        if fence:
            if match and match.group(1)[0] == fence[0] and len(match.group(1)) >= len(fence):
                fence = None
            continue
        if match:
            fence = match.group(1)
            continue
        anchors.update(HTML_ANCHOR.findall(line))
        heading = HEADING.match(line)
        if heading:
            base = slug(heading.group(2))
            seen = counts.get(base, 0)
            counts[base] = seen + 1
            anchors.add(base if seen == 0 else f"{base}-{seen}")
        code_free = strip_code(line)
        for target in INLINE.findall(code_free):
            links.append((number, target.strip("<>")))
        reference = REFERENCE.match(code_free)
        if reference:
            links.append((number, reference.group(1).strip("<>")))
    return anchors, links


def historical(text):
    return any(HISTORICAL.match(line) for line in text.splitlines()[:HISTORICAL_LINES])


class Checker:
    def __init__(self, root):
        self.root = Path(root)
        self.cache = {}

    def document(self, path):
        if path not in self.cache:
            self.cache[path] = scan(path.read_text(encoding="utf-8", errors="replace"))
        return self.cache[path]

    def relative(self, path):
        try:
            return PurePosixPath(Path(path).resolve().relative_to(self.root.resolve())).as_posix()
        except ValueError:
            return None

    def resolve(self, source, target, retired_files=False):
        """Return None when the target is reachable, else the reason."""
        if SCHEME.match(target) or target.startswith("//"):
            return None
        if target.startswith("#"):
            path, anchor = source, unquote(target[1:])
        else:
            raw = unquote(target)
            base = self.root if raw.startswith("/") else source.parent
            # A '#' may be part of a file name; only then is it not an anchor.
            if "#" in raw and (base / raw.lstrip("/")).exists():
                return None
            name, _, anchor = raw.partition("#")
            path = (base / name.lstrip("/")) if name else source
            relative = self.relative(path)
            if relative is not None and relative.startswith(ALLOWED_ROOTS):
                return None
            if not path.exists():
                return None if retired_files else f"no file {relative or path}"
        if not anchor or path.suffix.lower() != ".md" or not path.is_file():
            return None
        anchors, _ = self.document(path)
        if anchor in anchors or anchor.lower() in anchors:
            return None
        return f"no heading #{anchor} in {self.relative(path) or path}"

    def check(self, paths):
        problems = []
        for name in paths:
            source = self.root / name
            record = historical(source.read_text(encoding="utf-8", errors="replace"))
            _, links = self.document(source)
            for line, target in links:
                reason = self.resolve(source, target, retired_files=record)
                if reason:
                    problems.append(f"{name}:{line}: {target}: {reason}")
        return problems


def tracked_markdown(root):
    listed = subprocess.check_output(["git", "ls-files", "-z", "--", "*.md"], cwd=root)
    names = [name.decode("utf-8") for name in listed.split(b"\0") if name]
    return [name for name in names if not name.startswith("external/")]


def main():
    root = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    problems = Checker(root).check(tracked_markdown(root))
    for problem in problems:
        print(problem)
    if problems:
        print(f"{len(problems)} unresolved local Markdown link(s).")
        return 1
    print("Local Markdown links resolve.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
