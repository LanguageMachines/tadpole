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
#include <string>
#include <cstdlib>

#include "timbl/TimblAPI.h"
#include "tadpole/Tadpole.h"
#include "tadpole/mblem_mod.h"

using namespace std;
using namespace Timbl;

namespace myMblem 
{
  string lemprefix="mblem/mblem";
  string lexname, transtablename,lexibase;
  string opts_lexibase;
  size_t history = 20;
  int mblemDebug=0;

  string punctuation = "?...,:;\\'`(){}[]%#+-_=/!";
  TimblAPI *myLex;
  map <string,string> classes;
  vector<string> lookuplemma, lookuptag;
  int nrlookup;

  void read_transtable() {
    ifstream bron( transtablename.c_str() );
    if ( !bron ) {
      cerr << "translation table file '" << transtablename 
	   << "' appears to be missing." << endl;
      exit(1);
    }
    while( bron ){
      string clas;
      string classcode;
      bron >> clas;
      bron >> ws;
      bron >> classcode;
      if ( classes.find( classcode ) == classes.end() )
	// stupid HACK to only accept first occurence
	// multiple occurences is a NO NO i think
	classes[classcode] = clas;
      bron >> ws;
    }
    return;
  }

  void create_MBlem_defaults() 
  {
    lexname = lemprefix + ".lex";
    transtablename = lemprefix + ".transtable";
    lexibase = lemprefix + ".tree";
    return;
  }

  bool readsettings( const string& dir, const string& fname) {
    ifstream setFile( fname.c_str(), ios::in);
    if( !setFile )
      return false;
    string buf;
    while( getline( setFile, buf ) ){
      vector<string> fields;
      int i = split_at( buf, fields, " " );
      if ( i < 2 ){
	cerr << "No option in settingsfile, (ignored)\n"
	     << "offensive line: '" << buf << "'." <<endl;
	continue;
      }
      switch ( fields[0][0] ) {
	//prefix for lex, transtable, and instancebase
	case 'd':
	  mblemDebug = stringTo<int>(fields[1]);
	  break;
	case 'p':
	  lemprefix = prefix( dir, fields[1] );
	  break;
	case 'O':
	  opts_lexibase = fields[1];
	  break;
      default:
	cerr << "Unknown option in settingsfile, (ignored)\n"
	     << "offensive line: '" << buf << "'." <<endl;
	break;
      }
    }
    return true;
  }
  

  void init( const string& cdir, const string& fname) {
    cerr << "Initiating lemmatizer..." << endl;
    opts_lexibase = "-a1";
    mblemDebug = tpDebug;
    if (!readsettings( cdir, fname)) 
      {
	cerr << "Cannot read MBLem settingsfile " << fname << endl;
	exit(1);
      }
    //make it silent
    opts_lexibase += " +vs -vf";	    
    create_MBlem_defaults();
    read_transtable();
    //Read in (igtree) data
    myLex = new TimblAPI(opts_lexibase);
    myLex->GetInstanceBase(lexibase);
    return;
  }

  string make_instance( const string& in ) {
    if (mblemDebug)
      cout << "making instance from: " << in << endl;
    string instance = "";
    size_t length = in.length();
    size_t j;
    for ( size_t i=0; i < history; i++) {
	j = length - history + i;
	if (( i < history - length ) &&
	    (length<history))
	  instance += "= ";
	//else if (in[j] == ',')
	//  instance += "C ";
	else {
	  instance += tolower(in[j]);
	  instance += ' ';
	}
    }
    instance += "?";
    if (mblemDebug)
      cout << "inst: " << instance << endl;

    return instance;
  }

  /* BJ: Antal uses the tagger output more or less directly, BUT
     prefixes a sentencenr and a wordnr, and split word/tag pair,
     as well as postfixes nr of slashes, like this:
     #sent #word word tag #slashes

     we get something like:
     word//?tag(spec1,spec2,...)
  */

  string Classify( const string& in) {
    string  res,word,tag;
    int     nrslashes=0;

    nrlookup=0;
    lookuplemma.clear();
    lookuptag.clear();

    // BJ: 1st order of business: 
    //     split word and tag, and store num of slashes
    
    if (mblemDebug)
      cout << "mblem::Classify starting with " << in << endl;

    size_t pos = in.find("//");
    if ( pos != string::npos ) {
      // double slash: unknown word
      word = in.substr(0, pos);
      tag = in.substr(pos+2);
      nrslashes = 2;
    } 
    else {
	pos = in.find("/");
	if ( pos != string::npos ) {
	  // single slash: known word
	  word = in.substr(0, pos);
	  tag = in.substr(pos+1);
	  nrslashes = 1;
	} 
	else {
	  cerr << "no word/tag pair in this line: " << in << endl;
	  return "oops";
	}
      }
    if (mblemDebug) 
      cout << "word: " << word << "\ttag: " << tag << "\t#slashes: " << nrslashes<< endl;
    
    // BJ: checking specials 1st
    bool special=false;
    //punctuaton
    pos = punctuation.find(word);
    if ( pos != string::npos ){
      //it's punctuation
      //     word = in;
      res = word;
      res += " ";
      res += "LET()";
      res += " ";
      nrlookup = 1;
      special=true;
    }
    
    // proper names
    pos = tag.find("eigen");
    if ( pos != string::npos ){
      res = word;
      res += " ";
      res += "SPEC(deeleigen)";
      res += " ";
      nrlookup = 1;
      special=true;
    }
    
    // if it ain't special, call mblem
    
    if (special) {
      lookuptag.push_back( tag );
      lookuplemma.push_back( word );
      if (mblemDebug)
	cout << "special case, lemma = word, no MBLEM" << endl;
    }
    else {
      // decapitalize 1st letter, when appropriate  
      decap(word, tag);
      
      string inst = make_instance(word);
      if (mblemDebug) 
	cout << "inst: " << inst << endl;
      myLex->Classify(inst, res);
      if (mblemDebug)
	cout << "class: " << res << endl;
      //postprocess
      // 1st find all alternatives
      string part = "";
      pos = res.find("|");
      if ( pos == string::npos ){
	pos = res.length();
	if (mblemDebug)
	  cout << "no alternatives found\npos = " << pos << endl;
      }
      bool more = true;
      while (more) {
	string insstr;
	string delstr;
	string prefix;
	tag.clear();
	part = res.substr(0, pos);
	res.erase(0, pos + 1);
	if (mblemDebug)
	  cout <<"part = " << part << " res = " << res << endl;
	size_t lpos = part.find("+");
	if ( lpos != string::npos )
	  tag = part.substr(0, lpos);
	else 
	  tag = part;
	map<string,string>::const_iterator it = classes.find(tag);
	if ( it != classes.end() )
	  tag = it->second;
	size_t  pl = part.length();
	lpos++;
	while(lpos < pl) {
	  switch( part[lpos] ) {
	  case 'P': {
	    if (part[lpos-1] =='+') {
	      lpos++;
	      size_t tmppos = part.find("+", lpos);
	      if ( tmppos != string::npos )
		prefix = part.substr(lpos, tmppos - lpos);
	      else 
		prefix = part.substr(lpos);
	      if (mblemDebug)
		cout << "prefix=" << prefix << endl;
	    }
	    break;
	  }
	  case 'D': {
	    if (part[lpos-1] =='+') {
	      lpos++;
	      size_t tmppos = part.find("+", lpos);
	      if ( tmppos != string::npos )
		delstr = part.substr(lpos, tmppos - lpos);
	      else 
		delstr = part.substr(lpos);
	      if (mblemDebug)
		cout << "delstr=" << delstr << endl;
	    }
	    break;
	  }
	  case 'I': {
	    if (part[lpos-1] =='+') {
	      lpos++;
	      size_t tmppos = part.find("+", lpos);
	      if ( tmppos != string::npos )
		insstr = part.substr(lpos, tmppos - lpos);
	      else 
		insstr = part.substr(lpos);
	      if (mblemDebug)
		cout << "insstr=" << insstr << endl;
	    }
	    break;
	  }
	  default:
	    break;
	  }
	  lpos++;
	} // while lpos < pl
	
	if (mblemDebug)
	  cout << "part: " << part << " split up in: " << endl;
	
	string lemma = "";
	if (mblemDebug)
	  cout << "pre-prefix word: '" << word << "' prefix: '"
	       << prefix << "'" << endl;
	
	size_t  prefixpos = 0;
	if (!prefix.empty()) {
	  prefixpos = word.find(prefix);
	  if (mblemDebug)
	    cout << "prefixpos = " << prefixpos << endl;
	} // if !prefix.empty()

	// repair cases where there's actually not a prefix present
	if (prefixpos > word.length()-2) {
	  prefixpos = 0;
	  prefix.clear();
	}

	if (mblemDebug)
	  cout << "prefixpos = " << prefixpos << endl;
	if (prefixpos > 0) 
	  lemma += word.substr(0,prefixpos);
	size_t i = prefixpos + prefix.length();
	if (mblemDebug)
	  cout << "post prefix != 0 word: "<< word 
	       << " lemma: " << lemma 
	       << " prefix: " << prefix
	       << " insstr: " << insstr
	       << " delstr: " << delstr
	       << " l_delstr=" << delstr.length()
	       << " i=" << i
	       << " l_word=" << word.length()
	       << endl;

	// prefixpos is absurd here
	//
	// Ko: 13-5-2008 PROBLEM
	// This code doesn't seem to do what the comment suggests
	// what is the intention?
	if (word.length() - delstr.length())
	  lemma += word.substr(i, (word.length() - delstr.length() -i ) );
	else // skip delstr, unless delstr is exactly the word
	  if ( word.length() < delstr.length() ||
	       ( word.length()-delstr.length() &&
		 insstr.empty() ))
	    lemma += word.substr(i, (word.length() -i ) );
	// end PROBLEM
	if (!insstr.empty()) 
	  lemma += insstr;
	if (mblemDebug)
	  cout << "appending lemma " << lemma << " and tag " << tag << endl;
	lookuptag.push_back( tag );
	lookuplemma.push_back( lemma );
	
	if (mblemDebug)
	  cout << "Size after " << lookuplemma.size() << " and " << lookuptag.size() << endl;
	nrlookup++;
	
	// wrap up & prep for next iteration      
	pos = res.find("|");
	if ( pos != string::npos ) {
	  more = true;
	} 
	else {
	  pos = res.length();
	  if ( pos > 0 )
	    more = true;
	  else
	    more = false;
	}
      } // while)\(more)
    }
    if (mblemDebug) {
      cout << "stored lemma and tag options: " << lookuplemma.size() << " lemma's and " << lookuptag.size() << " tags:\n";
      for(vector<string>::iterator tmp = lookuplemma.begin(); tmp != lookuplemma.end(); tmp++) 
	cout << "lemma alt: " << (*tmp) << endl;
      for(vector<string>::iterator tmp = lookuptag.begin(); tmp != lookuptag.end(); tmp++) 
	cout << "tag alt: " << (*tmp) << endl;
      
      cout << "\n\n";
    }
    
    vector<string>::iterator lemmatmp = lookuplemma.begin();
    vector<string>::iterator tagtmp = lookuptag.begin();
    while (lemmatmp != lookuplemma.end() && tagtmp != lookuptag.end()) {
      res += *lemmatmp + "/" + *tagtmp + " ";
      ++lemmatmp;
      ++tagtmp;
    }
    res.erase(res.end() -1);
    return res;
  }
  
}
