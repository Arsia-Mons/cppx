#include "tree_sitter/parser.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define LANGUAGE_VERSION 14
#define STATE_COUNT 69
#define LARGE_STATE_COUNT 2
#define SYMBOL_COUNT 37
#define ALIAS_COUNT 0
#define TOKEN_COUNT 18
#define EXTERNAL_TOKEN_COUNT 0
#define FIELD_COUNT 2
#define MAX_ALIAS_SEQUENCE_LENGTH 4
#define PRODUCTION_ID_COUNT 4

enum ts_symbol_identifiers {
  sym_cpp_text = 1,
  anon_sym_LT = 2,
  anon_sym_GT = 3,
  anon_sym_LT_SLASH = 4,
  anon_sym_SLASH_GT = 5,
  sym_jsx_text = 6,
  anon_sym_LBRACE = 7,
  anon_sym_RBRACE = 8,
  sym_expr_text = 9,
  sym_tag_name = 10,
  anon_sym_EQ = 11,
  sym_attribute_name = 12,
  anon_sym_DQUOTE = 13,
  aux_sym_string_token1 = 14,
  anon_sym_SQUOTE = 15,
  aux_sym_string_token2 = 16,
  sym_escape_sequence = 17,
  sym_source_file = 18,
  sym__item = 19,
  sym_jsx_element = 20,
  sym_jsx_opening_element = 21,
  sym_jsx_closing_element = 22,
  sym_jsx_self_closing_element = 23,
  sym__child = 24,
  sym_jsx_expression = 25,
  sym__expr_item = 26,
  sym_nested_braces = 27,
  sym_jsx_attribute = 28,
  sym__attribute_value = 29,
  sym_string = 30,
  aux_sym_source_file_repeat1 = 31,
  aux_sym_jsx_element_repeat1 = 32,
  aux_sym_jsx_opening_element_repeat1 = 33,
  aux_sym_jsx_expression_repeat1 = 34,
  aux_sym_string_repeat1 = 35,
  aux_sym_string_repeat2 = 36,
};

static const char * const ts_symbol_names[] = {
  [ts_builtin_sym_end] = "end",
  [sym_cpp_text] = "cpp_text",
  [anon_sym_LT] = "<",
  [anon_sym_GT] = ">",
  [anon_sym_LT_SLASH] = "</",
  [anon_sym_SLASH_GT] = "/>",
  [sym_jsx_text] = "jsx_text",
  [anon_sym_LBRACE] = "{",
  [anon_sym_RBRACE] = "}",
  [sym_expr_text] = "expr_text",
  [sym_tag_name] = "tag_name",
  [anon_sym_EQ] = "=",
  [sym_attribute_name] = "attribute_name",
  [anon_sym_DQUOTE] = "\"",
  [aux_sym_string_token1] = "string_token1",
  [anon_sym_SQUOTE] = "'",
  [aux_sym_string_token2] = "string_token2",
  [sym_escape_sequence] = "escape_sequence",
  [sym_source_file] = "source_file",
  [sym__item] = "_item",
  [sym_jsx_element] = "jsx_element",
  [sym_jsx_opening_element] = "jsx_opening_element",
  [sym_jsx_closing_element] = "jsx_closing_element",
  [sym_jsx_self_closing_element] = "jsx_self_closing_element",
  [sym__child] = "_child",
  [sym_jsx_expression] = "jsx_expression",
  [sym__expr_item] = "_expr_item",
  [sym_nested_braces] = "nested_braces",
  [sym_jsx_attribute] = "jsx_attribute",
  [sym__attribute_value] = "_attribute_value",
  [sym_string] = "string",
  [aux_sym_source_file_repeat1] = "source_file_repeat1",
  [aux_sym_jsx_element_repeat1] = "jsx_element_repeat1",
  [aux_sym_jsx_opening_element_repeat1] = "jsx_opening_element_repeat1",
  [aux_sym_jsx_expression_repeat1] = "jsx_expression_repeat1",
  [aux_sym_string_repeat1] = "string_repeat1",
  [aux_sym_string_repeat2] = "string_repeat2",
};

static const TSSymbol ts_symbol_map[] = {
  [ts_builtin_sym_end] = ts_builtin_sym_end,
  [sym_cpp_text] = sym_cpp_text,
  [anon_sym_LT] = anon_sym_LT,
  [anon_sym_GT] = anon_sym_GT,
  [anon_sym_LT_SLASH] = anon_sym_LT_SLASH,
  [anon_sym_SLASH_GT] = anon_sym_SLASH_GT,
  [sym_jsx_text] = sym_jsx_text,
  [anon_sym_LBRACE] = anon_sym_LBRACE,
  [anon_sym_RBRACE] = anon_sym_RBRACE,
  [sym_expr_text] = sym_expr_text,
  [sym_tag_name] = sym_tag_name,
  [anon_sym_EQ] = anon_sym_EQ,
  [sym_attribute_name] = sym_attribute_name,
  [anon_sym_DQUOTE] = anon_sym_DQUOTE,
  [aux_sym_string_token1] = aux_sym_string_token1,
  [anon_sym_SQUOTE] = anon_sym_SQUOTE,
  [aux_sym_string_token2] = aux_sym_string_token2,
  [sym_escape_sequence] = sym_escape_sequence,
  [sym_source_file] = sym_source_file,
  [sym__item] = sym__item,
  [sym_jsx_element] = sym_jsx_element,
  [sym_jsx_opening_element] = sym_jsx_opening_element,
  [sym_jsx_closing_element] = sym_jsx_closing_element,
  [sym_jsx_self_closing_element] = sym_jsx_self_closing_element,
  [sym__child] = sym__child,
  [sym_jsx_expression] = sym_jsx_expression,
  [sym__expr_item] = sym__expr_item,
  [sym_nested_braces] = sym_nested_braces,
  [sym_jsx_attribute] = sym_jsx_attribute,
  [sym__attribute_value] = sym__attribute_value,
  [sym_string] = sym_string,
  [aux_sym_source_file_repeat1] = aux_sym_source_file_repeat1,
  [aux_sym_jsx_element_repeat1] = aux_sym_jsx_element_repeat1,
  [aux_sym_jsx_opening_element_repeat1] = aux_sym_jsx_opening_element_repeat1,
  [aux_sym_jsx_expression_repeat1] = aux_sym_jsx_expression_repeat1,
  [aux_sym_string_repeat1] = aux_sym_string_repeat1,
  [aux_sym_string_repeat2] = aux_sym_string_repeat2,
};

static const TSSymbolMetadata ts_symbol_metadata[] = {
  [ts_builtin_sym_end] = {
    .visible = false,
    .named = true,
  },
  [sym_cpp_text] = {
    .visible = true,
    .named = true,
  },
  [anon_sym_LT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_GT] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_LT_SLASH] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_SLASH_GT] = {
    .visible = true,
    .named = false,
  },
  [sym_jsx_text] = {
    .visible = true,
    .named = true,
  },
  [anon_sym_LBRACE] = {
    .visible = true,
    .named = false,
  },
  [anon_sym_RBRACE] = {
    .visible = true,
    .named = false,
  },
  [sym_expr_text] = {
    .visible = true,
    .named = true,
  },
  [sym_tag_name] = {
    .visible = true,
    .named = true,
  },
  [anon_sym_EQ] = {
    .visible = true,
    .named = false,
  },
  [sym_attribute_name] = {
    .visible = true,
    .named = true,
  },
  [anon_sym_DQUOTE] = {
    .visible = true,
    .named = false,
  },
  [aux_sym_string_token1] = {
    .visible = false,
    .named = false,
  },
  [anon_sym_SQUOTE] = {
    .visible = true,
    .named = false,
  },
  [aux_sym_string_token2] = {
    .visible = false,
    .named = false,
  },
  [sym_escape_sequence] = {
    .visible = true,
    .named = true,
  },
  [sym_source_file] = {
    .visible = true,
    .named = true,
  },
  [sym__item] = {
    .visible = false,
    .named = true,
  },
  [sym_jsx_element] = {
    .visible = true,
    .named = true,
  },
  [sym_jsx_opening_element] = {
    .visible = true,
    .named = true,
  },
  [sym_jsx_closing_element] = {
    .visible = true,
    .named = true,
  },
  [sym_jsx_self_closing_element] = {
    .visible = true,
    .named = true,
  },
  [sym__child] = {
    .visible = false,
    .named = true,
  },
  [sym_jsx_expression] = {
    .visible = true,
    .named = true,
  },
  [sym__expr_item] = {
    .visible = false,
    .named = true,
  },
  [sym_nested_braces] = {
    .visible = true,
    .named = true,
  },
  [sym_jsx_attribute] = {
    .visible = true,
    .named = true,
  },
  [sym__attribute_value] = {
    .visible = false,
    .named = true,
  },
  [sym_string] = {
    .visible = true,
    .named = true,
  },
  [aux_sym_source_file_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_jsx_element_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_jsx_opening_element_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_jsx_expression_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_string_repeat1] = {
    .visible = false,
    .named = false,
  },
  [aux_sym_string_repeat2] = {
    .visible = false,
    .named = false,
  },
};

enum ts_field_identifiers {
  field_name = 1,
  field_value = 2,
};

static const char * const ts_field_names[] = {
  [0] = NULL,
  [field_name] = "name",
  [field_value] = "value",
};

static const TSFieldMapSlice ts_field_map_slices[PRODUCTION_ID_COUNT] = {
  [1] = {.index = 0, .length = 1},
  [2] = {.index = 1, .length = 1},
  [3] = {.index = 2, .length = 2},
};

static const TSFieldMapEntry ts_field_map_entries[] = {
  [0] =
    {field_name, 1},
  [1] =
    {field_name, 0},
  [2] =
    {field_name, 0},
    {field_value, 2},
};

static const TSSymbol ts_alias_sequences[PRODUCTION_ID_COUNT][MAX_ALIAS_SEQUENCE_LENGTH] = {
  [0] = {0},
};

static const uint16_t ts_non_terminal_alias_map[] = {
  0,
};

static const TSStateId ts_primary_state_ids[STATE_COUNT] = {
  [0] = 0,
  [1] = 1,
  [2] = 2,
  [3] = 3,
  [4] = 3,
  [5] = 2,
  [6] = 2,
  [7] = 3,
  [8] = 8,
  [9] = 9,
  [10] = 10,
  [11] = 11,
  [12] = 10,
  [13] = 13,
  [14] = 11,
  [15] = 15,
  [16] = 16,
  [17] = 17,
  [18] = 18,
  [19] = 19,
  [20] = 20,
  [21] = 19,
  [22] = 20,
  [23] = 20,
  [24] = 19,
  [25] = 25,
  [26] = 26,
  [27] = 27,
  [28] = 28,
  [29] = 29,
  [30] = 30,
  [31] = 31,
  [32] = 32,
  [33] = 33,
  [34] = 34,
  [35] = 35,
  [36] = 36,
  [37] = 37,
  [38] = 38,
  [39] = 39,
  [40] = 40,
  [41] = 41,
  [42] = 42,
  [43] = 43,
  [44] = 43,
  [45] = 26,
  [46] = 38,
  [47] = 42,
  [48] = 41,
  [49] = 30,
  [50] = 33,
  [51] = 51,
  [52] = 52,
  [53] = 43,
  [54] = 54,
  [55] = 26,
  [56] = 42,
  [57] = 38,
  [58] = 41,
  [59] = 59,
  [60] = 60,
  [61] = 60,
  [62] = 62,
  [63] = 63,
  [64] = 60,
  [65] = 59,
  [66] = 62,
  [67] = 59,
  [68] = 62,
};

static bool ts_lex(TSLexer *lexer, TSStateId state) {
  START_LEXER();
  eof = lexer->eof(lexer);
  switch (state) {
    case 0:
      if (eof) ADVANCE(23);
      ADVANCE_MAP(
        '"', 100,
        '\'', 103,
        '/', 12,
        '<', 37,
        '=', 98,
        '>', 38,
        '\\', 18,
        '{', 43,
        '}', 44,
      );
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') SKIP(0);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(96);
      END_STATE();
    case 1:
      if (lookahead == '\n') SKIP(1);
      if (lookahead == '"') ADVANCE(100);
      if (lookahead == '\\') ADVANCE(18);
      if (lookahead == '\t' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(101);
      if (lookahead != 0) ADVANCE(102);
      END_STATE();
    case 2:
      if (lookahead == '\n') SKIP(2);
      if (lookahead == '\'') ADVANCE(103);
      if (lookahead == '\\') ADVANCE(18);
      if (lookahead == '\t' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(104);
      if (lookahead != 0) ADVANCE(105);
      END_STATE();
    case 3:
      ADVANCE_MAP(
        '"', 5,
        '\'', 9,
        '<', 36,
        't', 51,
        '{', 43,
        '}', 44,
        '\t', 48,
        '\n', 48,
        '\r', 48,
        ' ', 48,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0) ADVANCE(49);
      END_STATE();
    case 4:
      if (lookahead == '"') ADVANCE(49);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == '/' ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= '{') ||
          lookahead == '}') ADVANCE(5);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 5:
      if (lookahead == '"') ADVANCE(49);
      if (lookahead == '\\') ADVANCE(19);
      if (lookahead != 0) ADVANCE(5);
      END_STATE();
    case 6:
      if (lookahead == '"') ADVANCE(73);
      if (lookahead == '\'') ADVANCE(60);
      if (lookahead == '\\') ADVANCE(47);
      if (lookahead == '/' ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= '{') ||
          lookahead == '}') ADVANCE(7);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 7:
      if (lookahead == '"') ADVANCE(73);
      if (lookahead == '\'') ADVANCE(60);
      if (lookahead == '\\') ADVANCE(21);
      if (lookahead != 0) ADVANCE(7);
      END_STATE();
    case 8:
      if (lookahead == '\'') ADVANCE(49);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == '/' ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= '{') ||
          lookahead == '}') ADVANCE(9);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 9:
      if (lookahead == '\'') ADVANCE(49);
      if (lookahead == '\\') ADVANCE(20);
      if (lookahead != 0) ADVANCE(9);
      END_STATE();
    case 10:
      if (lookahead == '/') ADVANCE(12);
      if (lookahead == '=') ADVANCE(98);
      if (lookahead == '>') ADVANCE(38);
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') SKIP(10);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(99);
      END_STATE();
    case 11:
      if (lookahead == '<') ADVANCE(37);
      if (lookahead == '{') ADVANCE(43);
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(41);
      if (lookahead != 0 &&
          lookahead != '}') ADVANCE(42);
      END_STATE();
    case 12:
      if (lookahead == '>') ADVANCE(40);
      END_STATE();
    case 13:
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') SKIP(13);
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(97);
      END_STATE();
    case 14:
      if (('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(97);
      END_STATE();
    case 15:
      if (lookahead != 0 &&
          lookahead != '/' &&
          (lookahead < 'A' || 'Z' < lookahead) &&
          lookahead != '_' &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 16:
      if (lookahead != 0 &&
          lookahead != '/' &&
          (lookahead < 'A' || 'Z' < lookahead) &&
          lookahead != '_' &&
          (lookahead < 'a' || 'z' < lookahead)) ADVANCE(42);
      END_STATE();
    case 17:
      if (lookahead != 0 &&
          lookahead != '/' &&
          (lookahead < 'A' || 'Z' < lookahead) &&
          lookahead != '_' &&
          (lookahead < 'a' || 'z' < lookahead)) ADVANCE(35);
      END_STATE();
    case 18:
      if (lookahead != 0 &&
          lookahead != '\n') ADVANCE(106);
      END_STATE();
    case 19:
      if (lookahead != 0 &&
          lookahead != '\n') ADVANCE(5);
      END_STATE();
    case 20:
      if (lookahead != 0 &&
          lookahead != '\n') ADVANCE(9);
      END_STATE();
    case 21:
      if (lookahead != 0 &&
          lookahead != '\n') ADVANCE(7);
      END_STATE();
    case 22:
      if (eof) ADVANCE(23);
      if (lookahead == '<') ADVANCE(36);
      if (lookahead == 't') ADVANCE(26);
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(24);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 23:
      ACCEPT_TOKEN(ts_builtin_sym_end);
      END_STATE();
    case 24:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(36);
      if (lookahead == 't') ADVANCE(26);
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(24);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 25:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'a') ADVANCE(34);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('b' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 26:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'e') ADVANCE(30);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 27:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'e') ADVANCE(29);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 28:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'l') ADVANCE(25);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 29:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'm') ADVANCE(31);
      if (lookahead == 't') ADVANCE(26);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(32);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 30:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'm') ADVANCE(31);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 31:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 'p') ADVANCE(28);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 32:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 't') ADVANCE(26);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(32);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 33:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 34:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(33);
      if (lookahead == 't') ADVANCE(27);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 35:
      ACCEPT_TOKEN(sym_cpp_text);
      if (lookahead == '<') ADVANCE(17);
      if (lookahead == 't') ADVANCE(26);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(33);
      if (lookahead != 0) ADVANCE(35);
      END_STATE();
    case 36:
      ACCEPT_TOKEN(anon_sym_LT);
      END_STATE();
    case 37:
      ACCEPT_TOKEN(anon_sym_LT);
      if (lookahead == '/') ADVANCE(39);
      END_STATE();
    case 38:
      ACCEPT_TOKEN(anon_sym_GT);
      END_STATE();
    case 39:
      ACCEPT_TOKEN(anon_sym_LT_SLASH);
      END_STATE();
    case 40:
      ACCEPT_TOKEN(anon_sym_SLASH_GT);
      END_STATE();
    case 41:
      ACCEPT_TOKEN(sym_jsx_text);
      if (lookahead == '<') ADVANCE(37);
      if (lookahead == '{') ADVANCE(43);
      if (lookahead == '\t' ||
          lookahead == '\n' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(41);
      if (lookahead != 0 &&
          lookahead != '}') ADVANCE(42);
      END_STATE();
    case 42:
      ACCEPT_TOKEN(sym_jsx_text);
      if (lookahead == '<') ADVANCE(16);
      if (lookahead != 0 &&
          lookahead != '{' &&
          lookahead != '}') ADVANCE(42);
      END_STATE();
    case 43:
      ACCEPT_TOKEN(anon_sym_LBRACE);
      END_STATE();
    case 44:
      ACCEPT_TOKEN(anon_sym_RBRACE);
      END_STATE();
    case 45:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '\n') ADVANCE(49);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(4);
      if (lookahead == 't') ADVANCE(62);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 46:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '\n') ADVANCE(49);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(8);
      if (lookahead == 't') ADVANCE(75);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 47:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '\n') ADVANCE(49);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(6);
      if (lookahead == 't') ADVANCE(87);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(7);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 48:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 5,
        '\'', 9,
        '<', 36,
        't', 51,
        '{', 43,
        '}', 44,
        '\t', 48,
        '\n', 48,
        '\r', 48,
        ' ', 48,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0) ADVANCE(49);
      END_STATE();
    case 49:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(15);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 50:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'a') ADVANCE(59);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('b' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 51:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'e') ADVANCE(55);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 52:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'e') ADVANCE(54);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 53:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'l') ADVANCE(50);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 54:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'm') ADVANCE(56);
      if (lookahead == 't') ADVANCE(51);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(57);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 55:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'm') ADVANCE(56);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 56:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 'p') ADVANCE(53);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 57:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 't') ADVANCE(51);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(57);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 58:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 59:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(5);
      if (lookahead == '\'') ADVANCE(9);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 't') ADVANCE(52);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 60:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(4);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == 't') ADVANCE(62);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 61:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'a', 70,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('b' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 62:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'e', 66,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 63:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'e', 65,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 64:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'l', 61,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 65:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'm', 67,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(68);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 66:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'm', 67,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 67:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 60,
        '\'', 7,
        '<', 72,
        '\\', 45,
        'p', 64,
        't', 62,
        '{', 5,
        '}', 5,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 68:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(72);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == 't') ADVANCE(62);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(68);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 69:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(72);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == 't') ADVANCE(62);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 70:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(7);
      if (lookahead == '<') ADVANCE(72);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == 't') ADVANCE(63);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 71:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(71);
      if (lookahead == 't') ADVANCE(51);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(58);
      if (lookahead != 0 &&
          (lookahead < 'a' || '{' < lookahead) &&
          lookahead != '}') ADVANCE(49);
      END_STATE();
    case 72:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(60);
      if (lookahead == '\'') ADVANCE(85);
      if (lookahead == '<') ADVANCE(72);
      if (lookahead == '\\') ADVANCE(45);
      if (lookahead == 't') ADVANCE(62);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(5);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(69);
      if (lookahead != 0) ADVANCE(60);
      END_STATE();
    case 73:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(8);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == 't') ADVANCE(75);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 74:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'a', 83,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('b' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 75:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'e', 79,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 76:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'e', 78,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 77:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'l', 74,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 78:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'm', 80,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(81);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 79:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'm', 80,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 80:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 7,
        '\'', 73,
        '<', 84,
        '\\', 46,
        'p', 77,
        't', 75,
        '{', 9,
        '}', 9,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 81:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(84);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == 't') ADVANCE(75);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(81);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 82:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(84);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == 't') ADVANCE(75);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 83:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(7);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(84);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == 't') ADVANCE(76);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 84:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(85);
      if (lookahead == '\'') ADVANCE(73);
      if (lookahead == '<') ADVANCE(84);
      if (lookahead == '\\') ADVANCE(46);
      if (lookahead == 't') ADVANCE(75);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(9);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(82);
      if (lookahead != 0) ADVANCE(73);
      END_STATE();
    case 85:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(85);
      if (lookahead == '\'') ADVANCE(85);
      if (lookahead == '<') ADVANCE(6);
      if (lookahead == '\\') ADVANCE(47);
      if (lookahead == 't') ADVANCE(87);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(7);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 86:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'a', 95,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('b' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 87:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'e', 91,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 88:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'e', 90,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 89:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'l', 86,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 90:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'm', 92,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(93);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 91:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'm', 92,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 92:
      ACCEPT_TOKEN(sym_expr_text);
      ADVANCE_MAP(
        '"', 85,
        '\'', 85,
        '<', 94,
        '\\', 47,
        'p', 89,
        't', 87,
        '{', 7,
        '}', 7,
      );
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 93:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(85);
      if (lookahead == '\'') ADVANCE(85);
      if (lookahead == '<') ADVANCE(94);
      if (lookahead == '\\') ADVANCE(47);
      if (lookahead == 't') ADVANCE(87);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(7);
      if (('\t' <= lookahead && lookahead <= '\r') ||
          lookahead == ' ') ADVANCE(93);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 94:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(85);
      if (lookahead == '\'') ADVANCE(85);
      if (lookahead == '<') ADVANCE(94);
      if (lookahead == '\\') ADVANCE(47);
      if (lookahead == 't') ADVANCE(87);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(7);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 95:
      ACCEPT_TOKEN(sym_expr_text);
      if (lookahead == '"') ADVANCE(85);
      if (lookahead == '\'') ADVANCE(85);
      if (lookahead == '<') ADVANCE(94);
      if (lookahead == '\\') ADVANCE(47);
      if (lookahead == 't') ADVANCE(88);
      if (lookahead == '{' ||
          lookahead == '}') ADVANCE(7);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(94);
      if (lookahead != 0) ADVANCE(85);
      END_STATE();
    case 96:
      ACCEPT_TOKEN(sym_tag_name);
      if (lookahead == '-') ADVANCE(99);
      if (lookahead == '.') ADVANCE(14);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(96);
      END_STATE();
    case 97:
      ACCEPT_TOKEN(sym_tag_name);
      if (lookahead == '.') ADVANCE(14);
      if (('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(97);
      END_STATE();
    case 98:
      ACCEPT_TOKEN(anon_sym_EQ);
      END_STATE();
    case 99:
      ACCEPT_TOKEN(sym_attribute_name);
      if (lookahead == '-' ||
          ('0' <= lookahead && lookahead <= '9') ||
          ('A' <= lookahead && lookahead <= 'Z') ||
          lookahead == '_' ||
          ('a' <= lookahead && lookahead <= 'z')) ADVANCE(99);
      END_STATE();
    case 100:
      ACCEPT_TOKEN(anon_sym_DQUOTE);
      END_STATE();
    case 101:
      ACCEPT_TOKEN(aux_sym_string_token1);
      if (lookahead == '\t' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(101);
      if (lookahead != 0 &&
          lookahead != '\t' &&
          lookahead != '\n' &&
          lookahead != '"' &&
          lookahead != '\\') ADVANCE(102);
      END_STATE();
    case 102:
      ACCEPT_TOKEN(aux_sym_string_token1);
      if (lookahead != 0 &&
          lookahead != '\n' &&
          lookahead != '"' &&
          lookahead != '\\') ADVANCE(102);
      END_STATE();
    case 103:
      ACCEPT_TOKEN(anon_sym_SQUOTE);
      END_STATE();
    case 104:
      ACCEPT_TOKEN(aux_sym_string_token2);
      if (lookahead == '\t' ||
          lookahead == '\r' ||
          lookahead == ' ') ADVANCE(104);
      if (lookahead != 0 &&
          lookahead != '\t' &&
          lookahead != '\n' &&
          lookahead != '\'' &&
          lookahead != '\\') ADVANCE(105);
      END_STATE();
    case 105:
      ACCEPT_TOKEN(aux_sym_string_token2);
      if (lookahead != 0 &&
          lookahead != '\n' &&
          lookahead != '\'' &&
          lookahead != '\\') ADVANCE(105);
      END_STATE();
    case 106:
      ACCEPT_TOKEN(sym_escape_sequence);
      END_STATE();
    default:
      return false;
  }
}

static const TSLexMode ts_lex_modes[STATE_COUNT] = {
  [0] = {.lex_state = 0},
  [1] = {.lex_state = 22},
  [2] = {.lex_state = 11},
  [3] = {.lex_state = 11},
  [4] = {.lex_state = 11},
  [5] = {.lex_state = 11},
  [6] = {.lex_state = 11},
  [7] = {.lex_state = 11},
  [8] = {.lex_state = 3},
  [9] = {.lex_state = 3},
  [10] = {.lex_state = 3},
  [11] = {.lex_state = 3},
  [12] = {.lex_state = 3},
  [13] = {.lex_state = 3},
  [14] = {.lex_state = 3},
  [15] = {.lex_state = 11},
  [16] = {.lex_state = 22},
  [17] = {.lex_state = 22},
  [18] = {.lex_state = 0},
  [19] = {.lex_state = 10},
  [20] = {.lex_state = 10},
  [21] = {.lex_state = 10},
  [22] = {.lex_state = 10},
  [23] = {.lex_state = 10},
  [24] = {.lex_state = 10},
  [25] = {.lex_state = 10},
  [26] = {.lex_state = 3},
  [27] = {.lex_state = 10},
  [28] = {.lex_state = 11},
  [29] = {.lex_state = 3},
  [30] = {.lex_state = 11},
  [31] = {.lex_state = 1},
  [32] = {.lex_state = 2},
  [33] = {.lex_state = 11},
  [34] = {.lex_state = 3},
  [35] = {.lex_state = 11},
  [36] = {.lex_state = 1},
  [37] = {.lex_state = 2},
  [38] = {.lex_state = 3},
  [39] = {.lex_state = 1},
  [40] = {.lex_state = 2},
  [41] = {.lex_state = 11},
  [42] = {.lex_state = 11},
  [43] = {.lex_state = 3},
  [44] = {.lex_state = 11},
  [45] = {.lex_state = 11},
  [46] = {.lex_state = 11},
  [47] = {.lex_state = 3},
  [48] = {.lex_state = 3},
  [49] = {.lex_state = 10},
  [50] = {.lex_state = 10},
  [51] = {.lex_state = 10},
  [52] = {.lex_state = 10},
  [53] = {.lex_state = 22},
  [54] = {.lex_state = 10},
  [55] = {.lex_state = 22},
  [56] = {.lex_state = 22},
  [57] = {.lex_state = 22},
  [58] = {.lex_state = 22},
  [59] = {.lex_state = 13},
  [60] = {.lex_state = 0},
  [61] = {.lex_state = 0},
  [62] = {.lex_state = 13},
  [63] = {.lex_state = 0},
  [64] = {.lex_state = 0},
  [65] = {.lex_state = 13},
  [66] = {.lex_state = 13},
  [67] = {.lex_state = 13},
  [68] = {.lex_state = 13},
};

static const uint16_t ts_parse_table[LARGE_STATE_COUNT][SYMBOL_COUNT] = {
  [0] = {
    [ts_builtin_sym_end] = ACTIONS(1),
    [anon_sym_LT] = ACTIONS(1),
    [anon_sym_GT] = ACTIONS(1),
    [anon_sym_LT_SLASH] = ACTIONS(1),
    [anon_sym_SLASH_GT] = ACTIONS(1),
    [anon_sym_LBRACE] = ACTIONS(1),
    [anon_sym_RBRACE] = ACTIONS(1),
    [sym_tag_name] = ACTIONS(1),
    [anon_sym_EQ] = ACTIONS(1),
    [sym_attribute_name] = ACTIONS(1),
    [anon_sym_DQUOTE] = ACTIONS(1),
    [anon_sym_SQUOTE] = ACTIONS(1),
    [sym_escape_sequence] = ACTIONS(1),
  },
  [1] = {
    [sym_source_file] = STATE(63),
    [sym__item] = STATE(16),
    [sym_jsx_element] = STATE(16),
    [sym_jsx_opening_element] = STATE(3),
    [sym_jsx_self_closing_element] = STATE(16),
    [aux_sym_source_file_repeat1] = STATE(16),
    [ts_builtin_sym_end] = ACTIONS(3),
    [sym_cpp_text] = ACTIONS(5),
    [anon_sym_LT] = ACTIONS(7),
  },
};

static const uint16_t ts_small_parse_table[] = {
  [0] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(11), 1,
      anon_sym_LT_SLASH,
    ACTIONS(13), 1,
      sym_jsx_text,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(43), 1,
      sym_jsx_closing_element,
    STATE(15), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [26] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    ACTIONS(17), 1,
      anon_sym_LT_SLASH,
    ACTIONS(19), 1,
      sym_jsx_text,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(58), 1,
      sym_jsx_closing_element,
    STATE(5), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [52] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(11), 1,
      anon_sym_LT_SLASH,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    ACTIONS(21), 1,
      sym_jsx_text,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(48), 1,
      sym_jsx_closing_element,
    STATE(2), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [78] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(13), 1,
      sym_jsx_text,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    ACTIONS(17), 1,
      anon_sym_LT_SLASH,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(53), 1,
      sym_jsx_closing_element,
    STATE(15), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [104] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(13), 1,
      sym_jsx_text,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    ACTIONS(23), 1,
      anon_sym_LT_SLASH,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(44), 1,
      sym_jsx_closing_element,
    STATE(15), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [130] = 7,
    ACTIONS(9), 1,
      anon_sym_LT,
    ACTIONS(15), 1,
      anon_sym_LBRACE,
    ACTIONS(23), 1,
      anon_sym_LT_SLASH,
    ACTIONS(25), 1,
      sym_jsx_text,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(41), 1,
      sym_jsx_closing_element,
    STATE(6), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [156] = 6,
    ACTIONS(27), 1,
      anon_sym_LT,
    ACTIONS(30), 1,
      anon_sym_LBRACE,
    ACTIONS(33), 1,
      anon_sym_RBRACE,
    ACTIONS(35), 1,
      sym_expr_text,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(8), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [179] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(42), 1,
      anon_sym_RBRACE,
    ACTIONS(44), 1,
      sym_expr_text,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(8), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [202] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(46), 1,
      anon_sym_RBRACE,
    ACTIONS(48), 1,
      sym_expr_text,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(14), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [225] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(44), 1,
      sym_expr_text,
    ACTIONS(50), 1,
      anon_sym_RBRACE,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(8), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [248] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(52), 1,
      anon_sym_RBRACE,
    ACTIONS(54), 1,
      sym_expr_text,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(11), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [271] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(56), 1,
      anon_sym_RBRACE,
    ACTIONS(58), 1,
      sym_expr_text,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(9), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [294] = 6,
    ACTIONS(38), 1,
      anon_sym_LT,
    ACTIONS(40), 1,
      anon_sym_LBRACE,
    ACTIONS(44), 1,
      sym_expr_text,
    ACTIONS(60), 1,
      anon_sym_RBRACE,
    STATE(4), 1,
      sym_jsx_opening_element,
    STATE(8), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__expr_item,
      sym_nested_braces,
      aux_sym_jsx_expression_repeat1,
  [317] = 6,
    ACTIONS(62), 1,
      anon_sym_LT,
    ACTIONS(65), 1,
      anon_sym_LT_SLASH,
    ACTIONS(67), 1,
      sym_jsx_text,
    ACTIONS(70), 1,
      anon_sym_LBRACE,
    STATE(7), 1,
      sym_jsx_opening_element,
    STATE(15), 5,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      sym__child,
      sym_jsx_expression,
      aux_sym_jsx_element_repeat1,
  [340] = 5,
    ACTIONS(7), 1,
      anon_sym_LT,
    ACTIONS(73), 1,
      ts_builtin_sym_end,
    ACTIONS(75), 1,
      sym_cpp_text,
    STATE(3), 1,
      sym_jsx_opening_element,
    STATE(17), 4,
      sym__item,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      aux_sym_source_file_repeat1,
  [359] = 5,
    ACTIONS(77), 1,
      ts_builtin_sym_end,
    ACTIONS(79), 1,
      sym_cpp_text,
    ACTIONS(82), 1,
      anon_sym_LT,
    STATE(3), 1,
      sym_jsx_opening_element,
    STATE(17), 4,
      sym__item,
      sym_jsx_element,
      sym_jsx_self_closing_element,
      aux_sym_source_file_repeat1,
  [378] = 4,
    ACTIONS(85), 1,
      anon_sym_LBRACE,
    ACTIONS(87), 1,
      anon_sym_DQUOTE,
    ACTIONS(89), 1,
      anon_sym_SQUOTE,
    STATE(54), 3,
      sym_jsx_expression,
      sym__attribute_value,
      sym_string,
  [393] = 4,
    ACTIONS(91), 1,
      anon_sym_GT,
    ACTIONS(93), 1,
      anon_sym_SLASH_GT,
    ACTIONS(95), 1,
      sym_attribute_name,
    STATE(20), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [407] = 4,
    ACTIONS(95), 1,
      sym_attribute_name,
    ACTIONS(97), 1,
      anon_sym_GT,
    ACTIONS(99), 1,
      anon_sym_SLASH_GT,
    STATE(25), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [421] = 4,
    ACTIONS(91), 1,
      anon_sym_GT,
    ACTIONS(95), 1,
      sym_attribute_name,
    ACTIONS(101), 1,
      anon_sym_SLASH_GT,
    STATE(22), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [435] = 4,
    ACTIONS(95), 1,
      sym_attribute_name,
    ACTIONS(97), 1,
      anon_sym_GT,
    ACTIONS(103), 1,
      anon_sym_SLASH_GT,
    STATE(25), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [449] = 4,
    ACTIONS(95), 1,
      sym_attribute_name,
    ACTIONS(97), 1,
      anon_sym_GT,
    ACTIONS(105), 1,
      anon_sym_SLASH_GT,
    STATE(25), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [463] = 4,
    ACTIONS(91), 1,
      anon_sym_GT,
    ACTIONS(95), 1,
      sym_attribute_name,
    ACTIONS(107), 1,
      anon_sym_SLASH_GT,
    STATE(23), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [477] = 3,
    ACTIONS(111), 1,
      sym_attribute_name,
    ACTIONS(109), 2,
      anon_sym_GT,
      anon_sym_SLASH_GT,
    STATE(25), 2,
      sym_jsx_attribute,
      aux_sym_jsx_opening_element_repeat1,
  [489] = 1,
    ACTIONS(114), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [496] = 2,
    ACTIONS(118), 1,
      anon_sym_EQ,
    ACTIONS(116), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [505] = 1,
    ACTIONS(120), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [512] = 1,
    ACTIONS(122), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [519] = 1,
    ACTIONS(124), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [526] = 4,
    ACTIONS(126), 1,
      anon_sym_DQUOTE,
    ACTIONS(128), 1,
      aux_sym_string_token1,
    ACTIONS(130), 1,
      sym_escape_sequence,
    STATE(36), 1,
      aux_sym_string_repeat1,
  [539] = 4,
    ACTIONS(126), 1,
      anon_sym_SQUOTE,
    ACTIONS(132), 1,
      aux_sym_string_token2,
    ACTIONS(134), 1,
      sym_escape_sequence,
    STATE(37), 1,
      aux_sym_string_repeat2,
  [552] = 1,
    ACTIONS(136), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [559] = 1,
    ACTIONS(138), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [566] = 1,
    ACTIONS(140), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [573] = 4,
    ACTIONS(142), 1,
      anon_sym_DQUOTE,
    ACTIONS(144), 1,
      aux_sym_string_token1,
    ACTIONS(146), 1,
      sym_escape_sequence,
    STATE(39), 1,
      aux_sym_string_repeat1,
  [586] = 4,
    ACTIONS(142), 1,
      anon_sym_SQUOTE,
    ACTIONS(148), 1,
      aux_sym_string_token2,
    ACTIONS(150), 1,
      sym_escape_sequence,
    STATE(40), 1,
      aux_sym_string_repeat2,
  [599] = 1,
    ACTIONS(152), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [606] = 4,
    ACTIONS(154), 1,
      anon_sym_DQUOTE,
    ACTIONS(156), 1,
      aux_sym_string_token1,
    ACTIONS(159), 1,
      sym_escape_sequence,
    STATE(39), 1,
      aux_sym_string_repeat1,
  [619] = 4,
    ACTIONS(162), 1,
      anon_sym_SQUOTE,
    ACTIONS(164), 1,
      aux_sym_string_token2,
    ACTIONS(167), 1,
      sym_escape_sequence,
    STATE(40), 1,
      aux_sym_string_repeat2,
  [632] = 1,
    ACTIONS(170), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [639] = 1,
    ACTIONS(172), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [646] = 1,
    ACTIONS(174), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [653] = 1,
    ACTIONS(174), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [660] = 1,
    ACTIONS(114), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [667] = 1,
    ACTIONS(152), 4,
      anon_sym_LT,
      anon_sym_LT_SLASH,
      sym_jsx_text,
      anon_sym_LBRACE,
  [674] = 1,
    ACTIONS(172), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [681] = 1,
    ACTIONS(170), 4,
      anon_sym_LT,
      anon_sym_LBRACE,
      anon_sym_RBRACE,
      sym_expr_text,
  [688] = 1,
    ACTIONS(176), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [694] = 1,
    ACTIONS(178), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [700] = 1,
    ACTIONS(180), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [706] = 1,
    ACTIONS(182), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [712] = 2,
    ACTIONS(184), 1,
      ts_builtin_sym_end,
    ACTIONS(174), 2,
      sym_cpp_text,
      anon_sym_LT,
  [720] = 1,
    ACTIONS(186), 3,
      anon_sym_GT,
      anon_sym_SLASH_GT,
      sym_attribute_name,
  [726] = 2,
    ACTIONS(188), 1,
      ts_builtin_sym_end,
    ACTIONS(114), 2,
      sym_cpp_text,
      anon_sym_LT,
  [734] = 2,
    ACTIONS(190), 1,
      ts_builtin_sym_end,
    ACTIONS(172), 2,
      sym_cpp_text,
      anon_sym_LT,
  [742] = 2,
    ACTIONS(192), 1,
      ts_builtin_sym_end,
    ACTIONS(152), 2,
      sym_cpp_text,
      anon_sym_LT,
  [750] = 2,
    ACTIONS(194), 1,
      ts_builtin_sym_end,
    ACTIONS(170), 2,
      sym_cpp_text,
      anon_sym_LT,
  [758] = 1,
    ACTIONS(196), 1,
      sym_tag_name,
  [762] = 1,
    ACTIONS(198), 1,
      anon_sym_GT,
  [766] = 1,
    ACTIONS(200), 1,
      anon_sym_GT,
  [770] = 1,
    ACTIONS(202), 1,
      sym_tag_name,
  [774] = 1,
    ACTIONS(204), 1,
      ts_builtin_sym_end,
  [778] = 1,
    ACTIONS(206), 1,
      anon_sym_GT,
  [782] = 1,
    ACTIONS(208), 1,
      sym_tag_name,
  [786] = 1,
    ACTIONS(210), 1,
      sym_tag_name,
  [790] = 1,
    ACTIONS(212), 1,
      sym_tag_name,
  [794] = 1,
    ACTIONS(214), 1,
      sym_tag_name,
};

static const uint32_t ts_small_parse_table_map[] = {
  [SMALL_STATE(2)] = 0,
  [SMALL_STATE(3)] = 26,
  [SMALL_STATE(4)] = 52,
  [SMALL_STATE(5)] = 78,
  [SMALL_STATE(6)] = 104,
  [SMALL_STATE(7)] = 130,
  [SMALL_STATE(8)] = 156,
  [SMALL_STATE(9)] = 179,
  [SMALL_STATE(10)] = 202,
  [SMALL_STATE(11)] = 225,
  [SMALL_STATE(12)] = 248,
  [SMALL_STATE(13)] = 271,
  [SMALL_STATE(14)] = 294,
  [SMALL_STATE(15)] = 317,
  [SMALL_STATE(16)] = 340,
  [SMALL_STATE(17)] = 359,
  [SMALL_STATE(18)] = 378,
  [SMALL_STATE(19)] = 393,
  [SMALL_STATE(20)] = 407,
  [SMALL_STATE(21)] = 421,
  [SMALL_STATE(22)] = 435,
  [SMALL_STATE(23)] = 449,
  [SMALL_STATE(24)] = 463,
  [SMALL_STATE(25)] = 477,
  [SMALL_STATE(26)] = 489,
  [SMALL_STATE(27)] = 496,
  [SMALL_STATE(28)] = 505,
  [SMALL_STATE(29)] = 512,
  [SMALL_STATE(30)] = 519,
  [SMALL_STATE(31)] = 526,
  [SMALL_STATE(32)] = 539,
  [SMALL_STATE(33)] = 552,
  [SMALL_STATE(34)] = 559,
  [SMALL_STATE(35)] = 566,
  [SMALL_STATE(36)] = 573,
  [SMALL_STATE(37)] = 586,
  [SMALL_STATE(38)] = 599,
  [SMALL_STATE(39)] = 606,
  [SMALL_STATE(40)] = 619,
  [SMALL_STATE(41)] = 632,
  [SMALL_STATE(42)] = 639,
  [SMALL_STATE(43)] = 646,
  [SMALL_STATE(44)] = 653,
  [SMALL_STATE(45)] = 660,
  [SMALL_STATE(46)] = 667,
  [SMALL_STATE(47)] = 674,
  [SMALL_STATE(48)] = 681,
  [SMALL_STATE(49)] = 688,
  [SMALL_STATE(50)] = 694,
  [SMALL_STATE(51)] = 700,
  [SMALL_STATE(52)] = 706,
  [SMALL_STATE(53)] = 712,
  [SMALL_STATE(54)] = 720,
  [SMALL_STATE(55)] = 726,
  [SMALL_STATE(56)] = 734,
  [SMALL_STATE(57)] = 742,
  [SMALL_STATE(58)] = 750,
  [SMALL_STATE(59)] = 758,
  [SMALL_STATE(60)] = 762,
  [SMALL_STATE(61)] = 766,
  [SMALL_STATE(62)] = 770,
  [SMALL_STATE(63)] = 774,
  [SMALL_STATE(64)] = 778,
  [SMALL_STATE(65)] = 782,
  [SMALL_STATE(66)] = 786,
  [SMALL_STATE(67)] = 790,
  [SMALL_STATE(68)] = 794,
};

static const TSParseActionEntry ts_parse_actions[] = {
  [0] = {.entry = {.count = 0, .reusable = false}},
  [1] = {.entry = {.count = 1, .reusable = false}}, RECOVER(),
  [3] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_source_file, 0, 0, 0),
  [5] = {.entry = {.count = 1, .reusable = false}}, SHIFT(16),
  [7] = {.entry = {.count = 1, .reusable = false}}, SHIFT(59),
  [9] = {.entry = {.count = 1, .reusable = false}}, SHIFT(65),
  [11] = {.entry = {.count = 1, .reusable = false}}, SHIFT(68),
  [13] = {.entry = {.count = 1, .reusable = false}}, SHIFT(15),
  [15] = {.entry = {.count = 1, .reusable = false}}, SHIFT(10),
  [17] = {.entry = {.count = 1, .reusable = false}}, SHIFT(62),
  [19] = {.entry = {.count = 1, .reusable = false}}, SHIFT(5),
  [21] = {.entry = {.count = 1, .reusable = false}}, SHIFT(2),
  [23] = {.entry = {.count = 1, .reusable = false}}, SHIFT(66),
  [25] = {.entry = {.count = 1, .reusable = false}}, SHIFT(6),
  [27] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_expression_repeat1, 2, 0, 0), SHIFT_REPEAT(67),
  [30] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_expression_repeat1, 2, 0, 0), SHIFT_REPEAT(13),
  [33] = {.entry = {.count = 1, .reusable = false}}, REDUCE(aux_sym_jsx_expression_repeat1, 2, 0, 0),
  [35] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_expression_repeat1, 2, 0, 0), SHIFT_REPEAT(8),
  [38] = {.entry = {.count = 1, .reusable = false}}, SHIFT(67),
  [40] = {.entry = {.count = 1, .reusable = false}}, SHIFT(13),
  [42] = {.entry = {.count = 1, .reusable = false}}, SHIFT(34),
  [44] = {.entry = {.count = 1, .reusable = false}}, SHIFT(8),
  [46] = {.entry = {.count = 1, .reusable = false}}, SHIFT(33),
  [48] = {.entry = {.count = 1, .reusable = false}}, SHIFT(14),
  [50] = {.entry = {.count = 1, .reusable = false}}, SHIFT(49),
  [52] = {.entry = {.count = 1, .reusable = false}}, SHIFT(50),
  [54] = {.entry = {.count = 1, .reusable = false}}, SHIFT(11),
  [56] = {.entry = {.count = 1, .reusable = false}}, SHIFT(29),
  [58] = {.entry = {.count = 1, .reusable = false}}, SHIFT(9),
  [60] = {.entry = {.count = 1, .reusable = false}}, SHIFT(30),
  [62] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_element_repeat1, 2, 0, 0), SHIFT_REPEAT(65),
  [65] = {.entry = {.count = 1, .reusable = false}}, REDUCE(aux_sym_jsx_element_repeat1, 2, 0, 0),
  [67] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_element_repeat1, 2, 0, 0), SHIFT_REPEAT(15),
  [70] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_jsx_element_repeat1, 2, 0, 0), SHIFT_REPEAT(10),
  [73] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_source_file, 1, 0, 0),
  [75] = {.entry = {.count = 1, .reusable = false}}, SHIFT(17),
  [77] = {.entry = {.count = 1, .reusable = true}}, REDUCE(aux_sym_source_file_repeat1, 2, 0, 0),
  [79] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_source_file_repeat1, 2, 0, 0), SHIFT_REPEAT(17),
  [82] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_source_file_repeat1, 2, 0, 0), SHIFT_REPEAT(59),
  [85] = {.entry = {.count = 1, .reusable = true}}, SHIFT(12),
  [87] = {.entry = {.count = 1, .reusable = true}}, SHIFT(31),
  [89] = {.entry = {.count = 1, .reusable = true}}, SHIFT(32),
  [91] = {.entry = {.count = 1, .reusable = true}}, SHIFT(28),
  [93] = {.entry = {.count = 1, .reusable = true}}, SHIFT(56),
  [95] = {.entry = {.count = 1, .reusable = true}}, SHIFT(27),
  [97] = {.entry = {.count = 1, .reusable = true}}, SHIFT(35),
  [99] = {.entry = {.count = 1, .reusable = true}}, SHIFT(55),
  [101] = {.entry = {.count = 1, .reusable = true}}, SHIFT(42),
  [103] = {.entry = {.count = 1, .reusable = true}}, SHIFT(45),
  [105] = {.entry = {.count = 1, .reusable = true}}, SHIFT(26),
  [107] = {.entry = {.count = 1, .reusable = true}}, SHIFT(47),
  [109] = {.entry = {.count = 1, .reusable = true}}, REDUCE(aux_sym_jsx_opening_element_repeat1, 2, 0, 0),
  [111] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_jsx_opening_element_repeat1, 2, 0, 0), SHIFT_REPEAT(27),
  [114] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_self_closing_element, 4, 0, 1),
  [116] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_attribute, 1, 0, 2),
  [118] = {.entry = {.count = 1, .reusable = true}}, SHIFT(18),
  [120] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_opening_element, 3, 0, 1),
  [122] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_nested_braces, 2, 0, 0),
  [124] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_expression, 3, 0, 0),
  [126] = {.entry = {.count = 1, .reusable = false}}, SHIFT(52),
  [128] = {.entry = {.count = 1, .reusable = true}}, SHIFT(36),
  [130] = {.entry = {.count = 1, .reusable = false}}, SHIFT(36),
  [132] = {.entry = {.count = 1, .reusable = true}}, SHIFT(37),
  [134] = {.entry = {.count = 1, .reusable = false}}, SHIFT(37),
  [136] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_expression, 2, 0, 0),
  [138] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_nested_braces, 3, 0, 0),
  [140] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_opening_element, 4, 0, 1),
  [142] = {.entry = {.count = 1, .reusable = false}}, SHIFT(51),
  [144] = {.entry = {.count = 1, .reusable = true}}, SHIFT(39),
  [146] = {.entry = {.count = 1, .reusable = false}}, SHIFT(39),
  [148] = {.entry = {.count = 1, .reusable = true}}, SHIFT(40),
  [150] = {.entry = {.count = 1, .reusable = false}}, SHIFT(40),
  [152] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_closing_element, 3, 0, 1),
  [154] = {.entry = {.count = 1, .reusable = false}}, REDUCE(aux_sym_string_repeat1, 2, 0, 0),
  [156] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_string_repeat1, 2, 0, 0), SHIFT_REPEAT(39),
  [159] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_string_repeat1, 2, 0, 0), SHIFT_REPEAT(39),
  [162] = {.entry = {.count = 1, .reusable = false}}, REDUCE(aux_sym_string_repeat2, 2, 0, 0),
  [164] = {.entry = {.count = 2, .reusable = true}}, REDUCE(aux_sym_string_repeat2, 2, 0, 0), SHIFT_REPEAT(40),
  [167] = {.entry = {.count = 2, .reusable = false}}, REDUCE(aux_sym_string_repeat2, 2, 0, 0), SHIFT_REPEAT(40),
  [170] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_element, 2, 0, 0),
  [172] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_self_closing_element, 3, 0, 1),
  [174] = {.entry = {.count = 1, .reusable = false}}, REDUCE(sym_jsx_element, 3, 0, 0),
  [176] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_expression, 3, 0, 0),
  [178] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_expression, 2, 0, 0),
  [180] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_string, 3, 0, 0),
  [182] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_string, 2, 0, 0),
  [184] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_element, 3, 0, 0),
  [186] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_attribute, 3, 0, 3),
  [188] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_self_closing_element, 4, 0, 1),
  [190] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_self_closing_element, 3, 0, 1),
  [192] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_closing_element, 3, 0, 1),
  [194] = {.entry = {.count = 1, .reusable = true}}, REDUCE(sym_jsx_element, 2, 0, 0),
  [196] = {.entry = {.count = 1, .reusable = true}}, SHIFT(19),
  [198] = {.entry = {.count = 1, .reusable = true}}, SHIFT(57),
  [200] = {.entry = {.count = 1, .reusable = true}}, SHIFT(46),
  [202] = {.entry = {.count = 1, .reusable = true}}, SHIFT(60),
  [204] = {.entry = {.count = 1, .reusable = true}},  ACCEPT_INPUT(),
  [206] = {.entry = {.count = 1, .reusable = true}}, SHIFT(38),
  [208] = {.entry = {.count = 1, .reusable = true}}, SHIFT(21),
  [210] = {.entry = {.count = 1, .reusable = true}}, SHIFT(61),
  [212] = {.entry = {.count = 1, .reusable = true}}, SHIFT(24),
  [214] = {.entry = {.count = 1, .reusable = true}}, SHIFT(64),
};

#ifdef __cplusplus
extern "C" {
#endif
#ifdef TREE_SITTER_HIDE_SYMBOLS
#define TS_PUBLIC
#elif defined(_WIN32)
#define TS_PUBLIC __declspec(dllexport)
#else
#define TS_PUBLIC __attribute__((visibility("default")))
#endif

TS_PUBLIC const TSLanguage *tree_sitter_cppx(void) {
  static const TSLanguage language = {
    .version = LANGUAGE_VERSION,
    .symbol_count = SYMBOL_COUNT,
    .alias_count = ALIAS_COUNT,
    .token_count = TOKEN_COUNT,
    .external_token_count = EXTERNAL_TOKEN_COUNT,
    .state_count = STATE_COUNT,
    .large_state_count = LARGE_STATE_COUNT,
    .production_id_count = PRODUCTION_ID_COUNT,
    .field_count = FIELD_COUNT,
    .max_alias_sequence_length = MAX_ALIAS_SEQUENCE_LENGTH,
    .parse_table = &ts_parse_table[0][0],
    .small_parse_table = ts_small_parse_table,
    .small_parse_table_map = ts_small_parse_table_map,
    .parse_actions = ts_parse_actions,
    .symbol_names = ts_symbol_names,
    .field_names = ts_field_names,
    .field_map_slices = ts_field_map_slices,
    .field_map_entries = ts_field_map_entries,
    .symbol_metadata = ts_symbol_metadata,
    .public_symbol_map = ts_symbol_map,
    .alias_map = ts_non_terminal_alias_map,
    .alias_sequences = &ts_alias_sequences[0][0],
    .lex_modes = ts_lex_modes,
    .lex_fn = ts_lex,
    .primary_state_ids = ts_primary_state_ids,
  };
  return &language;
}
#ifdef __cplusplus
}
#endif
