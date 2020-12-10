# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [2.1.0](https://github.com/ledgerhq/app-xrpr/compare/v2.1.0...v2.0.1) - 2020-12-10

### Added

- Codestyle enforced with clang format
- Github Action CI to check code style and compilation
- Project config for vscode
- Changelog file
- Swap support

### Changed

- More errors, less THROWs

### Fixed

- Overflow in xrp_print_amount and check return values
- Invalid integer size in isAllZeros
