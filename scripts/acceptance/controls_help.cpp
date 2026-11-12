#include "game/pch.h"
#include "game/input/action_bindings.h"
#include "game/input/input_bindings.h"
#include "engine/input/keyboard.h"
#include "scripts/acceptance/support/probe.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// HYG#102: the README's Controls table and the packaged HOW-TO-PLAY.txt table
// restate the default bindings. This case reads the defaults the game starts
// from (default_action_bindings and the default PadProfile), names each
// control through the game's own tokens (pad_control_name, key_name,
// desktop_control_name) and requires one of that token's help spellings on
// the action's row, in the right column, of both tables. It matches words in
// cells, not the tables' layout, so prose can be reworded freely; a changed
// default, or a table naming a different control, fails.
namespace
{
    using acceptance::require;

    enum class Column { pad, keyboard };

    struct Expectation
    {
        std::string action;  // the row label's first word
        Column column = Column::pad;
        std::string token;   // the game's name for the control
    };

    struct Row
    {
        std::string label, pad, keyboard;
    };

    struct Table
    {
        std::string name;
        std::vector<Row> rows;
    };

    std::string lower(std::string_view text)
    {
        std::string result(text);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
        return result;
    }

    std::string trim(std::string_view text)
    {
        const auto first = text.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) { return {}; }
        const auto last = text.find_last_not_of(" \t\r");
        return std::string(text.substr(first, last - first + 1));
    }

    bool word_char(char value) { return std::isalnum(static_cast<unsigned char>(value)) != 0; }

    // A whole-word occurrence: case-sensitive for a single letter (so "A" is
    // the key, not the article), case-insensitive otherwise.
    bool has_word(std::string_view cell, std::string_view phrase)
    {
        const bool exact = phrase.size() == 1;
        const std::string haystack = exact ? std::string(cell) : lower(cell);
        const std::string needle = exact ? std::string(phrase) : lower(phrase);
        for (std::size_t at = haystack.find(needle); at != std::string::npos; at = haystack.find(needle, at + 1))
        {
            const bool open = at == 0 || !word_char(haystack[at - 1]);
            const std::size_t end = at + needle.size();
            const bool close = end == haystack.size() || !word_char(haystack[end]);
            if (open && close) { return true; }
        }
        return false;
    }

    // How the help may spell each game token. A default with no entry here
    // fails, so a new default control cannot pass unwritten.
    std::vector<std::string> spellings(Column column, const std::string& token)
    {
        if (column == Column::pad)
        {
            static const std::map<std::string, std::vector<std::string>> pad{
                { "a", { "A" } }, { "b", { "B" } }, { "x", { "X" } }, { "y", { "Y" } },
                { "lb", { "left bumper", "LB" } }, { "rb", { "right bumper", "RB" } },
                { "lt", { "left trigger", "LT" } }, { "rt", { "right trigger", "RT" } },
                { "ls", { "left stick button", "LS" } }, { "rs", { "right stick button", "RS" } },
                { "start", { "Start" } },
                // Movement and the held drop are stick readings, not bindings.
                { "left_stick", { "left stick" } }
            };
            const auto found = pad.find(token);
            require(found != pad.end(), "no help spelling for pad control " + token);
            return found->second;
        }
        if (token.size() == 1 && token[0] >= 'a' && token[0] <= 'z')
        {
            return { std::string(1, static_cast<char>(token[0] - 'a' + 'A')) };
        }
        static const std::map<std::string, std::vector<std::string>> desktop{
            { "space", { "Space" } }, { "escape", { "Escape", "Esc" } }, { "down", { "Down" } },
            { "mouse_left", { "left mouse button", "left click" } },
            { "mouse_right", { "right mouse button", "right click" } },
            { "mouse_middle", { "middle mouse button", "middle click" } },
            { "mouse_x1", { "mouse 4" } }, { "mouse_x2", { "mouse 5" } }
        };
        const auto found = desktop.find(token);
        require(found != desktop.end(), "no help spelling for keyboard control " + token);
        return found->second;
    }

    void add_desktop(std::vector<Expectation>& out, const std::string& action, GameAction game)
    {
        const DesktopActionBinding& binding = default_action_bindings().game_desktop[static_cast<std::size_t>(game)];
        require(!binding.keys.empty() || !binding.mouse_buttons.empty(),
            "default " + action + " has no keyboard or mouse control");
        for (const labrador::Key key : binding.keys)
        {
            const std::string_view name = labrador::key_name(key);
            require(!name.empty(), "default " + action + " has an unnamed key");
            out.push_back({ action, Column::keyboard, std::string(name) });
        }
        for (const labrador::MouseButton button : binding.mouse_buttons)
        {
            out.push_back({ action, Column::keyboard,
                std::string(desktop_control_name(DesktopControl{ labrador::Key::none, button })) });
        }
    }

    void add_pad(std::vector<Expectation>& out, const std::string& action, PadAction pad_action)
    {
        const PadActionChoice& choice = PadProfile()[pad_action];
        out.push_back({ action, Column::pad, std::string(pad_control_name(choice.primary)) });
        if (choice.alternate != PadControl::none)
        {
            out.push_back({ action, Column::pad, std::string(pad_control_name(choice.alternate)) });
        }
    }

    void add_stick(std::vector<Expectation>& out, const std::string& action, GameAction game)
    {
        const PadActionBinding& binding = default_action_bindings().game_pad[static_cast<std::size_t>(game)];
        require(binding.buttons.empty() && binding.triggers.empty(),
            "default pad " + action + " is now a binding; name it here instead of the stick");
        out.push_back({ action, Column::pad, "left_stick" });
    }

    std::vector<Expectation> expectations()
    {
        std::vector<Expectation> out;
        add_stick(out, "Move", GameAction::move_left);
        add_stick(out, "Move", GameAction::move_right);
        add_desktop(out, "Move", GameAction::move_left);
        add_desktop(out, "Move", GameAction::move_right);
        add_pad(out, "Jump", PadAction::jump);
        add_desktop(out, "Jump", GameAction::jump);
        add_pad(out, "Shoot", PadAction::shoot);
        add_desktop(out, "Shoot", GameAction::shoot);
        add_pad(out, "Sub", PadAction::sub);
        add_desktop(out, "Sub", GameAction::sub);
        add_stick(out, "Drop", GameAction::drop);
        add_desktop(out, "Drop", GameAction::drop);
        add_pad(out, "Pause", PadAction::pause);
        add_desktop(out, "Pause", GameAction::pause);
        // Each pad action is covered, so a new one must be given a row here.
        require(pad_action_count == 4, "a new pad action needs a help row in this case");
        require(keyboard_action_count == 7, "a new keyboard action needs a help row in this case");
        return out;
    }

    std::vector<std::string> lines_of(const std::string& text)
    {
        std::vector<std::string> lines;
        std::istringstream stream(text);
        for (std::string line; std::getline(stream, line);)
        {
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            lines.push_back(line);
        }
        return lines;
    }

    std::string read_file(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        require(file.good(), "cannot read " + path.string());
        std::ostringstream text;
        text << file.rdbuf();
        return text.str();
    }

    // Column indices from a header's cells: "Gamepad" and "Keyboard ...".
    Row row_from(const std::vector<std::string>& header, const std::vector<std::string>& cells,
        const std::string& table)
    {
        require(cells.size() == header.size(), table + " row has " + std::to_string(cells.size()) +
            " cells, header has " + std::to_string(header.size()));
        Row row;
        row.label = cells[0];
        for (std::size_t index = 1; index < header.size(); ++index)
        {
            const std::string name = lower(header[index]);
            if (name.starts_with("gamepad")) { row.pad = cells[index]; }
            else if (name.starts_with("keyboard")) { row.keyboard = cells[index]; }
        }
        return row;
    }

    // The README's pipe table under "### Controls".
    Table readme_table(const std::string& text)
    {
        Table table{ "README.md Controls", {} };
        const std::vector<std::string> lines = lines_of(text);
        auto at = std::find(lines.begin(), lines.end(), "### Controls");
        require(at != lines.end(), "README.md has no ### Controls heading");
        while (at != lines.end() && !at->starts_with("|")) { ++at; }
        std::vector<std::string> header;
        for (; at != lines.end() && at->starts_with("|"); ++at)
        {
            std::vector<std::string> cells;
            std::string_view rest(*at);
            rest.remove_prefix(1);
            for (std::size_t bar = rest.find('|'); bar != std::string_view::npos; bar = rest.find('|'))
            {
                cells.push_back(trim(rest.substr(0, bar)));
                rest.remove_prefix(bar + 1);
            }
            if (header.empty()) { header = cells; continue; }
            if (!cells.empty() && cells[0].starts_with("---")) { continue; }
            table.rows.push_back(row_from(header, cells, table.name));
        }
        require(!table.rows.empty(), "README.md Controls has no table rows");
        return table;
    }

    // Cells separated by two or more spaces.
    std::vector<std::string> spaced_cells(const std::string& line)
    {
        std::vector<std::string> cells;
        std::string current;
        std::size_t spaces = 0;
        for (const char value : trim(line))
        {
            if (value == ' ') { ++spaces; continue; }
            if (spaces >= 2 && !current.empty()) { cells.push_back(current); current.clear(); }
            else if (spaces == 1) { current += ' '; }
            spaces = 0;
            current += value;
        }
        if (!current.empty()) { cells.push_back(current); }
        return cells;
    }

    // HOW-TO-PLAY's table: the line naming "Gamepad" and "Keyboard and
    // mouse", then each line until a blank one. The header has no label cell.
    Table guide_table(const std::string& text)
    {
        Table table{ "HOW-TO-PLAY.txt controls", {} };
        const std::vector<std::string> lines = lines_of(text);
        const auto at = std::find_if(lines.begin(), lines.end(), [](const std::string& line)
            { return line.find("Gamepad") != std::string::npos && line.find("Keyboard and mouse") != std::string::npos; });
        require(at != lines.end(), "HOW-TO-PLAY.txt has no Gamepad / Keyboard and mouse table");
        std::vector<std::string> header = spaced_cells(*at);
        header.insert(header.begin(), std::string());
        for (auto line = at + 1; line != lines.end() && !trim(*line).empty(); ++line)
        {
            table.rows.push_back(row_from(header, spaced_cells(*line), table.name));
        }
        require(!table.rows.empty(), "HOW-TO-PLAY.txt controls has no table rows");
        return table;
    }

    // The one row whose label starts with the action's word.
    std::size_t row_index(const Table& table, const std::string& action)
    {
        std::size_t found = table.rows.size();
        for (std::size_t index = 0; index < table.rows.size(); ++index)
        {
            const std::string label = lower(table.rows[index].label);
            const std::string want = lower(action);
            if (label.starts_with(want) && (label.size() == want.size() || !word_char(label[want.size()])))
            {
                require(found == table.rows.size(), table.name + " has two " + action + " rows");
                found = index;
            }
        }
        require(found != table.rows.size(), table.name + " has no " + action + " row");
        return found;
    }

    // Every failure, so one run names every stale cell.
    std::vector<std::string> check(const Table& table, const std::vector<Expectation>& expected)
    {
        std::vector<std::string> failures;
        for (const Expectation& item : expected)
        {
            const Row& row = table.rows[row_index(table, item.action)];
            const std::string& cell = item.column == Column::pad ? row.pad : row.keyboard;
            const std::vector<std::string> names = spellings(item.column, item.token);
            const bool named = std::any_of(names.begin(), names.end(),
                [&cell](const std::string& name) { return has_word(cell, name); });
            if (!named)
            {
                failures.push_back(table.name + ": " + item.action + " row's " +
                    (item.column == Column::pad ? "gamepad" : "keyboard") + " cell \"" + cell +
                    "\" does not name the default " + item.token + " (as \"" + names.front() + "\")");
            }
        }
        return failures;
    }

    // Replaces every whole-word spelling of a token in a cell.
    std::string without(std::string cell, const std::vector<std::string>& names, const std::string& stand_in)
    {
        for (const std::string& name : names)
        {
            while (has_word(cell, name))
            {
                const bool exact = name.size() == 1;
                const std::string haystack = exact ? cell : lower(cell);
                const std::string needle = exact ? name : lower(name);
                for (std::size_t at = haystack.find(needle); at != std::string::npos; at = haystack.find(needle, at + 1))
                {
                    const std::size_t end = at + needle.size();
                    if ((at == 0 || !word_char(haystack[at - 1])) && (end == haystack.size() || !word_char(haystack[end])))
                    {
                        cell.replace(at, needle.size(), stand_in);
                        break;
                    }
                }
            }
        }
        return cell;
    }

    // The check must catch a table naming a different control: for each
    // expectation, swap that control's spellings in its cell for another
    // control and require exactly that failure.
    void require_detects_each_swap(const Table& table, const std::vector<Expectation>& expected)
    {
        for (const Expectation& item : expected)
        {
            Table changed = table;
            Row& row = changed.rows[row_index(changed, item.action)];
            std::string& cell = item.column == Column::pad ? row.pad : row.keyboard;
            const std::string stand_in = item.column == Column::pad
                ? (item.token == "y" ? "X" : "Y") : (item.token == "q" ? "Z" : "Q");
            cell = without(cell, spellings(item.column, item.token), stand_in);
            const std::vector<std::string> failures = check(changed, expected);
            const bool caught = std::any_of(failures.begin(), failures.end(), [&item](const std::string& failure)
                { return failure.find("the default " + item.token + " ") != std::string::npos; });
            require(caught, table.name + ": swapping " + item.action + "'s " + item.token + " went unnoticed");
        }
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("controls help", [&]
    {
        require(argc == 2, "usage: controls_help <repository>");
        const std::filesystem::path repo(argv[1]);
        const std::vector<Expectation> expected = expectations();
        const std::string guide = read_file(repo / "docs" / "HOW-TO-PLAY.txt");
        const std::vector<Table> tables{ readme_table(read_file(repo / "README.md")), guide_table(guide) };

        std::vector<std::string> failures;
        for (const Table& table : tables)
        {
            const std::vector<std::string> found = check(table, expected);
            failures.insert(failures.end(), found.begin(), found.end());
        }
        for (const std::string& failure : failures) { std::cerr << failure << '\n'; }
        require(failures.empty(), std::to_string(failures.size()) + " controls help cell(s) disagree with the defaults");

        for (const Table& table : tables) { require_detects_each_swap(table, expected); }

        // The same through the file text: the guide's Jump row naming Enter
        // instead of Space fails.
        std::string stale = guide;
        const std::size_t jump = stale.find("  Jump ");
        require(jump != std::string::npos, "HOW-TO-PLAY.txt has no Jump line");
        const std::size_t space = stale.find("Space", jump);
        require(space != std::string::npos && space < stale.find('\n', jump), "HOW-TO-PLAY.txt Jump line names no Space");
        stale.replace(space, 5, "Enter");
        require(!check(guide_table(stale), expected).empty(), "a guide naming Enter for Jump passed");

        std::cout << "controls help: " << expected.size() << " default controls named on both tables' rows\n";
        return 0;
    });
}
