#!/usr/bin/env bash
# Render every shipped template through a real `mcpp new`, then build and run it.
#
# The work lives in tools/test_template.py: it installs an isolated MCPP_HOME
# fixture for the candidate version, runs mcpp's own renderer, parses the
# rendered manifest (complete selector, version, features) with tomllib, and only
# then redirects the dependency to the fixture. Hand-substituting placeholders
# with sed is explicitly not evidence for a template.
#
# Usage: bash tools/template_smoke.sh [--template NAME] [--toolchain SPEC]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 "$ROOT/tools/test_template.py" "$@"
