/*
  Copyright (c) 2006 - 2008 
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

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <map>

#include "timbl/TimblAPI.h"

#include "tadpole/Tadpole.h" // defines etc.
#include "tadpole/mwu_chunker_mod.h"

using namespace Timbl;
using namespace std;

#define mymap2 multimap<string, vector<string> >

mymap2 MWUs;


namespace mwuChunker {

  string mwuFileName = "";
  string myCFS = "_";
  int mwuDebug = 0;

  int splitTag( const string& tag, string& tagResult, string& modsResult ){
    vector<string> fields;
    int numparts = split_at_first_of( tag, fields, string(myCFS) );
    for( int i=0; i < numparts; ++i ){
      vector<string> parts;
      int num = split_at_first_of( fields[i], parts, "()" );
      if ( num < 1 )
	tagResult += fields[i];
      else {
	tagResult += parts[0];
	if ( num > 1 ){
	  vector<string>fields2;
	  int size = split_at( parts[1], fields2, "," );
	  for ( int j = 0; j < size; ++j ){
	    modsResult += fields2[j];
	    if ( j < size-1 )
	      modsResult += '|';
	  }
	}
	else {
	  modsResult += "__";
	}
	if ( i < numparts-1 ){
	  tagResult += myCFS;
	  modsResult += myCFS;
	}
      }
    }
    return numparts;
  }

  string splitTag( const string& tag ){
    string tagResult;
    string modsResult;
    int num = splitTag( tag, tagResult, modsResult );
    if ( num <= 1 )
      return tagResult + myOFS + tagResult + myOFS + modsResult;
    else
      return "MWU" + myOFS + tagResult + myOFS + modsResult;
  }

  string ana::getTagHead() const {
    string head;
    string tail;
    splitTag( tag, head, tail );
    return head;
  }

  string ana::getTagMods() const {
    string head;
    string tail;
    splitTag( tag, head, tail );
    return tail;
  }

  bool isProper( const string& tag, string& pTag ){
    vector<string> parts;
    pTag = "ERROR";
    int num = split_at( tag, parts, "_" );
    if ( num > 1 ){
      pTag = parts[0];
      return pTag == "SPEC(deeleigen)";
    }
    return false;
  }

  const string ana::formatMWU() const{
    if ( isMWU ){
      string properTag;
      if ( isProper( tag, properTag ) )
	return properTag;
      else
	return "MWU()";
    }
    else
      return tag;
  }

  std::ostream& operator<< (std::ostream& os, const ana& a ){
    os << a.word << myOFS << a.lemma << myOFS << a.morphemes << myOFS
       << a.formatMWU()
       << myOFS << a.parseNum << myOFS << a.parseTag;
    return os;
  }

  void saveAna( std::ostream& os, const vector<ana> &ana ){
    for( size_t i = 0; i < ana.size(); ++i )
      os << i+1 << myOFS << ana[i].getWord() << myOFS 
	 << ana[i].getLemma() << myOFS 
	 << splitTag( ana[i].getTag() ) << myOFS << "0" << myOFS
	 << myCFS << myOFS << myCFS << myOFS << myCFS << endl;
  }

  void readAna( std::istream& is, vector<ana> &ana ){
    string line;
    int cnt=0;
    while ( getline( is, line ) ){
      vector<string> parts;
      int num = split_at( line, parts, " " );
      if ( num > 7 ){
	if ( stringTo<int>( parts[0] ) != cnt+1 ){
	  cerr << "confused! " << endl;
	  cerr << "got line '" << line << "'" << endl;
	  cerr << "expected something like '" << cnt+1 << " " << ana[cnt] << endl;
	}
	ana[cnt].setParseNum( parts[6] );
	ana[cnt].setParseTag( parts[7] );
      }
      ++cnt;
    }
  }

  bool readsettings( const string& cDir, const string& fname){
    ifstream setfile(fname.c_str(), ios::in);
    if(setfile.bad()){
      return false;
    }
    string SetBuffer;
    while( getline( setfile, SetBuffer ) ){
      if ( SetBuffer.empty() )
	continue;
      vector<string>parts;
      size_t num = split_at( SetBuffer, parts, " " );
      if ( num >= 2 ){
	switch ( parts[0][0] ) {
	case 't':
	  mwuFileName = prefix( cDir, parts[1] );
	  break;
 	case 'c':
	  myCFS = parts[1];
	  break;
 	case 'd':
	  mwuDebug = atoi(parts[1].c_str());
	  break;
	case 's':
	  myOFS = parts[1];
	  break;
	default:
	  cerr << "Unknown option in settingsfile, (ignored)\n"
	       << SetBuffer << " ignored." <<endl;
	  break;
	}
      }
      else
	cerr << "Unknown option in settingsfile, (ignored)\n"
	     << SetBuffer << " ignored." <<endl;
    }
    return true;
  }

  bool read_mwus( const string& fname) {
    cerr << "read mwus " + fname << endl;
    ifstream mwufile(fname.c_str(), ios::in);
    if(mwufile.bad()){
      return false;
    }
    string line;
    while( getline( mwufile, line ) ) {
      vector<string> res1, res2; //res1 has mwus and tags, res2 has ind. words
      split_at(line, res1, " ");
      split_at(res1[0], res2, "_");
      string key = res2[0];
      res2.erase(res2.begin());
      MWUs.insert( make_pair( key, res2 ) );
    }
    return true;
  }

  void init( const string& cDir, const string& fname) {
    cerr << "initiating mwuChunker ... " << endl;
    mwuDebug = tpDebug;
    if (!readsettings( cDir, fname)) {
      cerr << "Cannot read mwuChunker settingsfile " << fname << endl;
      exit(1);
    }
    if (!read_mwus(mwuFileName)) {
      cerr << "Cannot read mwu file " << mwuFileName << endl;
      exit(1);
    }
    return;
  }

  ostream &showResults( ostream& os,
			const vector<mwuChunker::ana>& ana ){
    for( size_t i = 0; i < ana.size(); ++i )
      os << i+1 << "\t" << ana[i] << endl;
    return os;
  }

  void Classify(vector<string> &words_in, vector<ana> &cur_ana) {
    if (mwuDebug)
      cout << "\nStarting mwu Classify\n";
    mymap2::iterator best_match;
    size_t matchLength = 0;
    size_t max = words_in.size();

    // add all current sequences of SPEC(deeleigen) words to MWUs
    for( size_t i=0; i < max-1; ++i ) {
      if ( cur_ana[i].getTag() == "SPEC(deeleigen)" &&
	   cur_ana[i+1].getTag() == "SPEC(deeleigen)" ) {
	vector<string> newmwu;
	while ( i < max &&
		cur_ana[i].getTag() == "SPEC(deeleigen)" ) {
	  newmwu.push_back(words_in[i]);
	  i++;
	}
	string key = newmwu[0];
	newmwu.erase( newmwu.begin() );
	MWUs.insert( make_pair(key, newmwu) );
      }
    }
    size_t i; 
    for( i = 0; i < max; i++) {
      if (mwuDebug)
	cout << "checking word[" << i <<"]: " << words_in[i] << endl;
      size_t no_matches =  MWUs.count(words_in[i]);
      if ( no_matches == 0 )
	continue;
      pair<mymap2::iterator, mymap2::iterator> matches = MWUs.equal_range(words_in[i]);
      if ( matches.first != MWUs.end() ) {
	//match
	mymap2::iterator current_match = matches.first;
	if ( mwuDebug ) {
	  cout << "MWU: match found!\t" << current_match->first << endl;
	}
	while(current_match != matches.second && current_match != MWUs.end()) {
	  vector<string> match = current_match->second;
	  size_t max_match = match.size();
	  size_t j = 0;
	  if (mwuDebug)
	    cout << "checking " << max_match << " matches\n";
	  for(; i + j + 1 < max && j < max_match; j++) {
	    if ( match[j] != words_in[i+j+1] ) {
	      if (mwuDebug)
		cout << "match " << j <<" (" << match[j] 
		     << ") doesn't match with word " << i+ j + 1
		     << " (" << words_in[i+j + 1] <<")\n";
	      // mismatch in jth word of current mwu
	      break;
	    }
	    else if (mwuDebug)
	      cout << " matched " <<  words_in[i+j+1] << " j=" << j << endl;

	  }
	  if (j == max_match){
	    // a match. remember this!
	    if ( j > matchLength ){
	      best_match = current_match;
	      matchLength = j;
	    }
	  }
	  ++current_match;
	} //while(focus1 <= focus2)
	if(mwuDebug){
	  if (matchLength >0 ) {
	    cout << "MWU: found match starting with " << (*best_match).first << endl;
	  } else {
	    cout <<"MWU: no match\n";
	  }
	}
	// we found a matching mwu, break out of loop thru sentence, 
	// do useful stuff, and recurse to find more mwus
	if (matchLength > 0 )
	  break;
      } //match found
      else { 
	if(mwuDebug)
	  cout <<"MWU:check: no match\n";
      }
    } //for (i < max)
    if (matchLength > 0 ) {
      //concat
      if (mwuDebug)
	cout << "mwu found, processing\n";
      for ( size_t j = 1; j <= matchLength; ++j) {
	if (mwuDebug)
	  cout << "concat " << words_in[i+j] << endl;
	words_in[i] += myCFS;
	words_in[i] += words_in[i + j];
	// and do the same for cur_ana elems (Word, Tag, Lemma, Morph)
	cur_ana[i].append( myCFS, cur_ana[i+j] );
	if (mwuDebug){
	  cout << "concat tag " << cur_ana[i+j].getTag() << endl;
	  cout << "gives : " << cur_ana[i].getTag() << endl;
	}
	
      }
      // now erase words_in[i+1 .. i+matchLenght]
      vector<string>::iterator tmp1 = words_in.begin() + i;
      vector<ana>::iterator anatmp1 = cur_ana.begin() + i;
      vector<string>::iterator tmp2 = ++tmp1 + matchLength;
      vector<ana>::iterator anatmp2 = ++anatmp1 + matchLength;
      words_in.erase(tmp1, tmp2);
      cur_ana.erase(anatmp1, anatmp2);
      if (mwuDebug){
	cout << "tussenstand" << endl;
	showResults( cout, cur_ana );
      }      
      Classify(words_in, cur_ana);
    } //if (matchLength)
    return;
  } // //Classify

} //namespace
