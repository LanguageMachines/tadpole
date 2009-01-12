/*
  Copyright (c) 2006 - 2009
  Tilburg University

  A Tagger-Lemmatizer-Morphological-Analyzer-Dependency-Parser for Dutch
  Version 0.04
 
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

#include <cstdlib>
#include <sys/time.h>
#include <ctime>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include "timbl/TimblAPI.h"
#include "mbt/MbtAPI.h"
// individual module headers

#include "config.h"
#include "tadpole/Tadpole.h"
#include "tadpole/mbma_mod.h"
#include "tadpole/mblem_mod.h"
#include "tadpole/mwu_chunker_mod.h"
#include "tadpole/Parser.h"

using namespace std;
using Mbma::MBMAana;

string TestFileName;
string ProgName;
string parserTmpFile = "tadpole.ana";
string TokenizedTestFileName = "";
string LineTokenizedTestFileName = "";
int num_modules = 0;
int mode = 0; // 0 = cmdline, 1 = webdemo
int tpDebug = 0; //0 for none, more for more output
string sep = " "; // "&= " for cgi 
string myOFS = "\t"; // \t
string skipMods = "";
bool doTok = true;
bool doMwu = true;
bool doParse = true;
DemoOptions ** ModOpts;

/* assumptions:
   each components gets its own configfile per cmdline options
   another possibility is the U: en K: style option seperation

   each component has a read_settings_file in own namespace,
        which sets variables global to that namespace
	to be used in the init() for that namespace

   Further, 
   each component provides a Test(File) which writes output to 
        File.<componentname>_out
   and a Classify(Instance) which produces relevant output 
        as a string return 
	or somehwere else, 
   to be determined later, 
   after pre- and postprocessing raw classification data

*/

// BJ: map to match cgn tags with mbma codes

#define mymap map< const string, const string >

mymap TAGconv;

// BJ: dirty hack with fixed everything to read in tag correspondences
void init_cgn( const string& dir ) {
  string line;
  size_t num =0;
  vector<string> tmp;
  string fn = dir + "cgntags.main";
  ifstream tc( fn.c_str() );
  if ( tc ){
    while( getline( tc, line) ) {
      tmp.clear();
      num = split_at(line, tmp, " ");
      if ( num < 2 ){
	cerr << "splitting '" << line << "' failed" << endl;
	throw ( runtime_error("panic") );
      }
      TAGconv.insert( make_pair( tmp[0], tmp[1] ) );
    }
  }
  else
    throw ( runtime_error( "unable to open:" + fn ) );
  fn = dir + "cgntags.sub";
  ifstream tc1( fn.c_str() );
  if ( tc1 ){
    while( getline(tc1, line) ) {
      tmp.clear();
      num = split_at(line, tmp, " ");
      TAGconv.insert( make_pair( tmp[0], tmp[1] ) );
    }
  }
  else
    throw ( runtime_error( "unable to open:" + fn ) );
}

void usage( const string& progname ) {
  cout << progname << " v." << VERSION << endl << "Options:\n";
  cout << "\t-d <dirName> path to config dir (default ./config)\n"
       << "\t-T <taggerconfigfile> (uses Mbt-style settings file)\n"
       << "\t-M <MBMAconfigfile> (morphological analysis)\n"
       <<"\t\t accepts:\n"
       <<"\t\tt <treefile>\n"
       <<"\t\tm <mode>\n"
       << "\t-L <MBlemconfigfile> (lemmatizer)\n"
       <<"\t\taccepts:\n"
       <<"\t\tp <prefix> (for filenames)\n"
       <<"\t\tO <timbl options>\n"
       << "\t-U <mwuconfigfile> (multiwordchunker)\n"
       <<"\t\taccepts:\n"
       <<"\t\tt <mwu unit file>\n"
       <<"\t\tc <connect_char> (char between tokens in a mwu)\n"
       << "\t-t <testfile>\n"
//       << "\t-m <mode> (default and only option = 0)\n"
       << "\t-s <output field separator> (default tab)\n"
       << "\t-d <debug level> (for more verbosity)\n";

}

//**** stuff to process commandline options *********************************

// used to convert relative paths to absolute paths

/**
 * If you do 'Mbt -s some-path/xxx.settingsfile' Timbl can not find the 
 * tree files.
 *
 * Because the settings file can contain relative paths for files these 
 * paths are converted to absolute paths. 
 * The relative paths are taken relative to the pos ition of the settings
 * file, so the path of the settings file is prefixed to the realtive path.
 *
 * Paths that do not begin with '/' and do not have as second character ':'
 *      (C: or X: in windows cygwin) are considered to be relative paths
 */

string prefixWithAbsolutePath( const string& fileName, const string& path ) {
  string result;
  cout << "in:" << fileName << endl;
  if (fileName[0] != '/' && fileName[1] != ':') {
    result = path + fileName;
  }
  else
    result = fileName;
  cout << "uit:" << result << endl;
  return result;
}

// BJ: one possibility for parsing modoptions is passing 
//a class with the config data to the module init
bool readsettings( const string& fname, const string& path, DemoOptions *DOpts ) {
  ifstream setFile( fname.c_str() , ios::in);
  if( !setFile )
    return false;

  string inputLine;
  char tmp[MAX_NAMELEN];
  while( getline( setFile, inputLine )) {
    if ( inputLine.empty() )
      continue;
    switch ( inputLine[0]) {
    case 'n':
      sscanf( inputLine.c_str(), "n %s", tmp);
      //      cout << "name supplied: " << tmp << endl;
      DOpts->setName(tmp);
      //      cout << "name stored: " << DOpts->getName();
      break;
    case 't': {
      sscanf( inputLine.c_str(), "t %s", tmp);
      string fn = prefixWithAbsolutePath( tmp, path );
      DOpts->setTrainFile(fn);
    }
      break;
    case 'u': {
      sscanf( inputLine.c_str(), "u %s", tmp);
      string fn = prefixWithAbsolutePath( tmp, path );
      DOpts->setTreeFile(fn);
    }
      break;
    case 'o':  // Option string for Timbl
      DOpts->setOptStr( inputLine.c_str()+1 );
      break;
    default:
      cerr << "Unknown option in settingsfile, (ignored)\n"
	   << inputLine << " ignored." <<endl;
      break;
    }
  }
  return true;
}

static MbtAPI *tagger;

void parse_args( TimblOpts& Opts ) {
  string value;
  bool mood;
  // We expect a config file for each module
  // and therefore each module has a specific cmdline switch for 
  // the module config file
  // The switch is the first CAPITAL of the module NAME
  string c_dirName = string(SYSCONF_PATH) + '/' + PACKAGE;
  string t_fileName = "cgn-wotan-dcoi.mbt.settings";
  string l_fileName = "lconfig";
  string m_fileName = "mconfig";
  string u_fileName = "mwuconfig";
  string p_fileName = "parserconfig";
  
   // is a config dir specified?
  if ( Opts.Find( 'c',   value, mood ) ) {
    c_dirName = value;
    Opts.Delete( 'c' );
  };
  if ( c_dirName[c_dirName.length()-1] != '/' )
    c_dirName += "/";
  // debug opts
  if ( Opts.Find ('d', value, mood)) {
    tpDebug = atoi(value.c_str());
    Opts.Delete('d');
  };
  if ( Opts.Find( "skip", value, mood)) {
    string skip = value;
    if ( skip.find('t') != string::npos )
      doTok = false;
    if ( skip.find('m') != string::npos )
      doMwu = false;
    if ( skip.find('p') != string::npos )
      doParse = false;
    Opts.Delete("skip");
  };
   // TAGGER opts
  if ( Opts.Find( 'T',   value, mood ) ) {
    t_fileName = prefix( c_dirName, value );
    Opts.Delete( 'T' );
  }
  else
    t_fileName = prefix( c_dirName, t_fileName );

  //LEMMATIZER opts
  if ( Opts.Find( 'L',   value, mood ) ) {
    l_fileName = prefix( c_dirName, value );
    Opts.Delete( 'L' );
  }
  else
    l_fileName = prefix( c_dirName, l_fileName );
  
  // MBMA opts
  if ( Opts.Find( 'M',   value, mood ) ) {
    m_fileName = prefix( c_dirName, value );
    Opts.Delete( 'M' );
  }
  else
    m_fileName = prefix( c_dirName, m_fileName );
  init_cgn( c_dirName );
  
  // mwuChunker Opts
  if ( Opts.Find( 'U',   value, mood ) ) {
    u_fileName = prefix( c_dirName, value );
    Opts.Delete( 'U' );
  }
  else
    u_fileName = prefix( c_dirName, u_fileName );
  
  // dependency parser Opts
  if ( Opts.Find( 'P', value, mood ) ) {
    p_fileName = prefix( c_dirName, value );
    Opts.Delete( 'P' );
  }
  else
    p_fileName = prefix( c_dirName, p_fileName );
  
#pragma omp parallel sections
  {
#pragma omp section
    myMblem::init( c_dirName, l_fileName );
#pragma omp section
    Mbma::init( c_dirName, m_fileName );
#pragma omp section 
    {
      cerr << "Initiating tagger..." << endl;
      tagger = new MbtAPI( string( "-s " ) + t_fileName );
    }
#pragma omp section
    {
      if ( doMwu ){
	mwuChunker::init( c_dirName, u_fileName);
	if ( doParse )
	  Parser::init( c_dirName, p_fileName );
      }
      else {
	if ( doParse )
	  cerr << " Parser disabled, because MWU is deselected" << endl;
	doParse = false;;
      }
    }
  }
  if ( doParse )
    showTimeSpan( cerr, "init Parse", Parser::initTime );

  if ( Opts.Find ('t', value, mood)) {
    TestFileName = value;
    Opts.Delete('t');
  };
  if ( Opts.Find ('m', value, mood)) {
    mode = atoi(value.c_str());
    Opts.Delete('m');
  };
  if ( Opts.Find ('s', value, mood)) {
    myOFS = value;
    if (tpDebug)
      cout << "Output Field Separator = >" << myOFS << "<" <<endl;
    Opts.Delete('s');
  };
  if ( Opts.Find ('h', value, mood)) {
    usage(ProgName);
  };
  
  cerr << "Initialization done." << endl;
  
}

bool similar( const string& tag, const string& lookuptag,
	      const string& CGNentry ){
  return tag.find( CGNentry ) != string::npos &&
    lookuptag.find( CGNentry ) != string::npos ;
}

string postprocess( const string& wstr, const string& lstr, 
		    vector<Mbma::MBMAana>& m) {
  vector<string> ltpairs;
  vector<string> wt;
  string word, tag;
  // BJ: quite inelegant, but I don't want to use the whole myMblem namespace;
  size_t nrlookup = myMblem::nrlookup;
  vector<string> lookuplemma = myMblem::lookuplemma;
  vector<string> lookuptag = myMblem::lookuptag;

  if (split_at(wstr, wt, "/") != 2) {
    wt[1] = wt[2];
  } 
  word = wt[0];
  tag = wt[1];

  if (tpDebug)
    cout << "wstr " << wstr << " split in word " << word << " and tag " << tag << endl;
  size_t num = split_at(lstr, ltpairs, " ");
  if (tpDebug) {
    cout << "postprocess:\n\tword: " << wt[0] <<"\n\ttags: " << wt[1] << " ";
    for(vector<MBMAana>::iterator it=m.begin(); it != m.end(); it++)
      cout << it->getTag() << it->getInflection()<< " ";
    
    for ( size_t i=0; i < num; i++) {
      vector<string> tmp;
      if ( split_at(ltpairs[i], tmp, "/") > 1 )
	cout << tmp[1] << " ";
      else
	cout << ltpairs[i] << " ";
    }
    cout << "\n\tmorph analyses: ";
    for (vector<MBMAana>::iterator it=m.begin(); it != m.end(); it++)
      cout << it->getMorph() << " ";
    cout << "\n\tlemmas: ";
    for ( size_t i=0; i < num; i++) {
      vector<string> tmp;
      split_at(ltpairs[i], tmp, "/");
      cout << tmp[0] << " ";
    }
    cout << endl <<endl;
  }
  size_t l = 0;
  if (nrlookup != num && tpDebug) {
    cout << "OOPS, nrlookup != num: " << nrlookup << " != " << num << endl;
  }
  /*
	for (l=0; l<nrlookup; l++)
	  lookupvisited[l]=0;
  */
  l=0;

  while ( l<nrlookup &&
	  tag != lookuptag[l] &&
	  
	  // Dutch CGN constraints
	  !similar( tag, lookuptag[l], "hulpofkopp" ) &&
	  !similar( tag, lookuptag[l], "neut,zelfst" ) &&
	  !similar( tag, lookuptag[l], "rang,bep,zelfst,onverv" ) &&
	  !similar( tag, lookuptag[l], "stell,onverv" ) &&
	  !similar( tag, lookuptag[l], "hoofd,prenom" ) &&
	  !similar( tag, lookuptag[l], "soort,ev" ) &&
	  !similar( tag, lookuptag[l], "ev,neut" ) &&
	  !similar( tag, lookuptag[l], "inf" ) &&
	  !similar( tag, lookuptag[l], "zelfst" ) &&
	  !similar( tag, lookuptag[l], "voorinf" ) &&
	  !similar( tag, lookuptag[l], "verldw,onverv" ) &&
	  !similar( tag, lookuptag[l], "ott,3,ev" ) &&
	  !similar( tag, lookuptag[l], "ott,2,ev" ) &&
	  !similar( tag, lookuptag[l], "ott,1,ev" ) &&
	  !similar( tag, lookuptag[l], "ott,2,ev" ) &&
	  !similar( tag, lookuptag[l], "ott,1,ev" ) &&
	  !similar( tag, lookuptag[l], "ott,1of2of3,mv" ) &&
	  !similar( tag, lookuptag[l], "ovt,1of2of3,mv" ) &&
	  !similar( tag, lookuptag[l], "ovt,1of2of3,ev" ) &&
	  !similar( tag, lookuptag[l], "ovt,3,ev" ) &&
	  !similar( tag, lookuptag[l], "ovt,2,ev" ) &&
	  !similar( tag, lookuptag[l], "ovt,1,ev" ) &&
	  !similar( tag, lookuptag[l], "ovt,1of2of3,mv" ))
   	l++;
  //if ((l<nrlookup-1)&&
  //  (strcmp(tag,lookuptag[l+1])==0))
  //l++;


  // BJ: HERE l is either < nrlookup which means there is some similarity between tag and lookuptag[l], or == nrlookup, which means no match
  
  string res;
  if ( l==nrlookup ) {
    if (tpDebug)
      cout << "NO CORRESPONDING TAG! " << tag << endl;
    
    /* first print the tag. was there a unique lexicon match, and
       the word is not a hicap plus the tagger said proper noun?
       then copy the tag from the lexicon. Else, believe the 
       tagger. */
    
    if ( word[0] >= 'A' &&
	 word[0] <= 'Z' &&
	 tag.find( "eigen") != string::npos )	{
      //		cout << tag << " "  << word << endl;
      res = tag + myOFS + word;
    } 
    else {
      //		cout << tag << " " << lookuplemma[0] << endl;
      res = tag + myOFS + lookuplemma[0];
    }
  }
  else {
    // l != nrlookup
    if ( word[0] >= 'A' &&
	 word[0] <= 'Z'&&
	 tag.find( "eigen" ) != string::npos ) {	    
      //		cout << tag << " " << word << endl;
      res = tag + myOFS + word;
    }
    else {
      //		cout << tag << " " << lookuplemma[l] << endl;
      res = tag + myOFS + lookuplemma[l];
    }
    //	    lookupvisited[l]=1;
  } // else l == nrlookup
  
  if (tpDebug)
    cout << "before morpho: " << res << endl;
  
  // BJ: OK, this was taglemma, now get the morphological analysis in
  vector<string> tag_parts;
  string converted_main_tag = "";
  string cseps = ",";
  size_t num_tag_parts = split_at(tag, tag_parts, cseps);
  // BJ:
  // tag_parts[0] == maintag + '(' + tagsub[0]
  // tagparts[-1] == tagsub[-1] + ')'
  if (tpDebug)
    cout << "tag: " << tag << endl;
  size_t sp = tag_parts[0].find("(");
  if ( sp == string::npos ) {
    cerr << "main tag without subparts: impossible\n";
    exit(-1);
  }
  string main_tag = tag_parts[0].substr( 0, sp );
  tag_parts[0].erase(0, sp+1 );
  sp = tag_parts[num_tag_parts - 1].find(")" );
  if ( sp == string::npos ) {
    cerr << "last subtag unclosed: impossible\n";
    exit(-1);
  }
  tag_parts[num_tag_parts -1].erase( sp, 1 );
  tag_parts.insert(tag_parts.begin(), main_tag);
  if (tpDebug)
    for ( size_t q =0 ; q < tag_parts.size(); ++q ) {
      cout << "\tpart #" << q << ": " << tag_parts[q] << endl;
    }
  
  mymap::const_iterator num_c = TAGconv.find(tag_parts[0]);
  if (tpDebug){
    if(num_c != TAGconv.end())
      cout << "#matches: " << num_c->first << " " << num_c->second << endl;
    else
      cout << "no match!\n";
  }
  size_t match = 0;	
  vector<size_t> matches;
  // since no match is impossible: check anyway
  if ( num_c != TAGconv.end() ) {
    converted_main_tag = num_c->second;
    match = 0;
    
    for ( size_t m_index = 0; m_index < m.size(); ++m_index ) {
      if (tpDebug)
	cout << "comparing " << converted_main_tag << " with " << m[m_index].getTag() << endl;
      if ( converted_main_tag == m[m_index].getTag() ) {
	match++;
	matches.push_back( m_index );
      }
    }
    if (tpDebug) {
      cout << "main tag " << converted_main_tag 
	   << " matches " << match 
	   << " morpho analyses: " << endl;
      for( size_t p=0; p < match; ++p )  {
	cout << "\tmatch #" << p << " : " << m[matches[p]].getMorph() << endl;
      }
    }
    //should be(come?) a switch
    if (match == 1) {
      res += myOFS + m[matches[0]].getMorph();
    } 
    else {
      // find the best match
      // from here, everything is going to have a double set of square brackets
      // to indicate uncertainty
      if (match > 1) {
	if (tpDebug)
	  cout << "multiple lemma's\n";
	map<string, int> possible_lemmas;
	map<string, int>::iterator lemma_count;
	
	// find unique lemma's
	for ( size_t q = 0; q < match; ++q ) {
	  lemma_count = possible_lemmas.find(m[matches[q]].getMorph());
	  if (lemma_count == possible_lemmas.end())
	    possible_lemmas.insert( make_pair( m[matches[q]].getMorph(),1 ) );
	  else
	    lemma_count->second++;
	}
	if (tpDebug)
	  cout << "#unique lemma's: " << possible_lemmas.size() << endl;
	// append all unique lemmas
	size_t current_best = 0;
	bool hit = false;
	if ( possible_lemmas.size() >= 1) {
	  // find best match
	  // loop through subparts 1 t/m num_tag_parts-1, 
	  // and match with inflections from each m
	  int match_count = 0;
	  int max_count = 0;
	  
	  string inflection;
	  for ( size_t q=0; q < match; ++q ) {
	    match_count = 0;
	    inflection = m[matches[q]].getInflection();
	    if (tpDebug)
	      cout << "matching " << inflection << " with " << tag << endl;
	    for ( size_t u=1; u < num_tag_parts; ++u ) {
	      mymap::const_iterator conv_tag_p = TAGconv.find(tag_parts[u]);
	      if (conv_tag_p != TAGconv.end()) {
		string c = conv_tag_p->second;
		if ( inflection.find( c ) != string::npos )
		  match_count++;
	      }
	    }
	    if (tpDebug)
	      cout << "score: " << match_count << endl;
	    if (match_count > max_count) {
	      max_count = match_count;
	      current_best = q;
	      hit = true;
	    }
	  }
	} 			
	if ( hit ) {
	  if (tpDebug)
	    cout << "best match: " << m[matches[current_best]].getMorph() << endl;
	  res += myOFS + m[matches[current_best]].getMorph();
	} 
	else {
	  // last resort: append all (> 1) lemma's
	  if (tpDebug)
	    cout << "Oops, no match, improvise" << endl;
	  lemma_count = possible_lemmas.begin();
	  if (tpDebug)
	    cout << "Appending: " << lemma_count->first << " to " << res << endl;
	  res += myOFS + "[" + lemma_count->first;
	  lemma_count++;
	  while (lemma_count != possible_lemmas.end()){
	    if (tpDebug)
	      cout << "Appending: " << lemma_count->first << " to " << res << endl;
	    res += "/" + lemma_count->first;
	    lemma_count++;
	  }
	  res += "]";
	} // no current_best
      } 
      else {
	// match < 1
	// fallback option: put the word in DOUBLE bracket's and pretend it's a lemma ;-)
	decap(word,tag);
	res += myOFS + "[" + word + "]";
	//res += " [[" + word +"]]";
      }
    }
  }
  if (tpDebug)
    cout << "final: " << res << endl;
  return res;

}  // postprocess

ostream &showResults( ostream& os,
		      const vector<mwuChunker::ana>& ana ){
  for( size_t i = 0; i < ana.size(); ++i )
    os << i+1 << "\t" << ana[i] << endl;
  return os;
}

void showProgress( ostream& os, const string& line, 
		   struct timeval& timeBefore ){
  struct timeval timeAfter;
  gettimeofday(&timeAfter, 0 );
  os << line << " took:" << timeAfter.tv_sec - timeBefore.tv_sec
     << " seconds and " << timeAfter.tv_usec - timeBefore.tv_usec 
     << " microseconds" << endl;
}
  
void Test( const string& infilename ) {
  // init's are done
  
  if ( doTok ){
    //Tokenize
    struct timeval tokTime;
    tokTime.tv_sec=0;
    tokTime.tv_usec=0;
    struct timeval startTime;
    struct timeval endTime;
    gettimeofday(&startTime, 0 );
    TokenizedTestFileName = tokenize(infilename);
    LineTokenizedTestFileName = linetokenize(TokenizedTestFileName);
    gettimeofday(&endTime,0);
    addTimeDiff( tokTime, startTime, endTime );
    showTimeSpan( cerr, "tokenizing", tokTime );
    // remove tokenized file
    string syscommand = "rm -f " + infilename + ".tok\n";
    if ( system(syscommand.c_str()) != 0 ){
      cerr << "execution of " << syscommand << " failed. We go on" << endl;
    }
  }
  else
    LineTokenizedTestFileName = infilename;

  ifstream IN( LineTokenizedTestFileName.c_str() );

  timeval parseTime;
  parseTime.tv_sec=0;
  parseTime.tv_usec=0;
  timeval mwuTime;
  mwuTime.tv_sec=0;
  mwuTime.tv_usec=0;
  timeval mblemTime;
  mblemTime.tv_sec=0;
  mblemTime.tv_usec=0;
  timeval mbaTime;
  mbaTime.tv_sec=0;
  mbaTime.tv_usec=0;
  timeval tagTime;
  tagTime.tv_sec=0;
  tagTime.tv_usec=0;

  string line;
  while( getline(IN, line) ) {
    if (line.length()>1) {

      if (tpDebug) 
	cout << "in: " << line << endl;
      timeval tagStartTime;
      timeval tagEndTime;
      gettimeofday(&tagStartTime,0);
      string tagged = tagger->Tag(line);
      gettimeofday(&tagEndTime,0);
      addTimeDiff( tagTime, tagStartTime, tagEndTime );
      if (tpDebug) {
	cout << "line: " << line
	     <<"\ntagged: "<< tagged
	     << endl;
      }
      vector<string> words, tagged_words;
      size_t num_words = split_at( line, words, sep );
      size_t num_tagged_words = split_at( tagged, tagged_words, sep );
      if (tpDebug) {
	  cout << "#words: " << num_words << endl;
	  for( size_t i = 0; i < num_words; i++) 
	    cout << "\tword #" << i <<": " << words[i] << endl;
	  cout << "#tagged_words: " << num_tagged_words << endl;
	  for( size_t i = 0; i < num_tagged_words; i++) 
	    cout << "\ttagged_word #" << i <<": " << tagged_words[i] << endl;
	  
      }
      if (num_words && num_words != num_tagged_words - 1) {
	// the last "word" is <utt> which gets added by the tagger
	cerr << "Something is rotten here, #words != #tagged_words\n";
	exit(1);
      }
      vector<mwuChunker::ana> final_ana;
      for( size_t i = 0; i < num_words; ++i ) {
	  //process each word and dump every ana for now
	  string analysis;
	  vector<MBMAana> res;
	  timeval mbaStartTime;
	  timeval mbaEndTime;
	  gettimeofday(&mbaStartTime,0);
	  Mbma::Classify(tagged_words[i], res);
	  gettimeofday(&mbaEndTime,0);
	  addTimeDiff( mbaTime, mbaStartTime, mbaEndTime );

	  timeval mblemStartTime;
	  timeval mblemEndTime;
	  gettimeofday(&mblemStartTime,0);
	  string lemma = myMblem::Classify(tagged_words[i]);
	  gettimeofday(&mblemEndTime,0);
	  addTimeDiff( mblemTime, mblemStartTime, mblemEndTime );

	  if (tpDebug) 
	    {
	      cout << "word: " << words[i] << "\t#anas: " << res.size()<< endl;
	      for (vector<MBMAana>::iterator it=res.begin(); it != res.end(); it++)
		cout << *it << endl;
	      cout   << "tagged word[" << i <<"]: " << tagged_words[i]  << endl
		     <<"lemma: " << lemma
		     << endl;
	    }

	  analysis = postprocess(tagged_words[i], lemma, res);

	  string tmp = words[i] + myOFS + analysis;
	  if (tpDebug)
	    cout << tmp;
	  mwuChunker::ana tmp1(tmp);
	  final_ana.push_back( tmp1 );
	  if (tpDebug){
	    cout << endl;
	  }
	  res.clear();

	} //for int i = 0 to num_words

      if ( doMwu ){
	//mwu chunker goes here, otherwise we get a mess when 
	if (tpDebug)
	  cout << "starting mwu Chunking ... \n";
	timeval mwuStartTime;
	timeval mwuEndTime;
	gettimeofday(&mwuStartTime,0);
	mwuChunker::Classify(words, final_ana);
	gettimeofday(&mwuEndTime,0);
	addTimeDiff( mwuTime, mwuStartTime, mwuEndTime );
      }
      if (tpDebug) {
	cout << "\n\nfinished mwu chunking!\n";
	for( size_t i =0; i < words.size(); i++)
	  cout << i <<": " << words[i] << endl;
	cout << endl;
      }
      if ( doParse ){
	timeval parseStartTime;
	timeval parseEndTime;
	gettimeofday(&parseStartTime,0);
	Parser::Parse( final_ana, parserTmpFile );
	gettimeofday(&parseEndTime,0);
	addTimeDiff( parseTime, parseStartTime, parseEndTime );
      }
      showResults( cout, final_ana ); 
      if (num_words>0)
      	cout <<endl;
    } //while getline
  }
  showTimeSpan( cerr, "tagging          ", tagTime );
  showTimeSpan( cerr, "MBA              ", mbaTime );
  showTimeSpan( cerr, "Mblem            ", mblemTime );
  showTimeSpan( cerr, "MWU resolving    ", mwuTime );
  showTimeSpan( cerr, "Parsing (prepare)", Parser::prepareTime );
  showTimeSpan( cerr, "Parsing (pairs)  ", Parser::pairsTime );
  showTimeSpan( cerr, "Parsing (rels)   ", Parser::relsTime );
  showTimeSpan( cerr, "Parsing (dir)    ", Parser::dirTime );
  showTimeSpan( cerr, "Parsing (csi)    ", Parser::csiTime );
  showTimeSpan( cerr, "Parsing (total)  ", parseTime );
  if ( doTok ){
    // remove linetokenized file
    string syscommand = "rm -f " + infilename + ".tok.lin\n";
    if ( system(syscommand.c_str()) != 0 ){
      cerr << "execution of " << syscommand << " failed. We go on" << endl;
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    exit(0);
  }
  ProgName = argv[0];
  cerr << ProgName << " v." << VERSION << endl << endl;

  try {
    TestFileName = "";
    TimblOpts Opts(argc, argv);
    
    parse_args(Opts); //gets a settingsfile for each component, 
    //and starts init for that mod

    if ( !TestFileName.empty() ) {
      Test(TestFileName);
    }
  }
  catch ( const exception& e ){
    cerr << "fatal error: " << e.what() << endl;
  }
  return 0;
}
