 ▐▛███▜▌   Claude Code v2.1.128
▝▜█████▛▘  Opus 4.7 · Claude Max
  ▘▘ ▝▝    ~/src/worktrees/fmt

❯ Custom fmtlib formatter for Eigen dense expressions at cpp/cmd/fmt.h:49 still 
   does not work for Eigen::Block despite use of std::remove_cv_t<> to deal     
  with differences in constness. Propose another solution to make this template 
   function applicable to Eigen::BLock.                                         

  Searched for 5 patterns, read 2 files (ctrl+o to expand)

● Looking at the formatter, I see the issue. The is_base_of_v CRTP check is
  fragile for Eigen::Block because Block goes through several CRTP layers (Block
   → BlockImpl → internal::BlockImpl_dense → MatrixBase<Block> →
  DenseBase<Block>), and depending on the instantiation point and partial
  specialization matching order, this can fail to be picked up by fmt's
  specialization lookup. It also conflicts with fmt 11's built-in range
  formatter, since Eigen 3.4+ DenseBase exposes begin()/end().

  Here are three workable alternatives, ordered from least-to-most invasive:

  Option 1 — Delegate to a single concrete formatter via evaluation

  Avoid the SFINAE matching trap entirely by routing every Eigen expression
  through one fully-qualified formatter on a PlainObject:

  template <typename Derived>
  struct formatter<Derived, char,
                   std::enable_if_t<std::is_base_of_v<Eigen::EigenBase<Derived>,
   Derived>>> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename FormatContext> auto format(const Derived &m, 
  FormatContext &ctx) const {
      auto out = ctx.out();
      const Eigen::Index rows = m.rows();
      const Eigen::Index cols = m.cols();
      // Force evaluation through coeff() — works uniformly for Block, Map, 
  Transpose, etc.
      if (rows == 1) { /* row vector branch */ }
      else if (cols == 1) { /* column vector branch */ }
      else { /* matrix branch */ }
      // ... use m.coeff(i, j) instead of m(i, j)
    }
  };

  Two changes vs. the current code: switch the base class to Eigen::EigenBase
  (the one universal CRTP root in Eigen, simpler hierarchy) and use m.coeff(i, 
  j) instead of m(i, j). coeff() is defined uniformly on every dense expression
  including Block, while operator() has overloads that can complicate template
  matching.

  Option 2 — Trait-based detection instead of CRTP is_base_of

  Skip CRTP detection altogether and SFINAE on Eigen's own traits dispatcher,
  which is specialized for every Eigen expression including Block:

  template <typename T, typename = void>
  struct is_eigen_dense : std::false_type {};

  template <typename T>
  struct is_eigen_dense<T, std::void_t<
      typename Eigen::internal::traits<std::remove_cv_t<T>>::Scalar,
      decltype(std::declval<const T&>().rows()),
      decltype(std::declval<const T&>().cols()),
      decltype(std::declval<const T&>().coeff(0, 0))>> : std::true_type {};

  template <typename T>
  struct formatter<T, char, std::enable_if_t<is_eigen_dense<T>::value>> { /* ...
   */ };

  This sidesteps the CRTP base-of question entirely — if
  Eigen::internal::traits<T> defines Scalar and the type has coeff(), treat it
  as a dense expression. Robust against any future expression class Eigen adds.

  Option 3 — Delegate to Eigen's operator<< via ostream_formatter

  Eigen already has a stream insertion operator that handles every expression
  type correctly. Just route through it:

  template <typename Derived>
  struct formatter<Derived, char,
                   std::enable_if_t<std::is_base_of_v<Eigen::EigenBase<Derived>,
   Derived>>>
      : ostream_formatter {};

  (Requires #include <fmt/ostream.h>.) Loses your custom [ ... ] / ^T
  formatting, but is one line and inherently compatible with anything Eigen can
  stream — Block, Map, Transpose, Replicate, you name it.

  Recommendation

  Go with Option 1: switch base class from DenseBase to EigenBase and replace
  m(i, j) with m.coeff(i, j). That's the minimal change that keeps your existing
   output formatting and almost certainly fixes Block. If that still misbehaves
  on some exotic expression, fall back to Option 2.

✻ Churned for 2m 40s