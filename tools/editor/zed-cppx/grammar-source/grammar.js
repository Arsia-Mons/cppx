/**
 * @file Lightweight tree-sitter grammar for CPPX.
 *
 * CPPX is C++ with embedded JSX-like tags. This grammar identifies the JSX
 * structure (tags, attributes, brace expressions) and leaves the rest as
 * opaque `cpp_text` / `expr_text` nodes. A companion injection query layers
 * tree-sitter-cpp over those nodes so the editor still gets real C++
 * highlighting inside them.
 *
 * Disambiguating `<` (JSX tag) from `<` (less-than / template) is done
 * structurally: a JSX block must either be self-closing (`<Foo .../>`) or
 * have a matching `</Foo>`. Sequences like `vector<int>` therefore fall into
 * `cpp_text` automatically.
 */

module.exports = grammar({
  name: 'cppx',

  extras: $ => [/[ \t\r\n]+/],

  rules: {
    source_file: $ => repeat($._item),

    _item: $ => choice(
      $.jsx_element,
      $.jsx_self_closing_element,
      $.cpp_text,
    ),

    // Opaque run of C++ source. A bare `<` is treated as a potential JSX
    // tag start; the alternatives below greedily consume `<`-positions
    // that we know are C++:
    //   - `template <...>` (the one keyword where `<` follows whitespace)
    //   - `Ident<...>` template instantiations (`vector<int>`, `cast<T>`)
    //   - `<<`, `< 5`, etc. (`<` followed by non-tag-start)
    cpp_text: $ => token(prec(-1, /(template\s*<|\w<|<[^A-Za-z_\/]|[^<])+/)),

    jsx_element: $ => seq(
      $.jsx_opening_element,
      repeat($._child),
      $.jsx_closing_element,
    ),

    jsx_opening_element: $ => seq(
      '<',
      field('name', $.tag_name),
      repeat($.jsx_attribute),
      '>',
    ),

    jsx_closing_element: $ => seq(
      '</',
      field('name', $.tag_name),
      '>',
    ),

    jsx_self_closing_element: $ => seq(
      '<',
      field('name', $.tag_name),
      repeat($.jsx_attribute),
      '/>',
    ),

    _child: $ => choice(
      $.jsx_element,
      $.jsx_self_closing_element,
      $.jsx_expression,
      $.jsx_text,
    ),

    // Text content between tags. Stops at `<` (next tag/close) or `{`
    // (expression). No identifier-before-`<` carve-out here — inside JSX
    // children, `</Foo>` legitimately follows text like "Resume", and
    // C++ template syntax can't appear at this position.
    jsx_text: $ => token(prec(-1, /(<[^A-Za-z_\/]|[^<{}])+/)),

    jsx_expression: $ => seq(
      '{',
      repeat($._expr_item),
      '}',
    ),

    _expr_item: $ => choice(
      $.jsx_element,
      $.jsx_self_closing_element,
      $.nested_braces,
      $.expr_text,
    ),

    // Nested `{...}` inside an expression body — preserves brace balance
    // without trying to understand the C++ inside.
    nested_braces: $ => seq(
      '{',
      repeat($._expr_item),
      '}',
    ),

    // C++ expression content; stops at braces or JSX tag starts. Strings
    // are absorbed whole so braces/`<` inside them don't break out.
    expr_text: $ => token(prec(-1, /(template\s*<|\w<|<[^A-Za-z_\/{}]|"(\\.|[^"\\])*"|'(\\.|[^'\\])*'|[^<{}"'])+/)),

    tag_name: $ => /[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)*/,

    jsx_attribute: $ => seq(
      field('name', $.attribute_name),
      optional(seq('=', field('value', $._attribute_value))),
    ),

    attribute_name: $ => /[A-Za-z_][A-Za-z0-9_-]*/,

    _attribute_value: $ => choice(
      $.jsx_expression,
      $.string,
    ),

    string: $ => choice(
      seq('"', repeat(choice(/[^"\\\n]+/, $.escape_sequence)), '"'),
      seq("'", repeat(choice(/[^'\\\n]+/, $.escape_sequence)), "'"),
    ),

    escape_sequence: $ => /\\./,
  },
});
