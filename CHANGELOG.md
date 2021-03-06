# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## Unreleased

## [2.0.2] - 2021-03-05

- Improve encoding performance for about 50%.

## [2.0.1] - 2020-12-22

- No noticable changes.
- Remove unused headers for the sake of static build reliability.

## [2.0.0] - 2019-10-09

- Due to exception handling specifics the module doesn't raise Lua errors
  anymore. Instead, it returns `nil, "Error description"`.
  **(incompatible change)**

## [1.0.1] - 2018-05-24

### Added

- Support for decimal escapes: `&#37;` would be decoded as percent sign `%`.
- Support for hexdecimal escapes: `&#x25;` would be decoded as percent sign `%`.

## [1.0.0] - 2018-03-28

### Added

- Basic functionality
- Unit tests
- Luarock-based packaging
- Gitlab CI integration
