#!/bin/bash
# Unit test for the check_syntax engine (check_syntax_engine.pl at the repo root).
#
# The fixture testing/data/check_syntax/offenders.cpp annotates every line the
# engine must flag with a trailing "// EXPECT: <rule>[, <rule> ...]" marker (on
# the START line of a multi-line match).  This test parses those markers as
# ground truth and asserts that the set of (line, rule) pairs the engine reports
# is exactly the set the markers declare -- catching both missed detections and
# false positives.  It also checks the diff-mode segment model in isolation.
#
# Any failing assertion makes the script exit non-zero, failing the test.
set -e

# The engine and fixture are served from the build tree (copied there at
# configuration time) via CHECK_SYNTAX_ENGINE / CHECK_SYNTAX_FIXTURE, set by
# CMake when the test is run through ctest.  Fall back to the source-relative
# locations so the script remains runnable by hand from a checkout.
DIR=$(dirname "${BASH_SOURCE[0]}")
ENGINE="${CHECK_SYNTAX_ENGINE:-$DIR/../../check_syntax_engine.pl}"
FIXTURE="${CHECK_SYNTAX_FIXTURE:-$DIR/../data/check_syntax/offenders.cpp}"

test -f "$ENGINE"
test -f "$FIXTURE"

# Ground truth: (line<TAB>rule) for every EXPECT marker.
expected=$(awk '
  match($0, /\/\/ EXPECT:[ ]*/) {
    rules = substr($0, RSTART + RLENGTH)
    n = split(rules, arr, /,[ ]*/)
    for (i = 1; i <= n; i++) { gsub(/[ ]+$/, "", arr[i]); if (arr[i] != "") print NR "\t" arr[i] }
  }
' "$FIXTURE" | LC_ALL=C sort)

# Engine detections: (line<TAB>rule), de-duplicated.
detected=$(perl "$ENGINE" --format tsv "$FIXTURE" \
  | awk -F'\t' '{print $2 "\t" $4}' | LC_ALL=C sort -u)

echo "expected $(printf '%s\n' "$expected" | grep -c .) (line, rule) pairs; detected $(printf '%s\n' "$detected" | grep -c .)"

if ! diff <(printf '%s\n' "$expected") <(printf '%s\n' "$detected"); then
  echo "FAIL: engine detections do not match EXPECT markers" >&2
  echo "  '<' = expected but not detected (missed); '>' = detected but not expected (false positive)" >&2
  exit 1
fi
echo "OK: fixture detections match expectation"

# The check_syntax-off suppression must drop its whole line: no detection should
# land on the line carrying that directive.
suppressed_line=$(grep -n 'check_syntax off' "$FIXTURE" | head -1 | cut -d: -f1)
if perl "$ENGINE" --format tsv "$FIXTURE" | awk -F'\t' -v L="$suppressed_line" '$2 == L { found = 1 } END { exit !found }'; then
  echo "FAIL: a 'check_syntax off' line was flagged" >&2
  exit 1
fi
echo "OK: check_syntax-off line suppressed"

# Diff-mode segment model: tokens that would form a c-array if merged must NOT
# match when split across two hunks (separated by a bare "--"), but MUST match
# (as a line range) when contiguous within one hunk.  perl exits non-zero when
# it reports hits, so "|| true" keeps "set -e" from aborting on the within case.
across=$(printf '10\tint\n--\n900\tbuffer[16];\n' | perl "$ENGINE" --diff --label x.cpp --format tsv || true)
if [ -n "$across" ]; then
  echo "FAIL: a match spanned two diff hunks: $across" >&2
  exit 1
fi
echo "OK: no match spans separate hunks"

within=$(printf '10\tint\n11\tbuffer[16];\n' | perl "$ENGINE" --diff --label x.cpp --format tsv || true)
if ! printf '%s\n' "$within" | awk -F'\t' '$2 == 10 && $3 == 11 && $4 == "c-array" { f = 1 } END { exit !f }'; then
  echo "FAIL: contiguous c-array not reported as range 10-11: $within" >&2
  exit 1
fi
echo "OK: contiguous multi-line match reported as line range"
