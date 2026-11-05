# Localisation (`GAP#L11`)

The locale axis: which language the shell's copy is in, where a translation
lives, what it may change, what happens to a string it leaves out, which
characters it may use and how numbers are written. Implemented 2026-11-04 with
one test locale. Which languages ship was the author's call (`GAP#L11`): on
2026-11-05 the author chose none for now, so the game ships English and the
`qps-ploc` test locale only, and the row is closed. Localisation returns as a
new row in [TODO.md](../TODO.md) when a language is chosen; the work that
waits for it is listed below.

## How a locale works

[`presentation.json`](../game/content/presentation.json) is the **base locale**
(English, tag `en`) and holds every string. Every other locale is a partial
copy of its text in `game/content/locales/<tag>.json`, listed in
[`locales/locales.json`](../game/content/locales/locales.json):

```json
{
  "base": { "tag": "en", "name": "English" },
  "locales": [
    { "tag": "qps-ploc", "name": "[Pseudo]", "fallback": "en", "test": true }
  ]
}
```

At startup, before any asset, [`main.cpp`](../game/main.cpp) reads the index
and every file it lists, chooses one locale and lays it, and each locale it
falls back to, over `presentation.json`'s text. `Presentation` then reads the
composed document exactly as it reads the base. Nothing downstream knows a
locale exists, and every check the shipped English passes - a template's
tokens, each character against the font that draws it, each composed line
against its box, the Mode rows built for every value - judges the translation
the same way. A diagnostic names the locale: `'presentation.json (locale fr)':
menu_text.title: start ...`.

The code is [game/config/locale.h](../game/config/locale.h): `Locales` (the
index, the files and composition) and `ActiveLocale` (the run's choice).

### The index

| Key | What |
|---|---|
| `base.tag`, `base.name` | the base locale, which is `presentation.json` itself |
| `locales[].tag` | a BCP 47 tag: a two- or three-letter language, then subtags of two to eight letters or digits (`fr`, `pt-BR`, `qps-ploc`). It is also the file name. Listed once |
| `locales[].name` | the language's name in itself, as a language picker would show it; drawable text |
| `locales[].fallback` | optional: the base (the default) or a locale **listed above** this one, so the chain always ends |
| `locales[].test` | optional: a test locale, selectable but never offered to players |

### A locale file

```json
{
  "_comment": "Keys beginning with _ are ignored anywhere in the file.",
  "fonts": { "lato_black_48": "noto_sans_48" },
  "presentation": {
    "menu_text": { "title": { "start": "JOUONS" } },
    "main_menu": { "weapon_select": { "choices": { "sprayer": { "name": "Pulverisateur" } } } },
    "team_palette": { "pairs": [ null, { "label": "A: Rose / B: Vert" }, null, null, null, null ] }
  }
}
```

`presentation` mirrors `presentation.json` and may name only what is there.
Objects merge key by key; an array must have the base's length, and a `null`
element leaves that element as it is. A locale changes **copy only**:

- everything under `menu_text`, the match's own copy included:
  `menu_text.hud` (the identity box's team labels and the HEALTH and PAINT
  meter labels; its device line is `menu_text.labels`),
  `menu_text.countdown` (the GET READY, RESUMING and REPLAY headings) and
  `menu_text.controls` (the in-match controls reminder, its KEYBOARD + MOUSE
  and GAMEPAD headings, the Controls help's "per pad", and the words among
  the control labels - see [Control labels](#control-labels)) and
  `menu_text.stage_names` (the shipped stages' titles - see
  [Stage names](#stage-names));
- `main_menu.weapon_descriptions.*` and `main_menu.weapon_select.choices.*.name`;
- `main_menu.credits.text`;
- `team_palette.pairs[].label`.

Anything else - a colour, a frame, a sound, a number, a box size, a pair's
`key` - is the shell's look and is the same in every language, so a locale that
names it is refused, as is a key `presentation.json` lacks or a value of a
different JSON type. Long text that does not fit is not a reason to widen a
box from a locale; the box is the base's, and the fix is shorter copy or a
layout change that every language shares.

`fonts` (optional) substitutes one sprite font for another everywhere
`presentation.json` names it: in any key called `font`, any key ending
`_font`, and any member of a `fonts` object. A key must be a font
`presentation.json` uses; the replacement must be a manifest font, which the
resource checks confirm. This is how a language whose letters the shipped
atlases lack gets its own (see [Font coverage](#font-coverage)).

Only the root keys `presentation` and `fonts` are allowed. Files are UTF-8; a
byte order mark, which some editors add, is ignored.

## Choosing a locale

`COLOURWARS_LOCALE=<tag>` chooses the run's locale, test locales included, in
Debug and Release alike. Unset or empty means the base. A tag the index does
not list starts the game in the base with one stderr line:

```text
locale: 'zz' is not listed in locales/locales.json; using 'en'.
```

A broken locale is a content bug and stops startup, chosen or not: every
listed locale is composed at startup, so an unknown key or a non-copy value in
any of them fails before the first menu. The chosen one is also judged by every
text, font and fit check; the `locales` acceptance case judges every listed one
that way, so CI fails a translation that cannot draw or does not fit before a
player could pick it.

A build without `locales/locales.json` has only the base.

There is no language row on Options, and the save has no locale field. Both
wait for the first language that is not a test locale (none was chosen on
2026-11-05): a picker that
offers only English would be noise. When one ships, the row goes on Options and
the choice in the save, read before `Presentation` the way the resolution is;
`locale_selection::requested_from_environment` is where that joins. A change
of language applies at the next launch: pages capture their copy when they are
built, and `Presentation` is not rebuilt in Release.

### The Debug tools

`reload` and `tune` rebuild `Presentation` through the run's locale, so the
copy stays in that language. Both read `presentation.json` again; the locale
files are read once, at startup. A `tune` of a key the locale overrides edits
the base, and the locale's text still wins. `tune.save` writes only
`presentation.json`.

## Fallback

A string a locale leaves out comes from its `fallback`, then that locale's, and
so on to `presentation.json`. The nearest locale wins each string and each font
substitution. `fr-CA` falling back to `fr` can carry only the strings Canadian
French says differently.

A missing string is never an error and never a blank: the base always has it.

## Font coverage

Authored text is printable ASCII, `U+0020` to `U+007E`, after the typographic
punctuation `drawable_text` folds (curly quotes, dashes, ellipsis, no-break
spaces); see [drawable_text.h](../game/config/drawable_text.h) and the table
it reads, [drawable_text_contract.h](../game/contracts/drawable_text_contract.h),
which the level editor's display-name check reads too. The seven
shipped atlases carry exactly that range
([font recipe](../game/art/README.md#sprite-font-recipe-gap87)). A locale is held to
the same two checks as the base:

1. while `Presentation` reads it, every string must convert to drawable text;
2. once the fonts load, every string must be drawable by the font that draws
   it, character by character.

So today a translation can use only the letters English uses. French, Spanish,
German and Portuguese need accented letters; a language with a non-Latin
script needs a different face. Adding one is three steps, in this order:

1. Add the character range to the recipe
   ([`fonts.json`](../game/art/sources/fonts/fonts.json)) and regenerate the
   atlases (`scripts/build-fonts.ps1`), or add new atlases and name them in
   the manifest and the locale's `fonts` table. Check the tracked face has
   the glyphs first; a script it lacks needs another OFL face.
2. Widen `drawable_text`'s accepted range to the new atlases' range.
3. Add the locale; the acceptance case then proves each string against the
   real atlases.

## Numbers

Numbers are not localised. Digits are ASCII `0`-`9`, a decimal point is `.`,
and there is no digit grouping; the code formats every number (percentages,
durations, scores, counters, the stage count) the same way in every locale.
What a locale controls is the template around a number: `{value}%` can become
`{value} %`, and `{seconds}s` can become `{seconds} s` or `{seconds} sec`. The
tokens a template must carry are checked whatever the language.

## The test locale

`qps-ploc` is Windows' pseudo-locale tag. Each line of copy is wrapped in
`[ ]`, so text that is cut off or still English shows at a glance:

```text
COLOURWARS_LOCALE=qps-ploc out/build/x64-debug/game/ColourWars.exe
```

It covers every copy role a locale may change. It leaves out the two strings
with no room for two more characters, which fall back to English:
`menu_text.labels.keyboard_mouse` and `menu_text.labels.roster_row` (the Team
Select roster row must end before the first choice icon). A translation longer
than English by more than a few characters will meet the same limit there.

## Control labels

A control's label is a legend or a word. The legends - the letters `A`-`Z`
and the pad's `A`, `B`, `X`, `Y`, `LB`, `RB`, `LT`, `RT`, `LS` and `RS` - are
what is printed on the key or button, the same in every language as digits
are, and stay in code ([input_bindings.cpp](../game/input/input_bindings.cpp)),
as does the ` / ` between a primary control and its alternate. The words are
`menu_text.controls.labels`: `space`, `escape` (the fixed pause key), `none`
(an unbound Pause key or alternate), `start` and the five mouse buttons
(`mouse_left` ... `mouse_x2`). [controls_text](../game/input/controls_text.h)
puts the two together for the reminder, the Controls help, the Keyboard Keys
and Pad Buttons rows and the Keys capture status. `input_bindings.cpp`'s own
labels are the base locale's twins of those words, for probes and
diagnostics; the `menu_text` case fails if the two drift. The save tokens
(`"space"`, `"mouse_left"`, `"start"`, ...) are a file format and never
change with a locale.

## Stage names

A shipped stage's title - Stage Select's arena name and the countdown's
stage-entry card, which every Random, series and losers' pick resolves to -
is `menu_text.stage_names`, an object keyed by the manifest's stage id:

```json
"stage_names": { "king_of_the_hill": "King of the Hill", "turbulence": "Turbulence" }
```

Its English is a mirror of each level file's `display_name`, kept honest by
two startup checks ([content_validation.h](../game/config/content_validation.h)):
`validate_stage_names` requires an entry for every stage the manifest lists
and none for anything else, in every locale, and `check_stage_names_agree`
requires each base entry to equal its level's `display_name` byte for byte. The
second is judged on the base `presentation.json`, not the composed locale, at
startup, by the Debug `reload` and by the presentation `tune` target. Both fail
startup, as a missing level does. A stage the editor registers gets its entry
from the registration tool ([Registration assistance](level-editor.md#registration-assistance-ed4)),
so a registered stage cannot fail the check, and a shipped stage's id may not
start with `_`, which the table would read as a comment.

A locale names any subset; a stage it leaves out keeps the base's English.
Registration writes only the base, so a newly registered stage is untranslated
in every locale, `qps-ploc` included, until someone adds it there: add
`"<name>": "[<display_name>]"` to `qps-ploc.json`'s `stage_names` to keep the
test locale bracketing every stage. Nothing fails meanwhile - the `locales`
case brackets the stages `qps-ploc` names and holds the rest to the base, and
the `menu_text` case pins the stages shipped when it was written and lets a
later one join. A locale
cannot add a stage: an id `presentation.json` lacks is refused like any other
unknown key. A **user stage** (Save to My Stages, `ROAD#16`) is never in the
table and draws its own `display_name` in every locale, untranslated: it is
the player's own text. `MenuText::stage_title` is the one reader, so a
fixture stage the table does not name falls back the same way.

## Not yet localisable

These still come from somewhere a locale does not reach:

- **Practice lesson cards.** Movement Course's station lessons (`DROP
  THROUGH`, `SHORT HOP`, `HELD JUMP` ...) are each station's `lesson.title`
  and `lesson.text` in [movement_course.json](../game/content/levels/movement_course.json),
  level copy that no locale reaches.
- **The countdown's `GO!`**, the start signal's last beat
  ([countdown_cards.cpp](../game/objects/countdown_cards.cpp),
  [level.cpp](../game/objects/level.cpp),
  [level_practice.cpp](../game/objects/level_practice.cpp)). It is also an
  identifier: [countdown_art.cpp](../game/objects/countdown_art.cpp) compares
  the label against `L"GO!"` to choose the go beat's look, so moving it needs
  that comparison replaced by a flag first.
- **User-stage scan details.** Stage Select's Scan details page (Down while
  browsing, Release included) draws each refused file as `user stage refused:
  <file>: <reason>`, built in C++ by
  [stage_loading.cpp](../game/config/stage_loading.cpp) with English reasons
  (`is not a folder`, the name rule, a loader's exception text). The page's
  heading, prompts and counter are `menu_text.stage_scan`; the lines are the
  same text stderr prints, so the reasons need codes before they can be copy.
- **System dialogs.** The first-save failure warning
  ([save.cpp](../game/save/save.cpp): `Could not write <path>. Your settings
  will not be remembered.`, captioned `ColourWars - save failure`) and the
  startup/runtime failure report
  ([fatal_report.cpp](../game/fatal_report.cpp)) are `MessageBoxW` text in
  C++. Both can fire before a locale exists: `Save` writes its first-run
  defaults before `wWinMain` reads the locales, and the failure report covers
  a failure to load them. Localising them means reading the locale earlier,
  or deciding they stay English.
- **Developer text**: the Debug console, overlays and diagnostics stay English.

All but the last need moving into data (or, for the dialogs, a decision to
keep them English) before a real translation ships; developer text is not
player-facing.

## Checks

- [`scripts/acceptance/locales.cpp`](../scripts/acceptance/locales.cpp)
  composes every shipped locale over `presentation.json` and runs startup's
  text checks on it against the real atlases, then holds the composition
  rules with fixtures: the three-step fallback chain, font substitution, null
  array elements, a translated stage name, and the refusals (unknown key, an
  unknown stage id, non-copy value, number, changed shape, array length, bad
  tag, duplicate, a fallback not listed above, a missing file, an undrawable
  language name, an unknown root key, a character outside the fonts, in a stage
  name too, a line that does not fit). Its judge names every shipped stage
  through `stage_names` in every locale, and checks the base agrees with the
  levels; `qps-ploc` must bracket each stage it names and leave the rest to
  the base.
- [`scripts/acceptance/user_stages.cpp`](../scripts/acceptance/user_stages.cpp)
  holds the stage-name checks against the stage list: a missing, extra, user
  or `_` entry and a drifted `display_name` are refused; a user stage keeps its
  own name.
- CI starts both builds with `check-startup.ps1 -Locale qps-ploc`, so the test
  locale passes the whole of startup, stage text and Mode rows included.
- `cmake/runtime_assets.cmake` stages and packages `locales/`;
  `cmake/release_assets.cmake` and the level editor's stage registration
  check that the list names the index and every file it lists.
- The level editor's registration adds a new stage's `stage_names` entry
  with the manifest and runtime-list entries, and refuses a stage named in only
  some of the three; its status line warns when a registered source level's
  `display_name` no longer matches its entry (`check-level-editor.ps1`,
  `check-stage-registration.ps1`).
