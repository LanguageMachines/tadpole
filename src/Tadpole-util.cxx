/*
  Copyright (c) 2006 - 2009
  Tilburg University

  This file is part of Tadpole.

  Tadpole is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by  
  the Free Software Foundation; either version 3 of the License, or  
  (at your option) any later version.  

  Tadpole is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of  
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
  GNU General Public License for more details.  

  You should have received a copy of the GNU General Public License  
  along with this program.  If not, see <http://www.gnu.org/licenses/>.  

  For more information and updates, see:                             
      http://ilk.uvt.nl/tadpole                                          
*/                                                                   

/*

contains help-functions for Tadpole, such as
-   web header and footer generator
-   ???

*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "config.h"
#include "tadpole/Tadpole.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_BOOST_FILESYSTEM_PATH_HPP
#include "boost/version.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#else
#include <dirent.h>
#endif

using namespace std;

string prefix( const string& path, const string& fn ){
  if ( fn.find( "/" ) == string::npos ){
    // only append prefix when NO path is specified
    return path + fn;
  }
  return fn;
}

#ifdef HAVE_BOOST_FILESYSTEM_PATH_HPP
namespace fs =  boost::filesystem;

bool existsDir( const string& dirName ){
  return fs::exists( fs::path(dirName) );
}
void getFileNames( const string& dirName, set<string>& fileNames ){
  //  cerr << "get file Names " << dirName << endl;
  fs::directory_iterator end_itr; // default construction yields past-the-end
  for ( fs::directory_iterator itr( dirName );
        itr != end_itr;
        ++itr ) {
#if BOOST_VERSION < 103301
#error BOOST: VERSION TOO OLD 
#else
    //    cerr << "found leaf " << itr->leaf() << endl;
# if BOOST_VERSION <= 103400
    if ( fs::exists( *itr ) && !fs::is_directory( *itr ) )
      fileNames.insert( itr->leaf() );
# else
    if ( is_regular( itr->status() ) )
      fileNames.insert( itr->leaf() );
# endif    
#endif
  }
  
}

#else
bool existsDir( const string& dirName ){
  bool result = false;
  DIR *dir = opendir( dirName.c_str() );
  if ( dir ){
    result = (closedir( dir ) == 0);
  }
  return result;
}

void getFileNames( const string& dirName, set<string>& fileNames ){
  DIR *dir = opendir( dirName.c_str() );
  if ( !dir )
    return;
  else {
    struct stat sb;
    struct dirent *entry = readdir( dir );
    while ( entry ){
      string fullName = dirName + "/" + entry->d_name;
      if ( stat( fullName.c_str(), &sb ) >= 0 ){
	if ( (sb.st_mode & S_IFMT) == S_IFREG )
	  fileNames.insert( entry->d_name );
      }
      entry = readdir( dir );
    }
  }
}

#endif

//BJ: to decapitalize 1st letter
void decap( string &w, const string &t) {
  if (tpDebug)
    cout << "Decapping " << w << " with tag " << t << endl;
  size_t pos = t.find("eigen");
  if ( pos != string::npos ) 
    // don't decap Eigennamen
    return;
  //pos = t.find("ADJ");
  //if ( pos != string::npos ) 
  //  return;
  w[0] = tolower( w[0] );
  if (tpDebug)
    cout << "decapped: " << w << endl;
}

void decap( UnicodeString &w, const string &t ) {
  if (tpDebug)
    cout << "Decapping " << UnicodeToUTF8(w) << " with tag " << t;
  size_t pos = t.find("eigen");
  if ( pos != string::npos ) 
    // don't decap Eigennamen
    return;
#ifdef HAVE_ICU
  w.replace( 0, 1, u_tolower( w[0] ) );
#else
  char tmp = tolower(w[0]);
  w.replace( 0, 1, &tmp );
#endif
  if (tpDebug)
    cout << " to : " << UnicodeToUTF8(w) << endl;
}

string tokenize( const string& infilename) {
  string tokenizedfilename = infilename + ".tok";
  string lang = "ned";
  string command = string( BIN_PATH) + "/TPtokenize " + lang + " < " + infilename + " > " + tokenizedfilename + "\n";
  if ( system(command.c_str()) != 0 ){
    cerr << "execution of " << command << " failed. We go on" << endl;
  };


  return tokenizedfilename;
}

string linetokenize( const string& infilename) {
  string linetokenizedfilename = infilename + ".lin";
  ifstream bron( infilename.c_str() );
  ofstream doel(linetokenizedfilename.c_str() );
  while ( bron ) {
    string readword;
    bron >> readword;
    if ( readword == "<utt>" )
      doel << endl;
    else {
      if ( readword.find('/') != string::npos ){
	for ( size_t i=0; i < readword.length(); ++i )
	  if ( readword[i] == '/' )
	    readword[i]='-';
      }
      doel << readword << ' ';
    }
  }
  return linetokenizedfilename;
}


void showTimeSpan( ostream& os, const string& line, 
		   struct timeval& timeBefore ){
  os << line << " took:" << timeBefore.tv_sec
     << " seconds and " << timeBefore.tv_usec 
     << " microseconds" << endl;
}
  
void addTimeDiff( struct timeval& time, 
		  struct timeval& start, 
		  struct timeval& end ){
  long usecs = (time.tv_sec + end.tv_sec - start.tv_sec) * 1000000 
    + time.tv_usec + end.tv_usec - start.tv_usec;
  ldiv_t div = ldiv( usecs, 1000000 );
  time.tv_sec = div.quot;
  time.tv_usec = div.rem;
}

