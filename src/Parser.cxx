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

#include "Python.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include "config.h"
#include "timbl/TimblAPI.h"
#include "tadpole/Tadpole.h"
#include "tadpole/mwu_chunker_mod.h"
#include "tadpole/Parser.h"

using namespace std;
using namespace Timbl;

namespace Parser {
  class PyObjectRef {
  private:
    PyObject* ref;
    
  public:
    PyObjectRef() : ref(0) {}
    
    PyObjectRef(PyObject* obj) : ref(obj) {}
    
    void assign(PyObject* obj) {
      // assume ref == 0, otherwhise call Py_DECREF
      ref = obj;
    }
    
    operator PyObject* () {
      return ref;
    }
    
    ~PyObjectRef() {
      Py_DECREF(ref);
    }
  };

  class PythonInterface {
  private:
    PyObjectRef module;
    PyObjectRef mainFunction;
    
  public:
    PythonInterface();
    ~PythonInterface();
    
    void parse(const std::string& depFile,
	       const std::string& modFile,
	       const std::string& dirFile,
	       const std::string& maxDist,
	       const std::string& inputFile,
	       const std::string& outputFile);
  };  

  PythonInterface::PythonInterface( ) {
    Py_OptimizeFlag = 1; // enable optimisation (-O) mode
    Py_Initialize();
    string newpath = Py_GetPath();
    newpath += string(":") + PYTHONDIR;
    PySys_SetPath( (char*)newpath.c_str() );
    try {
      PyObject *im = PyImport_ImportModule( "csidp" );
      if ( im ){
	module.assign( im );
	PyObject *mf = PyObject_GetAttrString(module, "main");
	if ( mf )
	  mainFunction.assign( mf );
	else {
	  PyErr_Print();
	  exit(1);
	}
      }
      else {
	PyErr_Print();
	exit(1);
      }
    }
    catch( exception const & ){
      PyErr_Print();
      exit(1);
    }
  }

  PythonInterface::~PythonInterface() {
    Py_Finalize();
  }
  
  void PythonInterface::parse(const std::string& depFile,
			      const std::string& modFile,
			      const std::string& dirFile,
			      const std::string& maxDist,
			      const std::string& inputFile,
			      const std::string& outputFile) {

    PyObjectRef tmp = PyObject_CallFunction(mainFunction,
					    "[s, s, s, s, s, s, s, s, s, s, s]",
					    "--dep", depFile.c_str(),
					    "--mod", modFile.c_str(),
					    "--dir", dirFile.c_str(),
					    "-m", maxDist.c_str(),
					    "--out", outputFile.c_str(),
					    inputFile.c_str() );
  }
  
  string pairsFileName;
  string pairsOptions = "-a1 +D +vdb+di";
  string dirFileName;
  string dirOptions = "-a1 +D +vdb+di";
  string relsFileName;
  string relsOptions = "-a1 +D +vdb+di";

  string groupSizeS = "20";
  size_t groupSize = 20;

  static TimblAPI *pairs = 0;
  static TimblAPI *dir = 0;
  static TimblAPI *rels = 0;
  PythonInterface *PI;
  static bool isInit = false;

  timeval initTime;
  timeval prepareTime;
  timeval pairsTime;
  timeval relsTime;
  timeval dirTime;
  timeval csiTime;

  bool readsettings( const string& cDir, const string& fname ){
    ifstream setfile(fname.c_str(), ios::in);
    if( !setfile.good()){
      return false;
    }
    string SetBuffer;
    while( getline( setfile, SetBuffer ) ){
      if ( SetBuffer.empty() )
	continue;
      bool problem = false;
      vector<string>parts;
      size_t num = split_at( SetBuffer, parts, " " );
      if ( num >= 2 ){
	switch ( parts[0][0] ) {
	case 'g':
	  if ( parts[0] == "groupSize" ){
	    size_t gs;
	    if ( stringTo<size_t>( parts[1], gs, 0, 50 ) ){
	      groupSizeS = parts[1];
	      groupSize = gs;
	    }
	    else {
	      cerr << "invalid groupSize value in config file" << endl;
	      cerr << "keeping default " << groupSizeS << endl;
	      problem = true;
	    }
	  }
	  else
	    problem = true;
	  break;
	case 'p':
	  if ( parts[0] == "pairsFile" )
	    pairsFileName = prefix( cDir, parts[1] );
	  else if ( parts[0] == "pairsOptions" ){
	    string opts;
	    for ( unsigned int i=1; i<num; ++i )
	      opts += parts[i] + " ";
	    pairsOptions = opts;
	  }
	  else
	    problem = true;
	  break;
 	case 'd':
	  if ( parts[0] == "dirFile" )
	    dirFileName = prefix( cDir, parts[1] );
	  else if ( parts[0] == "dirOptions" ){
	    string opts;
	    for ( unsigned int i=1; i<num; ++i )
	      opts += parts[i] + " ";
	    dirOptions = opts;
	  }
	  else
	    problem = true;
	  break;
 	case 'r':
	  if ( parts[0] == "relsFile" )
	    relsFileName = prefix( cDir, parts[1] );
	  else if ( parts[0] == "relsOptions" ){
	    string opts;
	    for ( unsigned int i=1; i<num; ++i )
	      opts += parts[i] + " ";
	    relsOptions = opts;
	  }
	  else
	    problem = true;
	  break;
	default:
	  problem = true;
	  break;
	}
      }
      else
	problem = true;
      if ( problem )
	cerr << "Unknown option in settingsfile, (ignored)\n"
	     << SetBuffer << " ignored." <<endl;
    }
    return true;
  }

  bool init( const string& cDir, const string& fname ){
    PI = new PythonInterface();
    bool happy = false;
    cerr << "initiating parser ... " << endl;
    prepareTime.tv_sec=0;
    prepareTime.tv_usec=0;
    pairsTime.tv_sec=0;
    pairsTime.tv_usec=0;
    relsTime.tv_sec=0;
    relsTime.tv_usec=0;
    dirTime.tv_sec=0;
    dirTime.tv_usec=0;
    csiTime.tv_sec=0;
    csiTime.tv_usec=0;
    initTime.tv_sec=0;
    initTime.tv_usec=0;
    timeval startTime;
    timeval endTime;
    gettimeofday(&startTime,0);
    if ( !readsettings( cDir, fname)) {
      cerr << "Cannot read parser settingsfile " << fname << endl;
    }
    else {
      pairs = new TimblAPI( pairsOptions );
      if ( pairs->Valid() )
	happy = pairs->GetInstanceBase( pairsFileName );
      else
	cerr << "creating Timbl for pairs failed:" << pairsOptions << endl;
      if ( happy ){
	dir = new TimblAPI( dirOptions );
	if ( dir->Valid() )
	  happy = dir->GetInstanceBase( dirFileName );
	else
	  cerr << "creating Timbl for dir failed:" << dirOptions << endl;
	if ( happy ){
	  rels = new TimblAPI( relsOptions );
	  if ( rels->Valid() )
	    happy = rels->GetInstanceBase( relsFileName );
	  else
	    cerr << "creating Timbl for rels failed:" << relsOptions << endl;
	}
      }
    }
    isInit = happy;
    gettimeofday(&endTime,0);
    addTimeDiff( initTime, startTime, endTime );
    return happy;
  }

  void createPairs( const vector<mwuChunker::ana>& ana,
		    const string& fileName ){
    string pFile = fileName + ".pairs.inst";
    unlink( pFile.c_str() );
    ofstream ps( pFile.c_str() );
    if ( ps ){
      if ( ana.size() == 1 ){
	ps << "__ " << ana[0].getWord() << " __"
	   << " ROOT ROOT ROOT __ " << ana[0].getTagHead() 
	   << " __ ROOT ROOT ROOT "
	   << ana[0].getTagHead() << "^ROOT ROOT ROOT ROOT^" 
	   << ana[0].getTagMods() 
	   << " _" << endl;
      }
      else {
	for ( size_t i=0 ; i < ana.size(); ++i ){
	  string word_1, word0, word1;
	  string tag_1, tag0, tag1;
	  string mods0;
	  if ( i == 0 ){
	    word_1 = "__";
	    tag_1 = "__";
	  }
	  else {
	    word_1 = ana[i-1].getWord();
	    tag_1 = ana[i-1].getTagHead();
	  }
	  word0 = ana[i].getWord();
	  tag0 = ana[i].getTagHead();
	  mods0 = ana[i].getTagMods();
	  if ( i == ana.size() - 1 ){
	    word1 = "__";
	    tag1 = "__";
	  }
	  else {
	    word1 = ana[i+1].getWord();
	    tag1 = ana[i+1].getTagHead();
	  }
	  ps << word_1 << " " << word0 << " " << word1
	     << " ROOT ROOT ROOT "
	     << tag_1 << " " << tag0 << " " << tag1 
	     << " ROOT ROOT ROOT "
	     << tag0 << "^ROOT ROOT ROOT ROOT^" << mods0
	     << " _" << endl;
	}
	// 
	for ( size_t wPos=0; wPos < ana.size(); ++wPos ){
	  string w_word_1, w_word0, w_word1;
	  string w_tag_1, w_tag0, w_tag1;
	  string w_mods0;
	  if ( wPos == 0 ){
	    w_word_1 = "__";
	    w_tag_1 = "__";
	  }
	  else {
	    w_word_1 = ana[wPos-1].getWord();
	    w_tag_1 = ana[wPos-1].getTagHead();
	  }
	  w_word0 = ana[wPos].getWord();
	  w_tag0 = ana[wPos].getTagHead();
	  w_mods0 = ana[wPos].getTagMods();
	  if ( wPos == ana.size()-1 ){
	    w_word1 = "__";
	    w_tag1 = "__";
	  }
	  else {
	    w_word1 = ana[wPos+1].getWord();
	    w_tag1 = ana[wPos+1].getTagHead();
	  }
	  for ( size_t pos=0; pos < ana.size(); ++pos ){
	    //	  os << wPos << "-" << pos << " ";
	    if ( pos > wPos + groupSize )
	      break;
	    if ( wPos == pos )
	      continue;
	    if ( wPos > groupSize + pos )
	      continue;

	    ps << w_word_1;
	    ps << " " << w_word0;
	    ps << " " << w_word1;

	    if ( pos == 0 )
	      ps << " __";
	    else
	      ps << " " << ana[pos-1].getWord();
	    if ( pos < ana.size() )
	      ps << " " << ana[pos].getWord();
	    else
	      ps << " __";
	    if ( pos < ana.size()-1 )
	      ps << " " << ana[pos+1].getWord();
	    else
	      ps << " __";
	    ps << " " << w_tag_1;
	    ps << " " << w_tag0;
	    ps << " " << w_tag1;
	    if ( pos == 0 )
	      ps << " __";
	    else
	      ps << " " << ana[pos-1].getTagHead();
	    if ( pos < ana.size() )
	      ps << " " << ana[pos].getTagHead();
	    else
	      ps << " __";
	    if ( pos < ana.size()-1 )
	      ps << " " << ana[pos+1].getTagHead();
	    else
	      ps << " __";
	    
	    ps << " " << w_tag0 << "^";
	    if ( pos < ana.size() )
	      ps << ana[pos].getTagHead();
	    else
	      ps << "__";
	    
	    if ( wPos > pos )
	      ps << " LEFT " << wPos - pos;
	    else
	      ps << " RIGHT " << pos - wPos;
	    if ( pos >= ana.size() )
	      ps << " __";
	    else
	      ps << " " << ana[pos].getTagMods();
	    ps << "^" << w_mods0;
	    ps << " __" << endl;
	  }
	}
      }
    }
  };

  void prepare( const vector<mwuChunker::ana>& ana,
		const string& fileName ){
    createPairs( ana, fileName );
    string word_2, word_1, word0, word1, word2;
    string tag_2, tag_1, tag0, tag1, tag2;
    string mod_2, mod_1, mod0, mod1, mod2;
    string dFile = fileName + ".dir.inst";
    unlink( dFile.c_str() );
    ofstream ds( dFile.c_str() );
    string rFile = fileName + ".rels.inst";
    unlink( rFile.c_str() );
    ofstream rs( rFile.c_str() );
    if ( ds && rs ){
      if ( ana.size() == 1 ){
	word0 = ana[0].getWord();
	tag0 = ana[0].getTagHead();
	mod0 = ana[0].getTagMods();
	ds << "__ __";
	ds << " " << word0;
	ds << " __ __ __ __";
	ds << " " << tag0;
	ds << " __ __ __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " __ __ __^" << tag0;
	ds << " " << tag0 << "^__";
	ds << " __";
	ds << " " << mod0;
	ds << " __ ROOT" << endl;
	//
	rs << "__ __";
	rs << " " << word0;
	rs << " __ __";
	rs << " " << mod0;
	rs << " __ __";
	rs << " " << tag0;
	rs << " __ __";
	rs << " __^" << tag0;
	rs << " " << tag0 << "^__";
	rs << " __^__^" << tag0;
	rs << " " << tag0 << "^__^__";
	rs << " __" << endl;
      }
      else if ( ana.size() == 2 ){
	word0 = ana[0].getWord();
	tag0 = ana[0].getTagHead();
	mod0 = ana[0].getTagMods();
	word1 = ana[1].getWord();
	tag1 = ana[1].getTagHead();
	mod1 = ana[1].getTagMods();
	ds << "__ __";
	ds << " " << word0;
	ds << " " << word1;
	ds << " __ __ __";
	ds << " " << tag0;
	ds << " " << tag1;
	ds << " __ __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " " << word1 << "^" << tag1;
	ds << " __ __^" << tag0;
	ds << " " << tag0 << "^" << tag1;
	ds << " __";
	ds << " " << mod0;
	ds << " " << mod1;
	ds << " ROOT" << endl;
	ds << "__";
	ds << " " << word0;
	ds << " " << word1;
	ds << " __ __ __";
	ds << " " << tag0;
	ds << " " << tag1;
	ds << " __ __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " " << word1 << "^" << tag1;
	ds << " __ __";
	ds << " " << tag0 << "^" << tag1;
	ds << " " << tag1 << "^__";
	ds << " " << mod0;
	ds << " " << mod1;
	ds << " __";
	ds << " ROOT" << endl;
	//
	rs << "__ __";
	rs << " " << word0;
	rs << " " << word1;
	rs << " __";
	rs << " " << mod0;
	rs << " __ __";
	rs << " " << tag0;
	rs << " " << tag1;
	rs << " __";
	rs << " __^" << tag0;
	rs << " " << tag0 << "^" << tag1;
	rs << " __^__^" << tag0;
	rs << " " << tag0 << "^" << tag1 << "^__";
	rs << " __" << endl;
	rs << "__";
	rs << " " << word0;
	rs << " " << word1;
	rs << " __ __";
	rs << " " << mod1;
	rs << " __";
	rs << " " << tag0;
	rs << " " << tag1;
	rs << " __ __";
	rs << " " << tag0 << "^" << tag1;
	rs << " " << tag1 << "^__";
	rs << " __^" << tag0 << "^" << tag1;
	rs << " " << tag1 << "^__^__";
	rs << " __" << endl;
      }
      else if ( ana.size() == 3 ) {
	word0 = ana[0].getWord();
	tag0 = ana[0].getTagHead();
	mod0 = ana[0].getTagMods();
	word1 = ana[1].getWord();
	tag1 = ana[1].getTagHead();
	mod1 = ana[1].getTagMods();
	word2 = ana[2].getWord();
	tag2 = ana[2].getTagHead();
	mod2 = ana[2].getTagMods();
	ds << "__ __";
	ds << " " << word0;
	ds << " " << word1;
	ds << " " << word2;
	ds << " __ __";
	ds << " " << tag0;
	ds << " " << tag1;
	ds << " " << tag2;
	ds << " __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " " << word1 << "^" << tag1;
	ds << " " << word2 << "^" << tag2;
	ds << " __^" << tag0;
	ds << " " << tag0 << "^" << tag1;
	ds << " __";
	ds << " " << mod0;
	ds << " " << mod1;
	ds << " ROOT" << endl;
	ds << "__";
	ds << " " << word0;
	ds << " " << word1;
	ds << " " << word2;
	ds << " __ __";
	ds << " " << tag0;
	ds << " " << tag1;
	ds << " " << tag2;
	ds << " __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " " << word1 << "^" << tag1;
	ds << " " << word2 << "^" << tag2;
	ds << " __";
	ds << " " << tag0 << "^" << tag1;
	ds << " " << tag1 << "^" << tag2;
	ds << " " << mod0;
	ds << " " << mod1;
	ds << " " << mod2;
	ds << " ROOT" << endl;
	ds << word0;
	ds << " " << word1;
	ds << " " << word2;
	ds << " __ __";
	ds << " " << tag0;
	ds << " " << tag1;
	ds << " " << tag2;
	ds << " __ __";
	ds << " " << word0 << "^" << tag0;
	ds << " " << word1 << "^" << tag1;
	ds << " " << word2 << "^" << tag2;
	ds << " __ __";
	ds << " " << tag1 << "^" << tag2;
	ds << " " << tag2 << "^__";
	ds << " " << mod1;
	ds << " " << mod2;
	ds << " __";
	ds << " ROOT" << endl;
	//
	rs << "__ __";
	rs << " " << word0;
	rs << " " << word1;
	rs << " " << word2;
	rs << " " << mod0;
	rs << " __ __";
	rs << " " << tag0;
	rs << " " << tag1;
	rs << " " << ana[2].getTagHead();
	rs << " __^" << tag0;
	rs << " " << tag0 << "^" << tag1;
	rs << " __^__^" << tag0;
	rs << " " << tag0 << "^" << tag1 << "^" << tag2;
	rs << " __" << endl;
	rs << "__";
	rs << " " << word0;
	rs << " " << word1;
	rs << " " << word2;
	rs << " __";
	rs << " " << mod1;
	rs << " __";
	rs << " " << tag0;
	rs << " " << tag1;
	rs << " " << tag2;
	rs << " __";
	rs << " " << tag0 << "^" << tag1;
	rs << " " << tag1 << "^" << tag2;
	rs << " __^" << tag0 << "^" << tag1;
	rs << " " << tag1 << "^" << tag2 << "^__";
	rs << " __" << endl;
	rs << word0;
	rs << " " << word1;
	rs << " " << word2;
	rs << " __ __";
	rs << " " << mod2;
	rs << " " << tag0;
	rs << " " << tag1;
	rs << " " << tag2;
	rs << " __ __";
	rs << " " << tag1 << "^" << tag2;
	rs << " " << tag2 << "^__";
	rs << " " << tag0 << "^" << tag1 << "^" << tag2;
	rs << " " << tag2 << "^__^__";
	rs << " __" << endl;
      }
      else {
	for ( size_t i=0 ; i < ana.size(); ++i ){
	  if ( i == 0 ){
	    word_2 = "__";
	    tag_2 = "__";
	    mod_2 = "__";
	    word_1 = "__";
	    tag_1 = "__";
	    mod_1 = "__";
	  }
	  else if ( i == 1 ){
	    word_2 = "__";
	    tag_2 = "__";
	    mod_2 = "__";
	    word_1 = ana[i-1].getWord();
	    tag_1 = ana[i-1].getTagHead();
	    mod_1 = ana[i-1].getTagMods();
	  }
	  else {
	    word_2 = ana[i-2].getWord();
	    tag_2 = ana[i-2].getTagHead();
	    mod_2 = ana[i-2].getTagMods();
	    word_1 = ana[i-1].getWord();
	    tag_1 = ana[i-1].getTagHead();
	    mod_1 = ana[i-1].getTagMods();
	  }
	  word0 = ana[i].getWord();
	  tag0 = ana[i].getTagHead();
	  mod0 = ana[i].getTagMods();
	  if ( i < ana.size() - 2 ){
	    word1 = ana[i+1].getWord();
	    tag1 = ana[i+1].getTagHead();
	    mod1 = ana[i+1].getTagMods();
	    word2 = ana[i+2].getWord();
	    tag2 = ana[i+2].getTagHead();
	    mod2 = ana[i+2].getTagMods();
	  }
	  else if ( i == ana.size() - 2 ){
	    word1 = ana[i+1].getWord();
	    tag1 = ana[i+1].getTagHead();
	    mod1 = ana[i+1].getTagMods();
	    word2 = "__";
	    tag2 = "__";
	    mod2 = "__";
	  }
	  else {
	    word1 = "__";
	    tag1 = "__";
	    mod1 = "__";
	    word2 = "__";
	    tag2 = "__";
	    mod2 = "__";
	  }
	  ds << word_2;
	  ds << " " << word_1;
	  ds << " " << word0;
	  ds << " " << word1;
	  ds << " " << word2;
	  ds << " " << tag_2;
	  ds << " " << tag_1;
	  ds << " " << tag0;
	  ds << " " << tag1;
	  ds << " " << tag2;
	  ds << " " << word_2 << "^" << tag_2;
	  ds << " " << word_1 << "^" << tag_1;
	  ds << " " << word0 << "^" << tag0;
	  ds << " " << word1 << "^" << tag1;
	  ds << " " << word2 << "^" << tag2;
	  ds << " " << tag_1 << "^" << tag0;
	  ds << " " << tag0 << "^" << tag1;
	  ds << " " << mod_1;
	  ds << " " << mod0;
	  ds << " " << mod1;
	  ds << " ROOT" << endl;
	  //
	  rs << word_2;
	  rs << " " << word_1;
	  rs << " " << word0;
	  rs << " " << word1;
	  rs << " " << word2;
	  rs << " " << mod0;
	  rs << " " << tag_2;
	  rs << " " << tag_1;
	  rs << " " << tag0;
	  rs << " " << tag1;
	  rs << " " << tag2;
	  rs << " " << tag_1 << "^" << tag0;
	  rs << " " << tag0 << "^" << tag1;
	  rs << " " << tag_2 << "^" << tag_1 << "^" << tag0;
	  rs << " " << tag0 << "^" << tag1 << "^" << tag2;
	  rs << " __" << endl;
	}
      }
    }
  }
  
  void Parse( vector<mwuChunker::ana>& final_ana, const string& fileName ){
    static const char *instOutName ="tadpoleParser.inst.out";
    static const char *dirOutName ="tadpoleParser.dir.out";
    static const char *relOutName ="tadpoleParser.rels.out";
    if ( !isInit ){
      cerr << "Parser is not initialized!" << endl;
      exit(1);
    }
    if ( final_ana.size() < 1 ){
      cerr << "unable to parse an analisis without words" << endl;
      return;
    }
    timeval startTime;
    timeval endTime;
    string resFileName = fileName + ".result"; 
    ofstream anaFile( fileName.c_str() );
    if ( anaFile ){
      saveAna( anaFile, final_ana );
      unlink( resFileName.c_str() );
      gettimeofday(&startTime,0);    
//       string cmd = string("sh ") + BIN_PATH + "/prepareParser.sh " + fileName;
//       // run some python scripts to prepare the input.
//       int result = system( cmd.c_str() ); 
//       if ( result != 0 ){
// 	cerr << "initializing parse failed" << endl;
// 	return;
//       }
      prepare( final_ana, fileName );
      gettimeofday(&endTime,0);
      addTimeDiff( prepareTime, startTime, endTime );    
#pragma omp parallel sections
      {
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  unlink( instOutName );
	  pairs->Test( fileName + ".pairs.inst", instOutName );
	  gettimeofday(&endTime,0);
	  addTimeDiff( pairsTime, startTime, endTime );
	}
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  unlink( dirOutName );
	  dir->Test( fileName +".dir.inst", dirOutName );
	  gettimeofday(&endTime,0);
	  addTimeDiff( dirTime, startTime, endTime );
	}
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  unlink( relOutName );
	  rels->Test( fileName + ".rels.inst", relOutName );
	  gettimeofday(&endTime,0);
	  addTimeDiff( relsTime, startTime, endTime );
	}
      }
      gettimeofday(&startTime,0);
      try {
	PI->parse( instOutName,
		   relOutName,
		   dirOutName,
		   groupSizeS,
		   fileName,
		   resFileName );
	if ( PyErr_Occurred() )
	  PyErr_Print();
      }
      catch( exception const & ){
	PyErr_Print();
      }
//       string cmd1 = string("sh ") + BIN_PATH + "/finalizeParser.sh " + fileName;
//       int result1 = system( cmd1.c_str() );  
//       if ( result1 != 0 ){
// 	cerr << "finalizing parse failed" << endl;
// 	return;
//       }
      gettimeofday(&endTime,0);
      addTimeDiff( csiTime, startTime, endTime );
      ifstream resFile( resFileName.c_str() );
      if ( resFile ){
	readAna( resFile, final_ana );
      }
      else
	cerr << "couldn't open results file: " << resFileName << endl;
    }
  }
}
