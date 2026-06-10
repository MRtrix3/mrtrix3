#!/usr/bin/perl
#
# Copyright (c) 2008-2026 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

# Heavy-lifting engine for the repository's `check_syntax` script.
#
# Both are handled by a single mechanism, the char -> source-line interval map:
# every transform the original performed (drop `^#` lines, blank `//`, join,
# collapse `\s+`, strip `/*...*/`, remove strings) is reproduced while a
# parallel array @map is kept in lockstep, so @map[i] is the source line of the
# i-th character of the working buffer.  A match at byte offset [ms,me) maps to
# source lines @map[ms] .. @map[me-1] -- an exact line, or a line range for a
# genuinely multi-line match.  The bytes the regexes see are pristine (no
# in-band markers), so matching behaviour is identical to the original.
#
# Diff-mode block separation: the engine processes a LIST OF SEGMENTS.  Each
# diff hunk is a separate segment; every line-joining transform operates within
# a single segment, so a match can never span two non-contiguous blocks.  The
# boundary is therefore structural -- it never enters any buffer the regexes
# inspect, and there is no token to strip or to accidentally match.
#
# Usage:
#   check_syntax_engine.pl [--format human|tsv] FILE [FILE ...]
#       Non-diff: read each file directly, treat it as one segment, tagging
#       lines by their source number and dropping `// ... check_syntax off`.
#   check_syntax_engine.pl --diff --label REL/PATH [--format human|tsv]
#       Diff: read a tagged stream on stdin -- one "<line>\t<content>" record
#       per line, a bare "--" line separating segments (hunks).
#
# Exit status: 1 if any offending line was reported, 0 otherwise.

use strict;
use warnings;

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
my @files;
my $diff = 0;
my $label = '';
my $format = 'human';
while (@ARGV) {
  my $a = shift @ARGV;
  if    ($a eq '--diff')   { $diff = 1; }
  elsif ($a eq '--label')  { $label = shift @ARGV; }
  elsif ($a eq '--format') { $format = shift @ARGV; }
  elsif ($a eq '--file')   { push @files, shift @ARGV; }
  elsif ($a =~ /^-/)       { die "unknown argument: $a\n"; }
  else                     { push @files, $a; }
}
die "unknown --format '$format'\n" unless $format eq 'human' || $format eq 'tsv';

# ---------------------------------------------------------------------------
# In-house syntax-check regular expressions.
#
# Each pattern encodes one house-style rule.  They are collected here, away from
# the scanning logic, so the rule set can be reviewed and amended in one place.
# For every pattern the comment names the check performed and the working buffer
# the pattern is matched against (see check_file() for how each buffer is built
# from a segment's records):
#   raw     : source lines verbatim (suppressed `check_syntax off` lines removed)
#   guard   : per-segment; // blanked, whitespace collapsed, C comments stripped
#   buffer2 : per-segment; `^#` lines dropped and trailing // comments blanked
#   buffer3 : buffer2 joined into one string, whitespace collapsed, and
#             C-style /*...*/ comments stripped
#   buffer4 : buffer3 with quoted string literals removed
# ---------------------------------------------------------------------------

# A: #define of a numeric/string constant -- applied per line to the raw buffer.
my $RE_define_constant =
  qr/^#define\s+[a-zA-Z_][a-zA-Z0-9_]*\s+(0x[\dA-F]+|0b[01]+|".*"|[-+]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)(?:[eE][-+]?[0-9]+)?)/;

# B: deprecated `#ifndef __X__ / #define __X__` include guard -- guard buffer.
my $RE_include_guard = qr/\#ifndef\s+__.*__\s+\#define\s+__.*__/;

# C: Eigen header included before eigen_plugins/eigen_plugins.h -- applied per
# #include line of the raw buffer (the order check is stateful; see check_file).
my $RE_include_any     = qr/^#include /;
my $RE_include_plugins = qr/#include "eigen_plugins\/eigen_plugins\.h"/;
my $RE_include_eigen   = qr/(#include <(?:unsupported\/)?Eigen\/[\w\/]+>)/;

# D: std::string_view used as a function return type -- per line to buffer2.
my $RE_string_view_return =
  qr/^\s*(const\s+|)std::string_view\s?\&?([a-zA-Z_][a-zA-Z0-9_]*|operator.*)\s?\(.*\)/;

# E: %zu inside a print() call -- applied to buffer3 (two-stage; see postfilter).
my $RE_print_zu = qr/\w*print\w*.*?%\d?zu\b/;
# E (stage 2): isolate the innermost print(...) call wrapping the %zu, for the report.
my $RE_print_zu_call = qr/(\w*print(?!.*?print).*?$)/;

# F: C-style array declaration -- applied to buffer3.
my $RE_c_array = qr/\b\w+(::\w+)*(?<!\breturn)(?<!\bnew)\s\w+(\[\d*\])+\s*[=;,]/;

# G: char* / char item[] -- applied to buffer4.
my $RE_c_char =
  qr/(const\s+|)char(\s?\*(\s?const|)\s?(?![>\(\)])|\s+[a-zA-Z_][a-zA-Z0-9_]*\[(\d*\]|))/;

# H: C-style string-manipulation functions (incl. strerror) -- applied to buffer4.
my $RE_c_string_fn =
  qr/\b(strcpy|strncpy|strcat|strncat|strxfrm|strlen|strcmp|strncmp|strcoll|strchr|strrchr|strspn|strcspn|strpbrk|strstr|strtok|wcscpy|wcsncpy|wcscat|wcsncat|wcsxfrm|wcslen|wcscmp|wcsncmp|wcscoll|wcschr|wcsrchr|wcsspn|wcscspn|wcspbrk|wcsstr|wcstok|strerror)\s*\(/;

# I: getenv() -- applied to buffer4.
my $RE_getenv = qr/\bgetenv\s*\(/;

# J: rand()/srand() -- applied to buffer4.
my $RE_rand = qr/\b(srand|rand)\s*\(/;

# K: const std::string & function argument -- applied to buffer4.
my $RE_const_string_ref = qr/\bconst\sstd::string\s?\&/;

# L: sequential namespaces that could be nested -- applied to buffer4.
my $RE_nested_namespace = qr/\bnamespace [\w\:]+\s*\{(\s*namespace [\w\:]+\s*\{)+/;

# M: using namespace std; -- applied to buffer4.
my $RE_using_namespace_std = qr/\busing\snamespace\sstd\s?;/;

# N: C-style casts -- applied to buffer4.
my $RE_c_cast =
  qr/(\((?!void\))[a-zA-Z][a-zA-Z0-9_]*\)[a-zA-Z][a-zA-Z0-9_]*|\([a-zA-Z][a-zA-Z0-9_]*\s?\*(\s?const)?\)[a-zA-Z][a-zA-Z0-9_]*|\(\s?void\s?\*(\s?const)?\)[\w&(]|(?<!\bstd::function<)\b(GLint|GLfloat|c?float|c?double|(default|value|index)_type|[^\w_](unsigned\s+)?int|u?int(8|16|32|64)_t|s?size_t|ValueType|(?<!Scalar)::Scalar)\(\(*\**(?![\d\-\.]*\)))/;

# O: NULL / NAN / INFINITY macros -- applied to buffer4.
my $RE_macro_null_nan = qr/\b(NAN|INFINITY|NULL)\b/;

# P: std::abs() or naked abs() -- applied to buffer4 (skipped in cpp/core/types.h).
my $RE_abs = qr/((?<!::)std::|)(?<!MR::)(?<!\.)\babs\s?\(/;

# Q: std::visit() with a lambda -- applied to buffer4.
my $RE_visit_lambda = qr/std::visit\s?\(\s?\[\s?(&?\w+(,\s?&?\w+)*)?\]\s?\(/;

# ---------------------------------------------------------------------------
# Transform / scan primitives that keep (string, @map) in lockstep.
# A "segment" is an arrayref of [line_number, text] records (text without its
# trailing newline).  @map entries are source line numbers.
# ---------------------------------------------------------------------------

# Drop a `// ... check_syntax off` line (mirror of the original `grep -v`).
sub is_suppressed { return $_[0] =~ m{//.*\bcheck_syntax\soff}; }

# Blank a trailing // comment on one line (mirror of perl -pe 's|//.*$||').
sub blank_line_comment {
  my ($t) = @_;
  $t =~ s{//.*$}{};
  return $t;
}

# Join a segment's records into ($string, \@map); each terminating newline
# becomes a single space (mirror of `tr '\n' ' '`).  Optionally blank //.
sub join_records {
  my ($recs, $blank_comments) = @_;
  my $s = '';
  my @map;
  for my $r (@$recs) {
    my ($ln, $txt) = @$r;
    $txt = blank_line_comment($txt) if $blank_comments;
    my $len = length $txt;
    for (my $i = 0; $i < $len; $i++) {
      $s .= substr($txt, $i, 1);
      push @map, $ln;
    }
    $s .= ' ';        # the line's terminating newline -> space
    push @map, $ln;
  }
  return ($s, \@map);
}

# Collapse runs of whitespace to a single space (mirror of s|\s+| |g).  The
# surviving space inherits the line of the LAST whitespace character in the run
# (the indent abutting the next token), so a leading-\s match begins on the
# token's own line.
sub collapse_ws {
  my ($s, $map) = @_;
  my $ns = '';
  my @nm;
  my $len = length $s;
  my $i = 0;
  while ($i < $len) {
    if (substr($s, $i, 1) =~ /\s/) {
      my $j = $i;
      $j++ while ($j < $len && substr($s, $j, 1) =~ /\s/);
      $ns .= ' ';
      push @nm, $map->[$j - 1];
      $i = $j;
    } else {
      $ns .= substr($s, $i, 1);
      push @nm, $map->[$i];
      $i++;
    }
  }
  return ($ns, \@nm);
}

# Remove every non-overlapping match of $re from ($string, @map), splicing both
# in lockstep (used for C-style comments and for quoted strings).
sub remove_matches {
  my ($s, $map, $re) = @_;
  my $ns = '';
  my @nm;
  my $prev = 0;
  while ($s =~ /$re/g) {
    my $ms = $-[0];
    my $me = $+[0];
    if ($me == $ms) { pos($s) = $ms + 1; next; }   # guard against zero-width
    $ns .= substr($s, $prev, $ms - $prev);
    push @nm, @{$map}[$prev .. $ms - 1] if $ms > $prev;
    $prev = $me;
  }
  $ns .= substr($s, $prev);
  push @nm, @{$map}[$prev .. $#$map] if $prev <= $#$map;
  return ($ns, \@nm);
}

# ---------------------------------------------------------------------------
# Hit collection
# ---------------------------------------------------------------------------
my @hits;   # each: { l1 => start_line, l2 => end_line, rule => '...', text => '...' }

sub add_hit {
  my ($l1, $l2, $rule, $text) = @_;
  push @hits, { l1 => $l1, l2 => (defined $l2 ? $l2 : $l1), rule => $rule, text => $text };
}

# Scan a joined ($string, @map) for $re, recording each match's line range.
# Optional $postfilter rewrites the reported text (used for the two-stage %zu
# check); returning undef drops the hit.
sub scan {
  my ($s, $map, $re, $rule, $postfilter) = @_;
  while ($s =~ /$re/g) {
    my $ms = $-[0];
    my $me = $+[0];
    if ($me == $ms) { pos($s) = $ms + 1; next; }
    my $text = substr($s, $ms, $me - $ms);
    if ($postfilter) { $text = $postfilter->($text); next unless defined $text; }
    add_hit($map->[$ms], $map->[$me - 1], $rule, $text);
  }
}

# Scan line-anchored checks per record (the P1 regime: ^-anchored, single line).
sub scan_lines {
  my ($recs, $re, $rule) = @_;
  for my $r (@$recs) {
    my ($ln, $txt) = @$r;
    while ($txt =~ /$re/g) {
      my $ms = $-[0];
      my $me = $+[0];
      if ($me == $ms) { pos($txt) = $ms + 1; next; }
      add_hit($ln, $ln, $rule, substr($txt, $ms, $me - $ms));
    }
  }
}

# ===========================================================================
# The check suite, applied to one file's segments.
# ===========================================================================
sub check_file {
  my ($segments, $path) = @_;
  @hits = ();

  my $is_types_h       = ($path =~ m{(?:^|/)cpp/core/types\.h$});
  my $is_eigen_plugins = ($path =~ m{(?:^|/)cpp/core/eigen_plugins/eigen_plugins\.h$});

  # All records in file order (for the per-line and whole-file structural checks).
  my @all = map { @$_ } @$segments;

  # --- whole-file / per-line checks (segment boundaries irrelevant) ---------

  # A: #define numeric/string constants (raw buffer, line-anchored).
  scan_lines(\@all, $RE_define_constant, 'define-constant');

  # C: Eigen header included before eigen_plugins/eigen_plugins.h.  Stateful in
  # include order across the whole file; reported at the offending #include.
  unless ($is_eigen_plugins) {
    my $plugin_seen = 0;
    for my $r (@all) {
      my ($ln, $txt) = @$r;
      next unless $txt =~ /$RE_include_any/;
      if ($txt =~ /$RE_include_plugins/) { $plugin_seen = 1; }
      elsif (!$plugin_seen && $txt =~ /$RE_include_eigen/) {
        add_hit($ln, $ln, 'eigen-plugin-order', $1);
      }
    }
  }

  # --- per-segment joined checks (a match can never cross a segment) ---------
  for my $seg (@$segments) {

    # B: deprecated #ifndef __X__ / #define __X__ include guard (guard buffer).
    {
      my ($s, $m) = join_records($seg, 1);
      ($s, $m) = collapse_ws($s, $m);
      ($s, $m) = remove_matches($s, $m, qr{/\*.*?\*/});
      scan($s, $m, $RE_include_guard, 'include-guard');
    }

    # buffer2: drop ^# lines, blank // comments.
    my @b2 = map { [ $_->[0], blank_line_comment($_->[1]) ] }
             grep { $_->[1] !~ /^#/ } @$seg;

    # D: std::string_view as a function return type (buffer2, line-anchored).
    scan_lines(\@b2, $RE_string_view_return, 'string_view-return');

    # buffer3: join, collapse, strip C-style comments.
    my ($s3, $m3) = join_records(\@b2, 0);
    ($s3, $m3) = collapse_ws($s3, $m3);
    ($s3, $m3) = remove_matches($s3, $m3, qr{/\*.*?\*/});

    # E: %zu inside a print() call (buffer3, two-stage like the original).
    scan($s3, $m3, $RE_print_zu, 'print-zu', sub {
      my ($t) = @_;
      return ($t =~ /$RE_print_zu_call/) ? $1 : $t;
    });

    # F: C-style array declarations (buffer3).
    scan($s3, $m3, $RE_c_array, 'c-array');

    # buffer4: remove quoted strings.
    my ($s4, $m4) = remove_matches($s3, $m3, qr/(")(\\"|.)*?"/);

    # G: char* / char item[] (buffer4).
    scan($s4, $m4, $RE_c_char, 'c-char');

    # H: C-style string manipulation functions, incl. strerror (buffer4).
    scan($s4, $m4, $RE_c_string_fn, 'c-string-fn');

    # I: getenv() (buffer4).
    scan($s4, $m4, $RE_getenv, 'getenv');

    # J: rand()/srand() (buffer4).
    scan($s4, $m4, $RE_rand, 'rand');

    # K: const std::string & (buffer4).
    scan($s4, $m4, $RE_const_string_ref, 'const-string-ref');

    # L: namespaces that could be nested (buffer4).
    scan($s4, $m4, $RE_nested_namespace, 'nested-namespace');

    # M: using namespace std; (buffer4).
    scan($s4, $m4, $RE_using_namespace_std, 'using-namespace-std');

    # N: C-style casts (buffer4).
    scan($s4, $m4, $RE_c_cast, 'c-cast');

    # O: NULL / NAN / INFINITY macros (buffer4).
    scan($s4, $m4, $RE_macro_null_nan, 'macro-null-nan');

    # P: std::abs() or naked abs(), buffer4 (skipped in cpp/core/types.h).
    unless ($is_types_h) {
      scan($s4, $m4, $RE_abs, 'abs');
    }

    # Q: std::visit() with a lambda (buffer4).
    scan($s4, $m4, $RE_visit_lambda, 'visit-lambda');
  }

  return [ @hits ];
}

# ---------------------------------------------------------------------------
# Input readers -> list of segments
# ---------------------------------------------------------------------------

# Non-diff: one segment containing every (non-suppressed) line of the file.
sub read_file_segments {
  my ($path) = @_;
  open(my $fh, '<', $path) or die "cannot open $path: $!\n";
  my @seg;
  while (my $line = <$fh>) {
    chomp $line;
    next if is_suppressed($line);
    push @seg, [ $., $line ];
  }
  close $fh;
  return [ [ @seg ] ];   # one segment containing every record
}

# Diff: "<line>\t<content>" records, a bare "--" line starting a new segment.
sub read_tagged_segments {
  my @segments = ([]);
  while (my $row = <STDIN>) {
    chomp $row;
    if ($row eq '--') { push @segments, []; next; }
    my ($ln, $txt) = split(/\t/, $row, 2);
    $txt = '' unless defined $txt;
    next if is_suppressed($txt);
    push @{$segments[-1]}, [ $ln + 0, $txt ];
  }
  return [ grep { @$_ } @segments ];
}

# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------
sub emit {
  my ($path, $file_hits) = @_;
  return 0 unless @$file_hits;
  my @sorted = sort {
    $a->{l1} <=> $b->{l1} || $a->{l2} <=> $b->{l2} || $a->{rule} cmp $b->{rule}
  } @$file_hits;

  if ($format eq 'tsv') {
    for my $h (@sorted) {
      print join("\t", $path, $h->{l1}, $h->{l2}, $h->{rule}, $h->{text}), "\n";
    }
  } else {
    print "################################### $path\n";
    for my $h (@sorted) {
      my $loc = ($h->{l1} == $h->{l2}) ? $h->{l1} : "$h->{l1}-$h->{l2}";
      printf "%s:%s: %-22s %s\n", $path, $loc, "[$h->{rule}]", $h->{text};
    }
  }
  return 1;
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
my $any = 0;
if ($diff) {
  my $segments = read_tagged_segments();
  $any = emit($label, check_file($segments, $label)) || $any;
} else {
  die "no input files\n" unless @files;
  for my $f (@files) {
    my $segments = read_file_segments($f);
    $any = emit($f, check_file($segments, $f)) || $any;
  }
}
exit($any ? 1 : 0);
