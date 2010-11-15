#include <cstdio>
#include <stdexcept>
#include <ostream>
#include <iostream>
#include <string>
#include "tadpole/unicode_utils.h"

#ifdef HAVE_ICU
using std::string;
using std::ostream;

const string itohs( const int i ){
  char tmp[30];
  sprintf( tmp, "%X", i );
  return tmp;
}

string printUChars( const UChar *uch  = 0,
		    int32_t     len   = -1 ){
  string result;
  int32_t i;

  if( (len == -1) && (uch) ) {
    len = u_strlen(uch);
  }

  for( i = 0; i <len; ++i) {
    result += "\\u";
    int kar = (int)uch[i];
    if ( kar < 0xFF )
      result += "00";
    result += itohs( kar );
  }
  return result;
}

string display_unicode( const UnicodeString& us ){
  return printUChars( us.getBuffer(), us.length() );
}

UnicodeString UTF8ToUnicode( const string& s ){
  return UnicodeString( s.c_str(), s.length(), "UTF-8" );
}

string UnicodeToUTF8( const UnicodeString& s ){
  string result;
  int len = s.length();
  if ( len > 0 ){
    char *buf = new char[len*6+1];
    s.extract( 0, len, buf, len*6, "UTF-8" );
    result = buf;
    delete [] buf;
  }
  return result;
}

string UnicodeTo( const UnicodeString& sin, const string& code ){
  string result;
  UnicodeString s = Normalize( sin, true );
  int len = s.length();
  if ( len > 0 ){
    char *buf = new char[len*6+1];
    s.extract( 0, len, buf, len*6, code.c_str() );
    result = buf;
    delete [] buf;
  }
  return result;
}

string UnicodeTo( const UnicodeString& sin, UConverter *cnv ){
  string result;
  UnicodeString s = Normalize( sin, true );
  int len = s.length();
  if ( len > 0 ){
    char *buf = new char[len*6+1];
    UErrorCode status=U_ZERO_ERROR;
    s.extract( buf, len*6, cnv, status );
    if (U_FAILURE(status)){
      throw std::invalid_argument("Converter");
    }
    result = buf;
    delete [] buf;
  }
  return result;
}

UnicodeString UnicodeFrom( const string& s, const string& code ){
  UnicodeString us = UnicodeString( s.c_str(), s.length(), code.c_str() );
  UnicodeString result = Normalize( us, false );
  return result;
}


UnicodeString UnicodeFrom( const string& s, UConverter *cnv ){
  UErrorCode status=U_ZERO_ERROR;
  UnicodeString us = UnicodeString( s.c_str(), s.length(), cnv, status );
  if (U_FAILURE(status)){
    throw std::invalid_argument("Converter");
  }
  UnicodeString result = Normalize( us, false );
  return result;
}

string UTF8ToCoding( const string& s, const string& code ){
  UnicodeString us = UTF8ToUnicode( s );
  return UnicodeTo( us, code );
}

string UTF8ToCoding( const string& s, UConverter *cnv ){
  UnicodeString us = UTF8ToUnicode( s );
  return UnicodeTo( us, cnv );
}

string UTF8FromCoding( const string& s, const string& code ){
  UnicodeString us = UnicodeString( s.c_str(), s.length(), code.c_str() );
  UnicodeString result = Normalize( us, false );
  return UnicodeToUTF8( result );
}

string UTF8FromCoding( const string& s, UConverter *cnv ){
  UErrorCode status=U_ZERO_ERROR;
  UnicodeString us = UnicodeString( s.c_str(), s.length(), cnv, status );
  if (U_FAILURE(status)){
    throw std::invalid_argument("Converter");
  }
  UnicodeString result = Normalize( us, false );
  return UnicodeToUTF8( result );
}

UnicodeString Normalize( const UnicodeString& s, bool composed ){
  UnicodeString r;
  UErrorCode status=U_ZERO_ERROR;
  Normalizer::normalize( s, (composed?UNORM_NFC:UNORM_NFD), 0, r, status );
  if (U_FAILURE(status)){
    throw std::invalid_argument("Normalizer");
  }
  return r;
}

string UTF8_Compose( const string& s ){
  UnicodeString us = UTF8ToUnicode( s );
  UnicodeString result = Normalize( us, true );
  return UnicodeToUTF8(result);
}

string UTF8_DeCompose( const string& s ){
  UnicodeString us = UTF8ToUnicode( s );
  UnicodeString result = Normalize( us, false );
  return UnicodeToUTF8(result);
}

UnicodeRegexMatcher::UnicodeRegexMatcher( const UnicodeString& pat ){
  failString = "";
  matcher = NULL;
  UErrorCode u_stat = U_ZERO_ERROR;
  UParseError errorInfo;
  pattern = RegexPattern::compile( pat, 0, errorInfo, u_stat );
  if ( U_FAILURE(u_stat) ){
    failString = "Invalid regular expression '" + UnicodeToUTF8(pat) +
      "', no compiling possible";
    throw std::invalid_argument(failString);
  }
  else {
    matcher = pattern->matcher( u_stat );
    if (U_FAILURE(u_stat)){
      failString = "unable to create PatterMatcher with pattern '" + 
	UnicodeToUTF8(pat) + "'";
      throw std::invalid_argument(failString);
    }
  }
}

UnicodeRegexMatcher::~UnicodeRegexMatcher(){
  delete pattern;
  delete matcher;
}

void UnicodeRegexMatcher::set_trace( bool state ) {
  matcher->setTrace( state );
}

UnicodeString UnicodeRegexMatcher::Pattern() const{
  return pattern->pattern();
}

bool UnicodeRegexMatcher::match( const UnicodeString& line ){
  UErrorCode u_stat = U_ZERO_ERROR;
  UnicodeString res;
  if ( matcher ){
    matcher->reset( line );
    if ( matcher->matches(u_stat) ){
      results.resize( matcher->groupCount()+1, "" );
      results[0] = line;
      for ( int i=1; i <= matcher->groupCount(); ++i ){
	u_stat = U_ZERO_ERROR;
	if ( matcher->start( i, u_stat ) < 0 ){
	  //	  std::cerr << "match NEXT" << std::endl;
	  continue;
	}
	res = matcher->group( i, u_stat );
	if (!U_FAILURE(u_stat)){
	  results[i] = res;
	}
	else
	  break;
      }
      return true;
    }
  }
  results.clear();
  return false;
}

bool UnicodeRegexMatcher::search( const UnicodeString& line ){
  UErrorCode u_stat = U_ZERO_ERROR;
  results.clear();
  UnicodeString res;
  if ( matcher ){
    matcher->reset( line );
    if ( matcher->find() ){
      results.resize( matcher->groupCount()+1, "" );
      results[0] = line;
      for ( int i=1; i <= matcher->groupCount(); ++i ){
	u_stat = U_ZERO_ERROR;
	if ( matcher->start( i, u_stat ) < 0 ){
	  //	  std::cerr << i << "search NEXT" << std::endl;
	  continue;
	}
	res = matcher->group( i, u_stat );
	if (!U_FAILURE(u_stat)){
	  results[i] = res;
	}
	else
	  break;
      }
      return true;
    }
  }
  results.clear();
  return false;
}

bool UnicodeRegexMatcher::search_begin( const UnicodeString& line ){
  UErrorCode u_stat = U_ZERO_ERROR;
  UnicodeString res;
  if ( matcher ){
    matcher->reset( line );
    if ( matcher->lookingAt(u_stat) ){
      results.resize( matcher->groupCount()+1, "" );
      results[0] = line;
      for ( int i=1; i <= matcher->groupCount(); ++i ){
	u_stat = U_ZERO_ERROR;
	if ( matcher->start( i, u_stat ) < 0 ){
	  // std::cerr << "search_b NEXT" << std::endl;
	  continue;
	}
	res = matcher->group( i, u_stat );
	if (!U_FAILURE(u_stat)){
	  results[i] = res;
	}
	else
	  break;
      }
      return true;
    }
  }
  results.clear();
  return false;
}

const UnicodeString UnicodeRegexMatcher::get_match( unsigned int n ) const{
  if ( n < results.size() )
    return results[n];
  else
    return "";
}

int UnicodeRegexMatcher::NumOfMatches() const {
  if ( results.size() > 0 )
    return results.size()-1;
  else
    return 0;
}

UnicodeString UnicodeRegexMatcher::replace_all( const UnicodeString& line,
					      const UnicodeString& replace ){
  UErrorCode u_stat = U_ZERO_ERROR;
  UnicodeString result;
  if ( matcher ){
    matcher->reset( line );
    result = matcher->replaceAll( replace, u_stat );
    if (U_FAILURE(u_stat)){
      result = "";
    }
  }
  return result;
}
#endif

