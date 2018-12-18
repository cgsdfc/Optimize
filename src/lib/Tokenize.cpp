#include "simplecc/Tokenize.h"
#include "simplecc/ErrorManager.h"

#include <algorithm>
#include <iomanip>

using namespace simplecc;

static bool IsBlank(const String &line) {
  for (auto ch : line)
    if (!std::isspace(ch))
      return false;
  return true;
}

static bool IsValidChar(char ch) {
  static const String valid_chars("+-*/_");
  return valid_chars.find(ch) != String::npos || std::isalnum(ch);
}

static bool IsValidStrChar(char ch) {
  return ch == 32 || ch == 33 || (35 <= ch && ch <= 126);
}

static bool IsNameBegin(char ch) { return ch == '_' || std::isalpha(ch); }

static bool IsNameMiddle(char ch) { return ch == '_' || std::isalnum(ch); }

static bool IsSpecial(char ch) {
  static String special("[](){};:,");
  return special.find(ch) != String::npos;
}

static bool IsOperator(char ch) {
  static String operators("+-*/<>!=");
  return operators.find(ch) != String::npos;
}

static void ToLowerInplace(String &string) {
  std::transform(string.begin(), string.end(), string.begin(), ::tolower);
}


namespace simplecc {
void Tokenize(std::istream &Input, std::vector<TokenInfo> &Output) {
  unsigned lnum = 0;
  String line;

  Output.clear();
  while (std::getline(Input, line)) {
    ++lnum;
    if (IsBlank(line))
      continue;

    unsigned pos = 0;
    unsigned max = line.size();

    while (pos < max) {
      Location start(lnum, pos);
      Symbol type = Symbol::ERRORTOKEN;

      if (std::isdigit(line[pos])) {
        while (std::isdigit(line[pos])) {
          ++pos;
        }
        type = Symbol::NUMBER;
      } else if (line[pos] == '\'') {
        ++pos;
        if (IsValidChar(line[pos])) {
          ++pos;
          if (line[pos] == '\'') {
            ++pos;
            type = Symbol::CHAR;
          }
        }
      } else if (line[pos] == '\"') {
        ++pos;
        while (IsValidStrChar(line[pos])) {
          ++pos;
        }
        if (line[pos] == '\"') {
          ++pos;
          type = Symbol::STRING;
        }
      } else if (IsNameBegin(line[pos])) {
        ++pos;
        while (IsNameMiddle(line[pos]))
          ++pos;
        type = Symbol::NAME;
      } else if (IsSpecial(line[pos])) {
        ++pos;
        type = Symbol::OP;
      } else if (IsOperator(line[pos])) {
        char chr = line[pos];
        ++pos;
        if (chr == '>' || chr == '<' || chr == '=') {
          type = Symbol::OP;
          if (line[pos] == '=') {
            ++pos;
          }
        } else if (chr == '!') {
          if (line[pos] == '=') {
            ++pos;
            type = Symbol::OP;
          }
        } else {
          type = Symbol::OP;
        }
      } else if (std::isspace(line[pos])) {
        while (std::isspace(line[pos]))
          ++pos;
        continue;
      } else {
        ++pos; // ERRORTOKEN
      }
      String token(line.begin() + start.getColOffset(), line.begin() + pos);
      if (type == Symbol::NAME) {
        ToLowerInplace(token);
      }
      Output.emplace_back(type, token, start, line);
    }
  }
  Output.emplace_back(Symbol::ENDMARKER, "", Location(lnum, 0), "");
}

static void DumpTokenInfo(std::ostream &O, const TokenInfo &T) {
  auto &&Loc = T.getLocation();
  std::ostringstream OS;
  OS << Loc.getLineNo() << ',' << Loc.getColOffset() << ":";
  auto &&Range = OS.str();

  O << std::left << std::setw(20) << Range;
  O << std::left << std::setw(15) << T.getTypeName();
  O << std::left << std::setw(15) << Quote(T.getString()) << "\n";
}

void PrintTokens(const std::vector<TokenInfo> &Tokens, std::ostream &O) {
  std::for_each(Tokens.begin(), Tokens.end(), [&O](const TokenInfo &T) {
    DumpTokenInfo(O, T);
  });
}

} // namespace simplecc

void TokenInfo::Format(std::ostream &O) const {
  O << "TokenInfo("
     << "type=" << getTypeName() << ", "
     << "string=" << Quote(getString()) << ", "
     << getLocation() << ", "
     << "line=" << Quote(getLine()) << ")";
}

const char *TokenInfo::getTypeName() const { return getSymbolName(Type); }
