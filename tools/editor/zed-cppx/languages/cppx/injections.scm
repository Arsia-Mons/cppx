; Inject C++ into every opaque chunk the grammar left for the host language.
; These nodes are produced anywhere CPPX falls through to "this is just C++".

((cpp_text) @injection.content
 (#set! injection.language "cpp")
 (#set! injection.combined))

((expr_text) @injection.content
 (#set! injection.language "cpp")
 (#set! injection.combined))
