// Regression fixture for the check_syntax engine (testing/tools/check_syntax_engine.pl).
//
// Every line that the engine MUST flag carries a trailing "EXPECT:" marker
// naming the rule(s) it triggers, e.g.   some_code();   EXPECT: rule-a, rule-b
// placed on the START line of the match (matches may span two lines).  Any line
// without an EXPECT marker MUST NOT be flagged; most such lines are deliberate
// near-misses chosen to catch a regex that is not careful about word
// boundaries, required punctuation, or surrounding context.
//
// Comments are stripped before the pattern checks run, so the markers
// themselves never trigger a rule.  This file is parsed only by check_syntax;
// no build target compiles it, so it need not be valid C++.
//
// clang-format is disabled below: reformatting would alter the exact spacing
// (pointer placement, cast layout, ...) that several checks key off, changing
// which lines match.
// clang-format off

#ifndef __OFFENDERS_H__   // EXPECT: include-guard
#define __OFFENDERS_H__

// =====================================================================
// define-constant : #define of a literal constant (use constexpr instead)
// =====================================================================
#define DC_INT 42                 // EXPECT: define-constant
#define DC_NEG -7                 // EXPECT: define-constant
#define DC_PLUS +5                // EXPECT: define-constant
#define DC_FLOAT 3.14             // EXPECT: define-constant
#define DC_SCI 6.022e23           // EXPECT: define-constant
#define DC_HEX 0xFF               // EXPECT: define-constant
#define DC_BIN 0b1010             // EXPECT: define-constant
#define DC_STR "literal"          // EXPECT: define-constant
// near-misses (must stay clean):
#define DC_FUNC(x) ((x) + 1)
#define DC_EXPR alpha + beta
#define DC_EMPTY
#define DC_CALL compute()
#define DC_CONCAT a ## b

// =====================================================================
// macro-null-nan : NULL / NAN / INFINITY macros
// =====================================================================
ptr = NULL;                       // EXPECT: macro-null-nan
flt = NAN;                        // EXPECT: macro-null-nan
inf = INFINITY;                   // EXPECT: macro-null-nan
// near-misses:
nullable = MY_NULL;
prefixed = NULLABLE;
lower = nan;
camel = NaN;

// =====================================================================
// abs : std::abs() or naked abs() (only MR::abs() is allowed)
// =====================================================================
a0 = abs(x);                      // EXPECT: abs
a1 = std::abs(x);                 // EXPECT: abs
a2 = abs (x);                     // EXPECT: abs
// near-misses:
a3 = MR::abs(x);
a4 = obj.abs(x);
a5 = fabs(x);
a6 = std::fabs(x);

// =====================================================================
// getenv : getenv() / std::getenv()
// =====================================================================
g0 = getenv("HOME");              // EXPECT: getenv
g1 = std::getenv("HOME");         // EXPECT: getenv
// near-misses:
g2 = my_getenv("HOME");
g3 = getenvironment();

// =====================================================================
// rand : rand() / srand()
// =====================================================================
r0 = rand();                      // EXPECT: rand
r1 = srand(1);                    // EXPECT: rand
// near-misses:
r2 = my_rand();
r3 = random();
r4 = operand(x);

// =====================================================================
// c-string-fn : C string-manipulation functions
// =====================================================================
s0 = strlen(p);                   // EXPECT: c-string-fn
s1 = strcpy(a, b);                // EXPECT: c-string-fn
s2 = strerror(e);                 // EXPECT: c-string-fn
s3 = wcslen(w);                   // EXPECT: c-string-fn
// near-misses:
s4 = my_strlen(p);
s5 = strlength(p);

// =====================================================================
// c-char : char* and char arrays (not casts)
// =====================================================================
char* cc0;                        // EXPECT: c-char
char *cc1;                        // EXPECT: c-char
char * cc2;                       // EXPECT: c-char
const char* cc3;                  // EXPECT: c-char
char cc4[10];                     // EXPECT: c-char, c-array
char cc5[];                       // EXPECT: c-char, c-array
// near-misses:
character_count = 5;
char_type cc6;
std::vector<char> cc7;

// =====================================================================
// using-namespace-std
// =====================================================================
using namespace std;              // EXPECT: using-namespace-std
// near-misses:
using namespace other;
using std::string;
using namespace std::chrono;

// =====================================================================
// const-string-ref : const std::string& (prefer std::string_view arg)
// =====================================================================
void k0(const std::string& s);    // EXPECT: const-string-ref
void k1(const std::string &s);    // EXPECT: const-string-ref
// near-misses:
void k2(const std::string s);
void k3(const std::string_view& s);
void k4(std::string& s);

// =====================================================================
// string_view-return : std::string_view as a return type
// =====================================================================
std::string_view sv0();           // EXPECT: string_view-return
const std::string_view sv1();     // EXPECT: string_view-return
std::string_view &sv2();          // EXPECT: string_view-return
// near-misses:
void sv3(std::string_view s);
std::string_view sv4 = source();
using sv5 = std::string_view;

// =====================================================================
// c-cast : C-style casts
// =====================================================================
n0 = (int)x;                      // EXPECT: c-cast
n1 = (MyType)y;                   // EXPECT: c-cast
n2 = (char*)p;                    // EXPECT: c-cast
n3 = (char* const)q;              // EXPECT: c-cast, c-char
n4 = (void*)addr;                 // EXPECT: c-cast
n5 = (void *)&ref;                // EXPECT: c-cast
n6 = (unsigned)q;                 // EXPECT: c-cast
n7 = float(val);                  // EXPECT: c-cast
n8 = size_t(count);               // EXPECT: c-cast
n9 = uint32_t(m);                 // EXPECT: c-cast
n10 = value_type(v);              // EXPECT: c-cast
n11 = Foo::Scalar(z);             // EXPECT: c-cast
// near-misses:
n12 = (void)expr;
n13 = float(3.14);
n14 = int(42);
n15 = double(-1.0);
std::function<double(double)> n16;
n17 = compute(arg);
if (cond) n18 = 1;
n19 = Scalar::Scalar(z);

// =====================================================================
// visit-lambda : std::visit() with a lambda
// =====================================================================
q0 = std::visit([](auto v){}, x); // EXPECT: visit-lambda
q1 = std::visit([&y](auto v){}, x); // EXPECT: visit-lambda
q2 = std::visit([a, b](auto v){}, x); // EXPECT: visit-lambda
// near-misses:
q3 = std::visit(visitor, x);
q4 = boost::visit([](auto v){}, x);

// =====================================================================
// eigen-plugin-order : Eigen header before eigen_plugins/eigen_plugins.h
// =====================================================================
#include <Eigen/Core>             // EXPECT: eigen-plugin-order
#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/Dense>

// =====================================================================
// nested-namespace : sequentially-defined namespaces (use nested form)
// =====================================================================
namespace NsA { namespace NsB {   // EXPECT: nested-namespace
} }
namespace NsSolo { int solo_x; }

// =====================================================================
// c-array : C-style array declarations
// =====================================================================
int ca0[3];                       // EXPECT: c-array
float ca1[] = {};                 // EXPECT: c-array
int ca2[2][3];                    // EXPECT: c-array
long                              // EXPECT: c-array
ca3[5];
// near-misses:
data[idx] = 5;
values = matrix[i];
new_buffer = new int[n];

// =====================================================================
// check_syntax off : suppression must drop the whole line
// =====================================================================
suppressed = strlen(p); // check_syntax off

// =====================================================================
// print-zu : %zu in a print() call (kept LAST; see note below)
// Ordered so each print is consumed by its own %zu and no stray %zu
// follows the final no-%zu print, avoiding whole-file-join artefacts.
// =====================================================================
z0 = print("x = %zu", a);         // EXPECT: print-zu
z1_decoy = "%zu";
z2 = myprintf("n = %3zu", b);     // EXPECT: print-zu
z3 = printf("%d", c);

#endif
