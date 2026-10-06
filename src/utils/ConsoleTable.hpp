#pragma once
#include <Arduino.h>
#include <string.h>

// Console table without dynamic memory, String, or floating-point arithmetic.
//
// COLS     - number of columns
// MAX_ROWS - maximum number of rows (data + sections + separators)
// BUF_SIZE - buffer size for numbers formatted by numFixed()/numInt()
//
// IMPORTANT: Cell text is NOT copied; only a pointer is stored.
// Pass string literals or the result of numFixed()/numInt().
// Do not pass temporary String/char[] objects - they may no longer exist
// when print() is called.
template <uint8_t COLS, uint8_t MAX_ROWS = 16, uint16_t BUF_SIZE = 64>
class ConsoleTable
{
  static_assert(COLS >= 1, "Need at least one column");

public:
  enum class Align : uint8_t
  {
    Left,
    Right,
    Center
  };

  explicit ConsoleTable(Print &out) : _out(out) { clear(); }

  // Pointers refer to memory inside _buf, so copying is not allowed.
  ConsoleTable(const ConsoleTable &) = delete;
  ConsoleTable &operator=(const ConsoleTable &) = delete;

  ConsoleTable &setTitle(const char *title)
  {
    _title = title;
    return *this;
  }

  template <typename... Rest>
  ConsoleTable &setHeader(const char *first, Rest... rest)
  {
    static_assert(1 + sizeof...(Rest) <= COLS, "Too many header cells");
    const char *tmp[] = {first, rest...};
    for (uint8_t i = 0; i < 1 + sizeof...(Rest); i++)
    {
      _header[i] = tmp[i];
      widen(i, tmp[i]);
    }
    _hasHeader = true;
    return *this;
  }

  ConsoleTable &setAlign(uint8_t col, Align a)
  {
    if (col < COLS)
      _align[col] = a;
    return *this;
  }

  template <typename... Rest>
  ConsoleTable &addRow(const char *first, Rest... rest)
  {
    static_assert(1 + sizeof...(Rest) <= COLS, "Too many cells");
    int16_t r = newRow(RowType::Data);
    if (r < 0)
      return *this;
    const char *tmp[] = {first, rest...};
    for (uint8_t i = 0; i < 1 + sizeof...(Rest); i++)
    {
      _cells[r][i] = tmp[i];
      widen(i, tmp[i]);
    }
    return *this;
  }

  // Add a row spanning the full table width (for example, "AHT20").
  ConsoleTable &addSection(const char *text)
  {
    int16_t r = newRow(RowType::Section);
    if (r >= 0)
      _cells[r][0] = text;
    return *this;
  }

  ConsoleTable &addSeparator()
  {
    newRow(RowType::Separator);
    return *this;
  }

  // Fixed-point formatting without float:
  // numFixed(2537, 2, "°C") -> "25.37 °C"
  const char *numFixed(int32_t v, uint8_t decimals = 2, const char *unit = nullptr)
  {
    static constexpr uint32_t POW10[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    if (decimals > 6)
      decimals = 6;

    const bool neg = v < 0;
    const uint32_t u = neg ? 0UL - static_cast<uint32_t>(v)
                           : static_cast<uint32_t>(v);
    const uint32_t scale = POW10[decimals];
    uint32_t ip = u / scale;
    uint32_t fp = u % scale;

    char tmp[20]; // Sign + 10 digits + decimal point + 6 fractional digits = 18
    char *p = tmp;
    if (neg)
      *p++ = '-';

    char rev[10];
    uint8_t n = 0;
    do
    {
      rev[n++] = '0' + (ip % 10);
      ip /= 10;
    } while (ip);
    while (n)
      *p++ = rev[--n];

    if (decimals)
    {
      *p++ = '.';
      for (uint8_t i = decimals; i > 0; i--)
      {
        p[i - 1] = '0' + (fp % 10);
        fp /= 10;
      }
      p += decimals;
    }
    return store(tmp, p - tmp, unit);
  }

  const char *numInt(long v, const char *unit = nullptr)
  {
    char tmp[14];
    char *p = tmp;
    unsigned long u = (v < 0) ? 0UL - (unsigned long)v : (unsigned long)v;
    if (v < 0)
      *p++ = '-';
    char rev[11];
    uint8_t n = 0;
    do
    {
      rev[n++] = '0' + (u % 10);
      u /= 10;
    } while (u);
    while (n)
      *p++ = rev[--n];
    return store(tmp, p - tmp, unit);
  }

  void clear()
  {
    _rowCount = 0;
    _bufUsed = 0;
    _title = nullptr;
    _hasHeader = false;
    for (uint8_t i = 0; i < COLS; i++)
    {
      _w[i] = 0;
      _header[i] = nullptr;
    }
  }

  void print()
  {
    // Width between "| " and " |".
    size_t inner = 3 * (COLS - 1);
    for (uint8_t i = 0; i < COLS; i++)
      inner += _w[i];

    // If the title/section is wider than the table, expand the last column.
    size_t need = _title ? textLen(_title) : 0;
    for (uint8_t r = 0; r < _rowCount; r++)
    {
      if (_types[r] == (uint8_t)RowType::Section)
      {
        size_t l = textLen(_cells[r][0]);
        if (l > need)
          need = l;
      }
    }
    size_t extra = (need > inner) ? need - inner : 0;
    inner += extra;

    auto colW = [&](uint8_t i) -> size_t
    { return _w[i] + (i == COLS - 1 ? extra : 0); };

    bool lastWasLine = false;

    auto line = [&]()
    {
      if (lastWasLine)
        return;
      _out.print('+');
      for (uint8_t i = 0; i < COLS; i++)
      {
        repeat('-', colW(i) + 2);
        _out.print('+');
      }
      _out.println();
      lastWasLine = true;
    };
    auto fullRow = [&](const char *s, Align a)
    {
      _out.print("| ");
      pad(s, inner, a);
      _out.println(" |");
      lastWasLine = false;
    };
    auto dataRow = [&](const char *const *cells)
    {
      _out.print('|');
      for (uint8_t i = 0; i < COLS; i++)
      {
        _out.print(' ');
        pad(cells[i], colW(i), _align[i]);
        _out.print(" |");
      }
      _out.println();
      lastWasLine = false;
    };

    line();
    if (_title)
    {
      fullRow(_title, Align::Center);
      line();
    }
    if (_hasHeader)
    {
      dataRow(_header);
      line();
    }
    for (uint8_t r = 0; r < _rowCount; r++)
    {
      switch ((RowType)_types[r])
      {
      case RowType::Data:
        dataRow(_cells[r]);
        break;
      case RowType::Section:
        line();
        fullRow(_cells[r][0], Align::Left);
        line();
        break;
      case RowType::Separator:
        line();
        break;
      }
    }
    line();
  }

private:
  enum class RowType : uint8_t
  {
    Data,
    Section,
    Separator
  };

  // Return the number of UTF-8 characters.
  // Continuation bytes (10xxxxxx) are not counted.
  static size_t textLen(const char *s)
  {
    size_t n = 0;
    if (!s)
      return 0;
    for (; *s; s++)
      if (((uint8_t)*s & 0xC0) != 0x80)
        n++;
    return n;
  }

  void widen(uint8_t col, const char *s)
  {
    size_t n = textLen(s);
    if (n > 255)
      n = 255;
    if (n > _w[col])
      _w[col] = (uint8_t)n;
  }

  int16_t newRow(RowType t)
  {
    if (_rowCount >= MAX_ROWS)
      return -1;
    uint8_t r = _rowCount++;
    _types[r] = (uint8_t)t;
    for (uint8_t i = 0; i < COLS; i++)
      _cells[r][i] = nullptr;
    return r;
  }

  // Copy "number [space unit]" into _buf and return a pointer to the copy.
  const char *store(const char *s, size_t n, const char *unit)
  {
    size_t ulen = (unit && *unit) ? strlen(unit) : 0;
    size_t total = n + (ulen ? 1 + ulen : 0) + 1;
    if (total > (size_t)(BUF_SIZE - _bufUsed))
      return "ovf";
    char *start = _buf + _bufUsed;
    char *d = start;
    memcpy(d, s, n);
    d += n;
    if (ulen)
    {
      *d++ = ' ';
      memcpy(d, unit, ulen);
      d += ulen;
    }
    *d = '\0';
    _bufUsed += total;
    return start;
  }

  void repeat(char c, size_t n)
  {
    while (n--)
      _out.print(c);
  }

  void pad(const char *s, size_t width, Align a)
  {
    size_t len = textLen(s);
    size_t space = (width > len) ? width - len : 0;
    size_t left = 0, right = 0;
    switch (a)
    {
    case Align::Left:
      right = space;
      break;
    case Align::Right:
      left = space;
      break;
    case Align::Center:
      left = space / 2;
      right = space - left;
      break;
    }
    repeat(' ', left);
    if (s)
      _out.print(s);
    repeat(' ', right);
  }

  Print &_out;
  const char *_title = nullptr;
  const char *_header[COLS];
  const char *_cells[MAX_ROWS][COLS];
  uint8_t _types[MAX_ROWS];
  uint8_t _w[COLS];
  Align _align[COLS] = {}; // All columns are left-aligned by default.
  char _buf[BUF_SIZE];
  uint16_t _bufUsed = 0;
  uint8_t _rowCount = 0;
  bool _hasHeader = false;
};
