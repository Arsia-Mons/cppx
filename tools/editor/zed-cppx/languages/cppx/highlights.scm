; JSX tag names — paint like types so they pop against C++ identifiers.
(jsx_opening_element name: (tag_name) @tag)
(jsx_closing_element name: (tag_name) @tag)
(jsx_self_closing_element name: (tag_name) @tag)

; Attribute names.
(jsx_attribute name: (attribute_name) @attribute)

; Punctuation: angle brackets and slashes that frame JSX tags.
(jsx_opening_element "<" @punctuation.bracket)
(jsx_opening_element ">" @punctuation.bracket)
(jsx_closing_element "</" @punctuation.bracket)
(jsx_closing_element ">" @punctuation.bracket)
(jsx_self_closing_element "<" @punctuation.bracket)
(jsx_self_closing_element "/>" @punctuation.bracket)

; `=` separating attribute from value.
(jsx_attribute "=" @operator)

; String attribute values.
(string) @string
(escape_sequence) @string.escape

; Brace expressions embedded in JSX — color the braces themselves so the
; transition between JSX context and embedded C++ is visible.
(jsx_expression "{" @punctuation.special)
(jsx_expression "}" @punctuation.special)
(nested_braces "{" @punctuation.bracket)
(nested_braces "}" @punctuation.bracket)
