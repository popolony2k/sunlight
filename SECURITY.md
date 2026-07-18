# Security Policy

## Supported Versions

`sunlight` has not yet had a tagged release (it's currently pre-`0.1.0`). Security
fixes are made against `main`, which is the only branch supported at this time.

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Instead, use GitHub's private vulnerability reporting:

1. Go to the [Security tab](https://github.com/popolony2k/sunlight/security) of this repository.
2. Click **"Report a vulnerability"**.
3. Describe the issue, including steps to reproduce it and its potential impact.

This opens a private advisory visible only to the maintainers until a fix is
ready, so details aren't disclosed before users can update.

You can expect an initial response within a few days. If the report is confirmed,
we'll work with you on a fix and coordinate disclosure timing before any public
advisory or release is published.

## Scope

`sunlight` is a game engine library that wraps [raylib](https://www.raylib.com/)
and [libtmx](https://github.com/baylej/tmx.git). Vulnerabilities in those upstream
projects should generally be reported to them directly, unless `sunlight`'s own
code introduces or amplifies the issue (e.g. unsafe handling of a loaded map or
asset file).
