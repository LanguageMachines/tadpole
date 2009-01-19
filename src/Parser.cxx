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

  size_t groupLen = 20;

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
    ofstream os( pFile.c_str() );
    if ( os ){
      if ( ana.size() == 1 ){
	os << "__ " << ana[0].getWord() << " __"
	   << " ROOT ROOT ROOT __ " << ana[0].getTagHead() 
	   << " __ ROOT ROOT ROOT "
	   << ana[0].getTagHead() << "^ROOT ROOT ROOT ROOT^" 
	   << ana[0].getTagMods() 
	   << " _" << endl;
      }
      else {
	os << "__ " << ana[0].getWord() << " " << ana[1].getWord()
	   << " ROOT ROOT ROOT __ " << ana[0].getTagHead() 
	   << " " << ana[1].getTagHead() << " ROOT ROOT ROOT "
	   << ana[0].getTagHead() << "^ROOT ROOT ROOT ROOT^" 
	   << ana[0].getTagMods() 
	   << " _" << endl;
	for ( size_t i=1 ; i < ana.size() -1 ; ++i ){
	  os << ana[i-1].getWord() << " " << ana[i].getWord()
	     << " " << ana[i+1].getWord()
	     << " ROOT ROOT ROOT " << ana[i-1].getTagHead() 
	     << " " << ana[i].getTagHead()
	     << " " << ana[i+1].getTagHead() << " ROOT ROOT ROOT "
	     << ana[i].getTagHead() << "^ROOT ROOT ROOT ROOT^" 
	     << ana[i].getTagMods() 
	     << " _" << endl;
	}
	os << ana[ana.size()-2].getWord() << " " << ana[ana.size()-1].getWord()
	   << " __ ROOT ROOT ROOT " << ana[ana.size()-2].getTagHead() 
	   << " " << ana[ana.size()-1].getTagHead() << " __ ROOT ROOT ROOT "
	   << ana[ana.size()-1].getTagHead() << "^ROOT ROOT ROOT ROOT^" 
	   << ana[ana.size()-1].getTagMods()
	   << " _" << endl;
	// 
	for ( size_t wPos=0 ; wPos < ana.size(); ++wPos ){
	  for ( size_t pos=0; pos < ana.size(); ++pos ){
	    //	  os << wPos << "-" << pos << " ";
	    if ( pos > wPos + groupLen )
	      break;
	    if ( wPos == pos )
	      continue;
	    if ( wPos > groupLen + pos )
	      continue;
	    if ( wPos >= ana.size()-1 ) {
	      os << ana[wPos-1].getWord();
	      os << " " << ana[wPos].getWord();
	      os << " __";
	    }
	    else if ( wPos == 0 ){
	      os << "__";
	      os << " " << ana[wPos].getWord();
	      os << " " << ana[wPos+1].getWord();
	    }
	    else {
	      os << ana[wPos-1].getWord();
	      os << " " << ana[wPos].getWord();
	      os << " " << ana[wPos+1].getWord();
	    }
	    if ( pos == 0 )
	      os << " __";
	    else
	      os << " " << ana[pos-1].getWord();
	    if ( pos < ana.size() )
	      os << " " << ana[pos].getWord();
	    else
	      os << " __";
	    if ( pos < ana.size()-1 )
	      os << " " << ana[pos+1].getWord();
	    else
	      os << " __";
	    if ( wPos == 0 )
	      os << " __";
	    else 
	      os << " " << ana[wPos-1].getTagHead() ;
	    os << " " << ana[wPos].getTagHead();
	    if ( wPos < ana.size() - 1 )
	      os << " " << ana[wPos+1].getTagHead();
	    else 
	      os << " __";
	    if ( pos == 0 )
	      os << " __";
	    else
	      os << " " << ana[pos-1].getTagHead();
	    if ( pos < ana.size() )
	      os << " " << ana[pos].getTagHead();
	    else
	      os << " __";
	    if ( pos < ana.size()-1 )
	      os << " " << ana[pos+1].getTagHead();
	    else
	      os << " __";
	    
	    os << " " << ana[wPos].getTagHead() << "^";
	    if ( pos < ana.size() )
	      os << ana[pos].getTagHead();
	    else
	      os << "__";
	    
	    if ( wPos > pos )
	      os << " LEFT " << wPos - pos;
	    else
	      os << " RIGHT " << pos - wPos;
	    if ( pos >= ana.size() )
	      os << " __";
	    else
	      os << " " << ana[pos].getTagMods();
	    os << "^" << ana[wPos].getTagMods();
	    os << " __" << endl;
	  }
	}
      }
    }
  };

  void createDir( const vector<mwuChunker::ana>& ana,
		    const string& fileName ){
    string pFile = fileName + ".dir.inst";
    ofstream os( pFile.c_str() );
    if ( os ){
      if ( ana.size() == 1 ){
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " __ __ __ __";
	os << " " << ana[0].getTagHead();
	os << " __ __ __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " __ __ __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^__";
	os << " __";
	os << " " << ana[0].getTagMods();
	os << " __ ROOT" << endl;
      }
      else if ( ana.size() == 2 ){
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " __ __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " __ __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " __ __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __";
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " ROOT" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " __ __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " __ __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " __ __";
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^__";
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " __";
	os << " ROOT" << endl;
      }
      else if ( ana.size() == 3 ) {
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " " << ana[2].getWord() << "^" << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __";
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " ROOT" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " " << ana[2].getWord() << "^" << ana[2].getTagHead();
	os << " __";
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " " << ana[2].getTagMods();
	os << " ROOT" << endl;
	os << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " " << ana[2].getWord() << "^" << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " " << ana[2].getTagHead() << "^__";
	os << " " << ana[1].getTagMods();
	os << " " << ana[2].getTagMods();
	os << " __";
	os << " ROOT" << endl;
      }
      else {
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " " << ana[2].getWord() << "^" << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __";
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " ROOT" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " " << ana[3].getWord();
	os << " __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " " << ana[3].getTagHead();
	os << " __";
	os << " " << ana[0].getWord() << "^" << ana[0].getTagHead();
	os << " " << ana[1].getWord() << "^" << ana[1].getTagHead();
	os << " " << ana[2].getWord() << "^" << ana[2].getTagHead();
	os << " " << ana[3].getWord() << "^" << ana[3].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " " << ana[0].getTagMods();
	os << " " << ana[1].getTagMods();
	os << " " << ana[2].getTagMods();
	os << " ROOT" << endl;
	for ( size_t i=2 ; i < ana.size() - 2 ; ++i ){
	  os << ana[i-2].getWord();
	  os << " " << ana[i-1].getWord();
	  os << " " << ana[i].getWord();
	  os << " " << ana[i+1].getWord();
	  os << " " << ana[i+2].getWord();
	  os << " " << ana[i-2].getTagHead();
	  os << " " << ana[i-1].getTagHead();
	  os << " " << ana[i].getTagHead();
	  os << " " << ana[i+1].getTagHead();
	  os << " " << ana[i+2].getTagHead();
	  os << " " << ana[i-2].getWord() << "^" << ana[i-2].getTagHead();
	  os << " " << ana[i-1].getWord() << "^" << ana[i-1].getTagHead();
	  os << " " << ana[i].getWord() << "^" << ana[i].getTagHead();
	  os << " " << ana[i+1].getWord() << "^" << ana[i+1].getTagHead();
	  os << " " << ana[i+2].getWord() << "^" << ana[i+2].getTagHead();
	  os << " " << ana[i-1].getTagHead() << "^" << ana[i].getTagHead();
	  os << " " << ana[i].getTagHead() << "^" << ana[i+1].getTagHead();
	  os << " " << ana[i-1].getTagMods();
	  os << " " << ana[i].getTagMods();
	  os << " " << ana[i+1].getTagMods();
	  os << " ROOT" << endl;
	}
	size_t pos = ana.size() - 2;
	os << ana[pos-2].getWord();
	os << " " << ana[pos-1].getWord();
	os << " " << ana[pos].getWord();
	os << " " << ana[pos+1].getWord();
	os << " __";
	os << " " << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getTagHead();
	os << " " << ana[pos].getTagHead();
	os << " " << ana[pos+1].getTagHead();
	os << " __";
	os << " " << ana[pos-2].getWord() << "^" << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getWord() << "^" << ana[pos-1].getTagHead();
	os << " " << ana[pos].getWord() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos+1].getWord() << "^" << ana[pos+1].getTagHead();
	os << " __";
	os << " " << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^" << ana[pos+1].getTagHead();
	os << " " << ana[pos-1].getTagMods();
	os << " " << ana[pos].getTagMods();
	os << " " << ana[pos+1].getTagMods();
	os << " ROOT" << endl;
	pos = ana.size() - 1;
	os << ana[pos-2].getWord();
	os << " " << ana[pos-1].getWord();
	os << " " << ana[pos].getWord();
	os << " __";
	os << " __";
	os << " " << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getTagHead();
	os << " " << ana[pos].getTagHead();
	os << " __";
	os << " __";
	os << " " << ana[pos-2].getWord() << "^" << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getWord() << "^" << ana[pos-1].getTagHead();
	os << " " << ana[pos].getWord() << "^" << ana[pos].getTagHead();
	os << " __";
	os << " __";
	os << " " << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^__";
	os << " " << ana[pos-1].getTagMods();
	os << " " << ana[pos].getTagMods();
	os << " __";
	os << " ROOT" << endl;
      }
    }
    os << endl;
  }
  
  void createRels( const vector<mwuChunker::ana>& ana,
		   const string& fileName ){
    string pFile = fileName + ".rels.inst";
    ofstream os( pFile.c_str() );
    if ( os ){
      if ( ana.size() == 1 ){
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " __ __";
	os << " " << ana[0].getTagMods();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " __ __";
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^__";
	os << " __^__^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^__^__";
	os << " __" << endl;
      }
      else if ( ana.size() == 2 ){
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " __";
	os << " " << ana[0].getTagMods();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " __";
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __^__^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead() << "^__";
	os << " __" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " __ __";
	os << " " << ana[1].getTagMods();
	os << " __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " __ __";
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^__";
	os << " __^" << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^__^__";
	os << " __" << endl;
      }
      else if ( ana.size() == 3 ){
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " " << ana[0].getTagMods();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __^__^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " __" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __";
	os << " " << ana[1].getTagMods();
	os << " __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __";
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead() << "^__";
	os << " __" << endl;
	os << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " __ __";
	os << " " << ana[2].getTagMods();
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __ __";
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " " << ana[2].getTagHead() << "^__";
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " " << ana[2].getTagHead() << "^__^__";
	os << " __" << endl;
      }
      else {
	os << "__ __";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " " << ana[0].getTagMods();
	os << " __ __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " __^__^" << ana[0].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " __" << endl;
	os << "__";
	os << " " << ana[0].getWord();
	os << " " << ana[1].getWord();
	os << " " << ana[2].getWord();
	os << " " << ana[3].getWord();
	os << " " << ana[1].getTagMods();
	os << " __";
	os << " " << ana[0].getTagHead();
	os << " " << ana[1].getTagHead();
	os << " " << ana[2].getTagHead();
	os << " " << ana[3].getTagHead();
	os << " " << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead();
	os << " __^" << ana[0].getTagHead() << "^" << ana[1].getTagHead();
	os << " " << ana[1].getTagHead() << "^" << ana[2].getTagHead() << "^" << ana[3].getTagHead();
	os << " __" << endl;
	for ( size_t i=2 ; i < ana.size() - 2 ; ++i ){
	  os << ana[i-2].getWord();
	  os << " " << ana[i-1].getWord();
	  os << " " << ana[i].getWord();
	  os << " " << ana[i+1].getWord();
	  os << " " << ana[i+2].getWord();
	  os << " " << ana[i].getTagMods();
	  os << " " << ana[i-2].getTagHead();
	  os << " " << ana[i-1].getTagHead();
	  os << " " << ana[i].getTagHead();
	  os << " " << ana[i+1].getTagHead();
	  os << " " << ana[i+2].getTagHead();
	  os << " " << ana[i-1].getTagHead() << "^" << ana[i].getTagHead();
	  os << " " << ana[i].getTagHead() << "^" << ana[i+1].getTagHead();
	  os << " " << ana[i-2].getTagHead() << "^" << ana[i-1].getTagHead() << "^" << ana[i].getTagHead();
	  os << " " << ana[i].getTagHead() << "^" << ana[i+1].getTagHead() << "^" << ana[i+2].getTagHead();
	  os << " __" << endl;
	}
	size_t pos = ana.size() - 2;
	os << ana[pos-2].getWord();
	os << " " << ana[pos-1].getWord();
	os << " " << ana[pos].getWord();
	os << " " << ana[pos+1].getWord();
	os << " __";
	os << " " << ana[pos].getTagMods();
	os << " " << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getTagHead();
	os << " " << ana[pos].getTagHead();
	os << " " << ana[pos+1].getTagHead();
	os << " __";
	os << " " << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^" << ana[pos+1].getTagHead();
	os << " " << ana[pos-2].getTagHead() << "^" << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^" << ana[pos+1].getTagHead() << "^__";
	os << " __" << endl;
	pos = ana.size() - 1;
	os << ana[pos-2].getWord();
	os << " " << ana[pos-1].getWord();
	os << " " << ana[pos].getWord();
	os << " __ __";
	os << " " << ana[pos].getTagMods();
	os << " " << ana[pos-2].getTagHead();
	os << " " << ana[pos-1].getTagHead();
	os << " " << ana[pos].getTagHead();
	os << " __ __";
	os << " " << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^__";
	os << " " << ana[pos-2].getTagHead() << "^" << ana[pos-1].getTagHead() << "^" << ana[pos].getTagHead();
	os << " " << ana[pos].getTagHead() << "^__^__";
	os << " __" << endl;
      }
      os << endl;
    }
  }

  void Parse( vector<mwuChunker::ana>& final_ana, const string& fileName ){
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
      createDir( final_ana, fileName );
      createRels( final_ana, fileName );
      createPairs( final_ana, fileName );
      gettimeofday(&endTime,0);
      addTimeDiff( prepareTime, startTime, endTime );    
#pragma omp parallel sections
      {
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  pairs->Test( fileName + ".pairs.inst", "tadpoleParser.inst.out" );
	  gettimeofday(&endTime,0);
	  addTimeDiff( pairsTime, startTime, endTime );
	}
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  dir->Test( fileName +".dir.inst", "tadpoleParser.dir.out" );
	  gettimeofday(&endTime,0);
	  addTimeDiff( dirTime, startTime, endTime );
	}
#pragma omp section
	{
	  timeval startTime;
	  timeval endTime;
	  gettimeofday(&startTime,0);
	  rels->Test( fileName + ".rels.inst", "tadpoleParser.rels.out" );
	  gettimeofday(&endTime,0);
	  addTimeDiff( relsTime, startTime, endTime );
	}
      }
      gettimeofday(&startTime,0);
      try {
	PI->parse( "tadpoleParser.inst.out",
		   "tadpoleParser.rels.out",
		   "tadpoleParser.dir.out",
		   "20",
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
