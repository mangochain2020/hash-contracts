
#include <string>

using namespace std;

#define TO_ASSET( amount, code ) \
    asset(amount, symbol(code, PRECISION))

inline std::string add_symbol(symbol symbol0, symbol symbol1){
    std::string code =  symbol0.code().to_string() + symbol1.code().to_string();
    return code.substr(0,7);
}

static constexpr uint64_t char_to_symbol( char c ) {
    if (c >= 'a' && c <= 'z')
        return (c - 'a') + 6;
    if (c >= '1' && c <= '5')
        return (c - '1') + 1;
    return 0;
}

// Each char of the string is encoded into 5-bit chunk and left-shifted
// to its 5-bit slot starting with the highest slot for the first char.
// The 13th char, if str is long enough, is encoded into 4-bit chunk
// and placed in the lowest 4 bits. 64 = 12 * 5 + 4
static constexpr uint64_t string_to_name( const char *str ) {
    uint64_t name = 0;
    int i = 0;
    for (; str[i] && i < 12; ++i) {
        // NOTE: char_to_symbol() returns char type, and without this explicit
        // expansion to uint64 type, the compilation fails at the point of usage
        // of string_to_name(), where the usage requires constant (compile time) expression.
        name |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
    }

    // The for-loop encoded up to 60 high bits into uint64 'name' variable,
    // if (strlen(str) > 12) then encode str[12] into the low (remaining)
    // 4 bits of 'name'
    if (i == 12)
        name |= char_to_symbol(str[12]) & 0x0F;
    return name;
}

#define N( X ) string_to_name(#X)
#define NAME( X ) string_to_name(X)

inline std::vector <std::string> string_split(string str, char delimiter) {
      std::vector <string> r;
      string tmpstr;
      while (!str.empty()) {
          int ind = str.find_first_of(delimiter);
          if (ind == -1) {
              r.push_back(str);
              str.clear();
          } else {
              r.push_back(str.substr(0, ind));
              str = str.substr(ind + 1, str.size() - ind - 1);
          }
      }
      return r;

  }