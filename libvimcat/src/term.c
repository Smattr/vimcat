#include "term.h"
#include "buffer.h"
#include "colour.h"
#include "compiler.h"
#include "debug.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// flip this to false to enable checks against untrusted input
enum { TRUST_CALLER = true };

#define PRECONDITION(cond)                                                     \
  do {                                                                         \
    if (TRUST_CALLER) {                                                        \
      assert(cond);                                                            \
    } else {                                                                   \
      if (ERROR(!(cond))) {                                                    \
        return EINVAL;                                                         \
      }                                                                        \
    }                                                                          \
  } while (0)

typedef struct {
  unsigned custom_fg : 1; ///< is `fg` non-default?
  unsigned custom_bg : 1; ///< is `bg` non-default?
  unsigned bold : 1;      ///< is bold enabled?
  unsigned underline : 1; ///< is underline enabled?
  colour_t fg;            ///< foreground colour
  colour_t bg;            ///< background colour
} style_t;

static style_t style_default(void) { return (style_t){0}; }

static bool style_eq(style_t a, style_t b) {

  if (a.custom_fg != b.custom_fg)
    return false;

  if (a.custom_bg != b.custom_bg)
    return false;

  if (a.bold != b.bold)
    return false;

  if (a.underline != b.underline)
    return false;

  if (a.custom_fg && !colour_eq(a.fg, b.fg))
    return false;

  if (a.custom_bg && !colour_eq(a.bg, b.bg))
    return false;

  return true;
}

/// write a directive for the given style
static int style_put(style_t style, FILE *f) {
  assert(f != NULL);

  if (ERROR(fputs("\033[", f) == EOF))
    return errno;

  // emit the foreground colour
  do {

    // default?
    if (!style.custom_fg) {
      if (ERROR(fputs("39;", f) == EOF))
        return errno;
      break;
    }

    unsigned colour8 = colour_24_to_8(style.fg);

    // can we do it as a 3-bit colour?
    if (colour8 <= 7) {
      if (ERROR(fprintf(f, "%u;", 30 + colour8) < 0))
        return errno;
      break;
    }

    // can we do it as a 4-bit colour?
    if (colour8 <= 15) {
      if (ERROR(fprintf(f, "%u;", 90 + colour8 - 8) < 0))
        return errno;
      break;
    }

    // can we do it as an 8-bit colour?
    if (colour8 <= 255) {
      if (ERROR(fprintf(f, "38;5;%um\033[", colour8) < 0))
        return errno;
      break;
    }

    // otherwise, 24-bit colour
    if (ERROR(fprintf(f, "38;2;%u;%u;%um\033[", (unsigned)style.fg.r,
                      (unsigned)style.fg.g, (unsigned)style.fg.b) < 0))
      return errno;

  } while (0);

  // emit the background colour
  do {

    // default?
    if (!style.custom_bg) {
      if (ERROR(fputs("49;", f) == EOF))
        return errno;
      break;
    }

    unsigned colour8 = colour_24_to_8(style.bg);

    // can we do it as a 3-bit colour?
    if (colour8 <= 7) {
      if (ERROR(fprintf(f, "%u;", 40u + colour8) < 0))
        return errno;
      break;
    }

    // can we do it as a 4-bit colour?
    if (colour8 <= 15) {
      if (ERROR(fprintf(f, "%u;", 100 + colour8 - 8) < 0))
        return errno;
      break;
    }

    // can we do it as an 8-bit colour?
    if (ERROR(fprintf(f, "48;5;%um\033[", colour8) < 0))
      return errno;

    // otherwise, 24-bit colour
    if (ERROR(fprintf(f, "48;2;%u;%u;%um\033[", (unsigned)style.bg.r,
                      (unsigned)style.bg.g, (unsigned)style.bg.b) < 0))
      return errno;

  } while (0);

  if (style.bold) {
    if (ERROR(fputs("1;", f) == EOF))
      return errno;
  } else {
    if (ERROR(fputs("22;", f) == EOF))
      return errno;
  }

  if (style.underline) {
    if (ERROR(fputs("4", f) == EOF))
      return errno;
  } else {
    if (ERROR(fputs("24", f) == EOF))
      return errno;
  }

  if (ERROR(fputc('m', f) == EOF))
    return errno;

  return 0;
}

/// a UTF-8 character
typedef struct {
  char bytes[4];
} utf8_t;

static bool utf8eq(utf8_t u, const char *s) {
  return strlen(s) <= sizeof(u.bytes) &&
         strncmp(u.bytes, s, sizeof(u.bytes)) == 0;
}

/// A grapheme, stored as a single UTF-8 character. Note that this
/// representation does not account for non-spacing combining marks, a
/// weakness we accept for the sake of efficiency.
typedef struct {
  utf8_t value;
} grapheme_t;

static bool grapheme_is_nul(const grapheme_t *g) {
  assert(g != NULL);
  return utf8eq(g->value, "");
}

static int grapheme_put(const grapheme_t *g, FILE *f) {
  assert(g != NULL);
  assert(f != NULL);

  if (ERROR(fprintf(f, "%.*s", (int)sizeof(g->value.bytes), g->value.bytes) <
            0))
    return errno;

  return 0;
}

static void grapheme_free(grapheme_t *g) {
  assert(g != NULL);
  memset(g, 0, sizeof(*g));
}

/// a 1-grapheme region of the terminal
typedef struct {
  grapheme_t grapheme;
  style_t style; ///< colour, format, etc for this character
} cell_t;

static bool cell_is_empty(const cell_t *c) {
  assert(c != NULL);

  return grapheme_is_nul(&c->grapheme);
}

static void cell_clear(cell_t *c) {
  assert(c != NULL);

  grapheme_free(&c->grapheme);
  memset(c, 0, sizeof(*c));
}

static int cell_put(const cell_t *c, FILE *f) {
  assert(c != NULL);
  assert(f != NULL);

  return grapheme_put(&c->grapheme, f);
}

struct term {
  /// dimensions of the terminal
  size_t columns;
  size_t rows;

  /// cursor position
  size_t x;
  size_t y;

  /// currently active style
  style_t style;

  /// scratch space for doing transient text manipulation
  buffer_t stage;

  /// data on the terminal
  cell_t screen[];
};

int term_new(term_t **t, size_t columns, size_t rows) {

  PRECONDITION(t != NULL);
  PRECONDITION(columns > 0);
  PRECONDITION(rows > 0);

  term_t *term = calloc(1, sizeof(*term) + sizeof(cell_t) * columns * rows);
  if (ERROR(term == NULL))
    return ENOMEM;

  int rc = 0;

  term->columns = columns;
  term->rows = rows;

  term->x = 1;
  term->y = 1;

  if (ERROR((rc = buffer_open(&term->stage))))
    goto done;

  // success
  *t = term;

done:
  if (UNLIKELY(rc != 0))
    term_free(&term);

  return rc;
}

/// get the cell at the given coordinates
static cell_t *get_cell(term_t *t, size_t x, size_t y) {
  assert(t != NULL);
  assert(x > 0);
  assert(x <= t->columns);
  assert(y > 0);
  assert(y <= t->rows);

  // indexing is 1-based
  size_t offset = t->columns * (y - 1) + x - 1;
  return &t->screen[offset];
}

/// get the cell we are currently pointing at
static cell_t *get_current_cell(term_t *t) {
  assert(t != NULL);

  return get_cell(t, t->x, t->y);
}

static int process_A(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  if (index > 0)
    return EBADMSG;
  if (entry >= t->y) {
    t->y = 1;
  } else {
    t->y -= entry;
  }
  return 0;
}

static int process_B(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  if (index > 0)
    return EBADMSG;
  if (entry + t->y > t->rows) {
    t->y = t->rows;
  } else {
    t->y += entry;
  }
  return 0;
}

static int process_C(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  if (index > 0)
    return EBADMSG;
  if (entry + t->x > t->columns) {
    t->x = t->columns;
  } else {
    t->x += entry;
  }
  return 0;
}

static int process_D(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  if (index > 0)
    return EBADMSG;
  if (entry >= t->x) {
    t->x = 1;
  } else {
    t->x -= entry;
  }
  return 0;
}

static int process_E(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  t->x = 1;
  return process_B(t, index, is_default, entry);
}

static int process_F(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  t->x = 1;
  return process_A(t, index, is_default, entry);
}

static int process_G(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  if (index > 0)
    return EBADMSG;
  if (entry <= t->columns)
    t->x = entry;
  return 0;
}

static int process_H(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);
  if (is_default)
    entry = 1;
  switch (index) {
  case 0:
    if (entry <= t->rows)
      t->y = entry;
    break;
  case 1:
    if (entry <= t->columns)
      t->x = entry;
    break;
  default:
    return EBADMSG;
  }
  return 0;
}

static void term_clear(term_t *t) {

  if (t == NULL)
    return;

  for (size_t y = 1; y <= t->rows; ++y) {
    for (size_t x = 1; x <= t->columns; ++x) {
      cell_t *cell = get_cell(t, x, y);
      cell_clear(cell);
    }
  }
}

static int process_J(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);

  if (is_default)
    entry = 0;

  if (index > 0)
    return EBADMSG;

  switch (entry) {

  case 0: { // clear to end of screen
    size_t offset = t->x;
    for (size_t y = t->y; y <= t->rows; ++y) {
      for (size_t x = offset; x <= t->columns; ++x) {
        cell_t *cell = get_cell(t, x, y);
        cell_clear(cell);
      }
      offset = 1;
    }
    break;
  }

  case 1: { // clear to beginning of screen
    size_t offset = t->x;
    for (size_t y = t->y; y > 0; --y) {
      for (size_t x = offset; x > 0; --x) {
        cell_t *cell = get_cell(t, x, y);
        cell_clear(cell);
      }
      offset = t->columns;
    }
    break;
  }

  case 2: // clear entire screen
  case 3: // clear entire screen and delete scrollback
    term_clear(t);
    break;

  default:
    return EBADMSG;
  }

  return 0;
}

/// handle `<esc>[38;2;<r>;<g>;<b>m`
static int process_38_2_m(term_t *t, size_t r, size_t g, size_t b) {
  assert(t != NULL);

  if (r > UINT8_MAX || g > UINT8_MAX || b > UINT8_MAX) {
    DEBUG("out of range SGR attribute <esc>[38;2;%zu;%zu;%zum", r, g, b);
    return ENOTSUP;
  }

  t->style.custom_fg = true;
  t->style.fg.r = (uint8_t)r;
  t->style.fg.g = (uint8_t)g;
  t->style.fg.b = (uint8_t)b;

  return 0;
}

/// handle `<esc>[48;2;<r>;<g>;<b>m`
static int process_48_2_m(term_t *t, size_t r, size_t g, size_t b) {
  assert(t != NULL);

  if (r > UINT8_MAX || g > UINT8_MAX || b > UINT8_MAX) {
    DEBUG("out of range SGR attribute <esc>[48;2;%zu;%zu;%zum", r, g, b);
    return ENOTSUP;
  }

  t->style.custom_bg = true;
  t->style.bg.r = (uint8_t)r;
  t->style.bg.g = (uint8_t)g;
  t->style.bg.b = (uint8_t)b;

  return 0;
}

/// handle `<esc>[38;5;<id>m`
static int process_38_5_m(term_t *t, size_t id) {
  assert(t != NULL);

  if (id > UINT8_MAX) {
    DEBUG("out of range SGR attribute <esc>[38;5;%zum", id);
    return ENOTSUP;
  }

  const colour_t c = colour_8_to_24((uint8_t)id);
  return process_38_2_m(t, c.r, c.g, c.b);
}

/// handle `<esc>[48;5;<id>m`
static int process_48_5_m(term_t *t, size_t id) {
  assert(t != NULL);

  if (id > UINT8_MAX) {
    DEBUG("out of range SGR attribute <esc>[48;5;%zum", id);
    return ENOTSUP;
  }

  const colour_t c = colour_8_to_24((uint8_t)id);
  return process_48_2_m(t, c.r, c.g, c.b);
}

static int process_m(term_t *t, size_t index, bool is_default, size_t entry) {
  assert(t != NULL);

  // ignore index
  (void)index;

  if (is_default)
    entry = 0;

  switch (entry) {
  case 0:
    memset(&t->style, 0, sizeof(t->style));
    break;

  case 1:
    t->style.bold = true;
    break;

  case 4:
    t->style.underline = true;
    break;

  case 22:
    t->style.bold = false;
    break;
  case 24:
    t->style.underline = false;
    break;

  // treat reset of features we do not support as a no-op
  case 23: // reset italic
  case 25: // reset blinking
  case 27: // reset inverse/reverse
  case 28: // reset hidden/invisible
  case 29: // reset strikethrough
    break;

  case 30 ... 37:
    return process_38_5_m(t, entry - 30);

  case 39:
    t->style.custom_fg = false;
    break;

  case 40 ... 47:
    return process_48_5_m(t, entry - 40);

  case 49:
    t->style.custom_bg = false;
    break;

  case 90 ... 97:
    return process_38_5_m(t, entry - 90 + 8);

  case 100 ... 107:
    return process_48_5_m(t, entry - 100 + 8);

  default:
    DEBUG("unsupported SGR attribute <esc>[%zum", entry);
    return ENOTSUP;
  }

  return 0;
}

/// parse and apply the effect of an ANSI CSI sequence
static int process_csi(term_t *t, const char *csi) {

  assert(t != NULL);
  assert(csi != NULL);
  assert(strlen(csi) > 0);

  char final = csi[strlen(csi) - 1];

  // if this is a private sequence, ignore it
  if (strchr(csi, '<') || strchr(csi, '=') || strchr(csi, '>') ||
      strchr(csi, '?') || (final >= 0x70 && final <= 0x7e)) {
    DEBUG("ignoring private sequence <esc>[%s", csi);
    return 0;
  }

  // if this is Set Mode, ignore it
  if (final == 'h') {
    DEBUG("ignoring set mode <esc>[%s", csi);
    return 0;
  }

  // <esc>[H is shorthand for move to origin
  if (strcmp(csi, "H") == 0) {
    t->x = 1;
    t->y = 1;
    return 0;
  }

#ifdef __APPLE__
  // Vim on macOS when `t_Co=2` can emit unusual sequences like `<esc>[311m`
  // that appear to have no defined meaning. This seems to result from an
  // incorrect call to `term_color` in Vim. I do not know if this is a Vim bug
  // or a bug in some default configuration on macOS or simply violation of an
  // assumption that there are no monochrome macOS environments (reasonable).
  // Just ignore this sequence if we see it.
  if (UNLIKELY(csi[0] == '3' && csi[1] == '1' && isdigit(csi[2]) &&
               csi[3] == 'm')) {
    DEBUG("ignoring <esc>[%s", csi);
    return 0;
  }
#endif

  // check we have a known CSI
  int (*handler)(term_t * t, size_t index, bool is_default, size_t entry) =
      NULL;
  switch (final) {
  case 'A': // Cursor Up
    handler = process_A;
    break;
  case 'B': // Cursor Down
    handler = process_B;
    break;
  case 'C': // Cursor Forward
    handler = process_C;
    break;
  case 'D': // Cursor Back
    handler = process_D;
    break;
  case 'E': // Cursor Next Line
    handler = process_E;
    break;
  case 'F': // Cursor Previous Line
    handler = process_F;
    break;
  case 'G': // Cursor Horizontal Absolute
    handler = process_G;
    break;
  case 'H': // Cursor Position
    handler = process_H;
    break;
  case 'J': // Erase in Display
    handler = process_J;
    break;
  case 'm': // Select Graphic Rendition (SGR)
    handler = process_m;
    break;
  default:
    DEBUG("unrecognised CSI sequence <esc>[%s", csi);
    return ENOTSUP;
  }

  DEBUG("processing <esc>[%s", csi);

  if (final == 'm') {

    // is this an 8-bit colour foreground switch?
    bool is_8_fg = strncmp(csi, "38;5;", strlen("38;5;")) == 0;
    if (is_8_fg) {
      const char *idm = csi + strlen("38;5;");
      size_t id = 0;
      for (; isdigit(*idm); ++idm)
        id = id * 10 + *idm - '0';
      if (*idm == 'm')
        return process_38_5_m(t, id);
    }

    // is this an 8-bit colour background switch?
    bool is_8_bg = strncmp(csi, "48;5;", strlen("48;5;")) == 0;
    if (is_8_bg) {
      const char *idm = csi + strlen("48;5;");
      size_t id = 0;
      for (; isdigit(*idm); ++idm)
        id = id * 10 + *idm - '0';
      if (*idm == 'm')
        return process_48_5_m(t, id);
    }

    // is this a 24-bit colour foreground switch?
    bool is_24_fg = strncmp(csi, "38;2;", strlen("38;2;")) == 0;
    if (is_24_fg) {
      do {
        const char *idm = csi + strlen("38;2");
        size_t r = 0;
        for (++idm; isdigit(*idm); ++idm)
          r = r * 10 + *idm - '0';
        if (*idm != ';')
          break;
        size_t g = 0;
        for (++idm; isdigit(*idm); ++idm)
          g = g * 10 + *idm - '0';
        if (*idm != ';')
          break;
        size_t b = 0;
        for (++idm; isdigit(*idm); ++idm)
          b = b * 10 + *idm - '0';
        if (*idm == 'm')
          return process_38_2_m(t, r, g, b);
      } while (0);
    }

    // is this a 24-bit colour background switch?
    bool is_24_bg = strncmp(csi, "48;2;", strlen("48;2;")) == 0;
    if (is_24_bg) {
      do {
        const char *idm = csi + strlen("48;2");
        size_t r = 0;
        for (++idm; isdigit(*idm); ++idm)
          r = r * 10 + *idm - '0';
        if (*idm != ';')
          break;
        size_t g = 0;
        for (++idm; isdigit(*idm); ++idm)
          g = g * 10 + *idm - '0';
        if (*idm != ';')
          break;
        size_t b = 0;
        for (++idm; isdigit(*idm); ++idm)
          b = b * 10 + *idm - '0';
        if (*idm == 'm')
          return process_48_2_m(t, r, g, b);
      } while (0);
    }
  }

  // process ';' separated entries
  size_t index = 0;
  bool is_default = true;
  size_t entry = 0;
  for (size_t i = 0;; ++i) {

    if (isdigit(csi[i])) {
      entry = entry * 10 + csi[i] - '0';
      is_default = false;

    } else { // end of an entry

      int rc = handler(t, index, is_default, entry);
      if (UNLIKELY(rc != 0))
        return rc;

      // are we done?
      if (csi[i] != ';')
        break;

      ++index;
      is_default = true;
      entry = 0;
    }
  }

  return 0;
}

/// consume a character and advance if it matches the expected
static bool eat_if(FILE *f, char expected) {
  assert(f != NULL);
  int c = getc(f);
  if (c == expected)
    return true;
  if (c != EOF)
    ungetc(c, f);
  return false;
}

/// read a UTF-8 character from the given stream
static utf8_t get_utf8(FILE *f) {
  assert(f != NULL);

  // character to emit for malformed UTF-8 data
  static const utf8_t REPLACEMENT = (utf8_t){.bytes = "�"};

  int c = getc(f);
  if (c == EOF)
    return (utf8_t){{0}};

  utf8_t u = {{c}};

  // recognise Windows line endings and treat them as a single character
  if (c == '\r' && eat_if(f, '\n')) {
    u.bytes[1] = '\n';
    return u;
  }

  // From the UTF-8 RFC (3629):
  //
  //   Char. number range  |        UTF-8 octet sequence
  //      (hexadecimal)    |              (binary)
  //   --------------------+---------------------------------------------
  //   0000 0000-0000 007F | 0xxxxxxx
  //   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
  //   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
  //   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  //
  // So we can determine the length of the current character by the bits set in
  // the first byte.

  size_t length = 1;
  // is this a 1-byte character? Assume this is the common case, given we are
  // parsing programming source code.
  if (LIKELY(((uint8_t)c >> 7) == 0)) {
    return u;
  } else if (((uint8_t)c >> 5) == 6) {
    length = 2;
  } else if (((uint8_t)c >> 4) == 14) {
    length = 3;
  } else if (((uint8_t)c >> 3) == 30) {
    length = 4;
  } else {
    // malformed first byte
    return REPLACEMENT;
  }

  assert(length >= 2 && length <= 4);

  // read byte 2
  c = getc(f);
  if (c == EOF || ((uint8_t)c >> 6) != 2) {
    DEBUG("malformed byte 0x%x seen", (unsigned)c);
    if (c != EOF)
      ungetc(c, f);
    return REPLACEMENT;
  }
  u.bytes[1] = c;

  if (length == 2)
    return u;

  // read byte 3
  c = getc(f);
  if (c == EOF || ((uint8_t)c >> 6) != 2) {
    DEBUG("malformed byte 0x%x seen", (unsigned)c);
    if (c != EOF)
      ungetc(c, f);
    return REPLACEMENT;
  }
  u.bytes[2] = c;

  if (length == 3)
    return u;

  // read byte 4
  c = getc(f);
  if (c == EOF || ((uint8_t)c >> 6) != 2) {
    DEBUG("malformed byte 0x%x seen", (unsigned)c);
    if (c != EOF)
      ungetc(c, f);
    return REPLACEMENT;
  }
  u.bytes[3] = c;

  return u;
}

int term_send(term_t *t, FILE *from) {

  PRECONDITION(t != NULL);
  PRECONDITION(from != NULL);

  while (true) {

    utf8_t u = get_utf8(from);
    if (utf8eq(u, "")) // EOF
      break;

    // is this an escape sequence?
    if (utf8eq(u, "\033")) {

      // is this a Control Sequence Introducer (CSI)?
      if (eat_if(from, '[')) {

        // clear our temporary buffer to stage the sequence
        buffer_clear(&t->stage);

        // drain to the terminator of the CSI sequence
        while (true) {

          int c = getc(from);
          if (c == EOF) {
            // malformed sequence, as we have not yet seen the terminator
            return EBADMSG;
          }

          // stage this character
          if (ERROR(fputc(c, t->stage.f) == EOF))
            return errno;

          // was this the CSI sequence terminator?
          if (c >= 0x40 && c <= 0x7e)
            break;
        }
        buffer_sync(&t->stage);

        // apply the effects of this sequence
        int rc = process_csi(t, t->stage.base);
        if (ERROR(rc != 0))
          return rc;

        continue;
      }

      // is this the Application Keypad sequence?
      if (eat_if(from, '=')) {
        // ignore
        continue;
      }

      // is this the Normal Keypad sequence?
      if (eat_if(from, '>')) {
        // ignore
        continue;
      }

      DEBUG("unsupported escape sequence");
      return ENOTSUP;
    }

    // what cell are we pointing at?
    cell_t *cell = get_current_cell(t);

    cell_clear(cell);

    // for a newline, just leave the cell clear
    if (!utf8eq(u, "\n") && !utf8eq(u, "\r\n")) {

      cell->style = t->style;
      cell->grapheme.value = u;
    }

    // advance our cell position
    if (utf8eq(u, "\n") || utf8eq(u, "\r\n") || t->x == t->columns) {
      if (t->y < t->rows) {
        ++t->y;
        t->x = 1;
      }
    } else {
      ++t->x;
    }
  }

  return 0;
}

int term_readline(term_t *t, size_t row, const char **line) {

  PRECONDITION(t != NULL);
  PRECONDITION(row > 0);
  PRECONDITION(row <= t->rows);
  PRECONDITION(line != NULL);

  // reset our staging buffer to prepare for reuse
  buffer_clear(&t->stage);
  FILE *f = t->stage.f;

  // assume we are beginning with a default style
  style_t style = style_default();

  // pre-calculate the length of this line, stripping trailing cells
  size_t limit = t->columns;
  while (limit > 0) {
    const cell_t *cell = get_cell(t, limit, row);
    if (!cell_is_empty(cell))
      break;
    --limit;
  }

  for (size_t i = 0; i < limit; ++i) {

    const cell_t *cell = get_cell(t, i + 1, row);

    // update style for this grapheme, if necessary
    if (!style_eq(style, cell->style)) {
      int rc = style_put(cell->style, f);
      if (ERROR(rc != 0))
        return rc;
      style = cell->style;
    }

    // if this cell is empty, write a space to mimic its effect
    if (cell_is_empty(cell)) {
      if (ERROR(fputc(' ', f) == EOF))
        return errno;

      // otherwise write the character itself
    } else {
      int rc = cell_put(cell, f);
      if (ERROR(rc != 0))
        return rc;
    }
  }

  // reset the style to simplify the caller’s life
  if (!style_eq(style, style_default())) {
    if (ERROR(fputs("\033[0m", f) == EOF))
      return errno;
  }

  // success; NUL terminate the buffer and make it available to the caller
  buffer_sync(&t->stage);
  *line = t->stage.base;

  return 0;
}

void term_reset(term_t *t) {

  if (t == NULL)
    return;

  term_clear(t);

  // reset cursor to home
  t->x = 1;
  t->y = 1;

  // reset current style
  t->style = style_default();
}

void term_free(term_t **t) {

  if (t == NULL)
    return;

  if (*t == NULL)
    return;

  term_clear(*t);

  buffer_close(&(*t)->stage);

  free(*t);

  *t = NULL;
}
