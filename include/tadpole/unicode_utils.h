#ifndef UNICODE_UTILS_H
#define UNICODE_UTILS_H

#include <vector>
#include <ostream>

#include "config.h"
#ifdef HAVE_ICU
#include "unicode/unistr.h"
#include "unicode/ustream.h"
#include "unicode/ucnv.h"
#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/regex.h"

std::string display_unicode( const UnicodeString& );
UnicodeString UTF8ToUnicode( const std::string& );
std::string UnicodeToUTF8( const UnicodeString& );
std::string UTF8_Compose( const std::string& );
std::string UTF8_DeCompose( const std::string& );
UnicodeString Normalize( const UnicodeString&, bool composed );
std::string UTF8ToCoding( const std::string&, const std::string& );
std::string UTF8FromCoding( const std::string&, const std::string& );
UnicodeString UnicodeFrom( const std::string&, const std::string& );
std::string UnicodeTo( const UnicodeString&, const std::string& );
std::string UTF8ToCoding( const std::string&, UConverter * );
std::string UTF8FromCoding( const std::string&, UConverter * );
UnicodeString UnicodeFrom( const std::string&, UConverter * );
std::string UnicodeTo( const UnicodeString&, UConverter * );

class UnicodeRegexMatcher {
 public:
  UnicodeRegexMatcher( const UnicodeString& );
  ~UnicodeRegexMatcher();
  bool match( const UnicodeString& );
  bool search( const UnicodeString& );
  bool search_begin( const UnicodeString& );
  UnicodeString replace_all( const UnicodeString&, const UnicodeString& );
  const UnicodeString get_match( unsigned int ) const;
  int NumOfMatches() const;
  UnicodeString Pattern() const;
  std::string failString;
  void set_trace( bool );
private:
  RegexPattern *pattern;
  RegexMatcher *matcher;
  UnicodeRegexMatcher();
  std::vector<UnicodeString> results;
};
#else
class UnicodeString: public std::string{
public:
  UnicodeString(): std::string(){};
  UnicodeString( const std::string& s ): std::string( s ){};
  UnicodeString( const char *s ): std::string( s ){};
 UnicodeString( const UnicodeString& s, long int b, long int l ):
  std::string( s.substr(b,l) ){};
  void remove( long i, long j ){ erase(i,j); };
  void remove(){ clear(); };
  bool isEmpty() { return empty(); };
  long indexOf( std::string& s ){ return find(s); };
};

inline const UnicodeString UTF8ToUnicode( const std::string& s ){
  return s;
}

inline std::string UnicodeToUTF8( const UnicodeString& u ) {
  return u;
};

#endif

#endif
