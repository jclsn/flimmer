<!--
SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guidelines for contributors and coding agents

clang-format, clang-tidy, gersemi and REUSE run in CI and enforce formatting,
most C++ idioms, CMake style and licence headers. This file covers the taste
that those tools can't check. When in doubt, follow the Linux kernel style:
short functions, little nesting, readable at a glance.

## Code structure

- Keep functions short and flat. Handle errors and special cases first and
  return early instead of nesting `if`s and loops.
- One function does one thing. Split a function once it needs comments to
  separate its steps.
- Don't reformat code you aren't changing anyway.

## Vertical whitespace

- Put an empty line before and after `if` blocks and loops. It isn't needed
  directly after an opening brace or before a closing one.
- Put an empty line before `return` unless it is the only statement in its
  block. This also applies after a `Q_EMIT`:

  ```cpp
  if (!QDir().mkpath(directory())) {
      Q_EMIT failed(tr("Cannot create %1").arg(directory()));

      return;
  }
  ```

- Group related declarations and statements, and separate the groups with an
  empty line.

## Naming

- Members are `m_name`. File-local constants are `s_name` and live in an
  anonymous namespace at the top of the `.cpp` file.
- Put the unit in the name when it isn't obvious: `s_timeoutMs`,
  `s_maxPending` (bytes), `bufferedMs()`.
- Give every file-local constant a one-line comment saying what it limits or
  why it has its value.

## Comments

- Explain *why*, not *what*. Leave out comments that repeat the code.
- Write short, plain sentences that start with a capital letter and end with
  a period.
- Every class gets a comment above it saying what it is for and how it fits
  in, e.g. who feeds it data and who reads it.
- Refer to parameters and properties in single quotes: `'url'`.
- When you change behaviour, update the comments, doc blocks, log messages
  and user-visible strings that describe it.
- Don't add a co-authored by Claude into the commit message. Assisted-by: model (without email)
  is preferred.

## Files and includes

- Every file starts with an SPDX header. `.cpp` and `.qml` files use `//`
  comments, headers use a `/* */` block, Markdown uses `<!-- -->`.
- Headers use `#ifndef NAME_H` include guards with `#endif // NAME_H`.
- Include groups, separated by an empty line: the file's own header, project
  headers, Qt headers, then standard library headers.
- Forward-declare classes in headers where that is enough.

## Qt and QML

- Use `QStringLiteral`/`QLatin1String` for string literals, `Q_EMIT` and
  `Q_SIGNALS` instead of the bare keywords, and `tr()`/`qsTr()` for every
  user-visible string.
- User-visible text is short and plain, without jargon.
- When adding a setting or property, keep the same order everywhere it shows
  up: `Q_PROPERTY`, getter, member, `load()`/`save()` and the QML page.
- QML: `id` first, then properties, then signals and functions. Comment
  properties whose meaning or encoding isn't obvious (e.g. `-1 = Auto`).

## Tests

- New behaviour gets a Qt Test in `tests/`, registered in
  `tests/CMakeLists.txt` with `flimmer_add_test`.
- Keep network and file code apart from logic so the logic can be tested
  without them. Inject clocks and timings instead of sleeping in tests.
- Before changing user-visible strings or behaviour, search `tests/` for
  anything that depends on them.

## Commits

- Titles follow Conventional Commits with a capitalised imperative summary:
  `feat: Add playback buffer`, `fix(ui): Align settings rows`. A scope is
  welcome but not required.
- One logical change per commit. Never mix functional changes with
  formatting or refactoring, and keep unrelated fixes in separate commits.
- Write a short body that explains *why* the change is needed, then briefly
  *what* it does. The history has to stand on its own without the pull
  request description.
- Incorporate review feedback with `git commit --amend` or
  `git commit --fixup <sha>` plus an autosquash rebase, not with new
  "address review" commits. Squash WIP and polish commits.
- Don't add AI attribution or `Co-Authored-By` trailers for tools.
