/*
  Copyright (c) 2006 - 2008
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

#ifdef BOOST_PYTHON
#include "boost/python.hpp"
using namespace boost::python;
#endif

namespace Parser {
  string pairsFileName;
  string pairsOptions = "-a1 +D +vdb+di";
  string dirFileName;
  string dirOptions = "-a1 +D +vdb+di";
  string relsFileName;
  string relsOptions = "-a1 +D +vdb+di";

  static TimblAPI *pairs = 0;
  static TimblAPI *dir = 0;
  static TimblAPI *rels = 0;
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
#ifdef BOOST_PYTHON
    Py_Initialize();
#endif
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
    string pFile = fileName + ".pairs.inst.1";
    ofstream os( pFile.c_str() );
    if ( os ){
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
	 << ana[ana.size()-1].getTagHead() << "^ROOT ROOT ROOT ROOT^__" 
	 << " _" << endl;
      // 
      for ( size_t j=0 ; j < ana.size() ; ++j ){
	for ( size_t i=0 ; i < ana.size() ; ++i ){
	  if ( j == ana.size()-1 && i == 0 )
	    ++i;
	  if ( j == 0 && i == ana.size() - 1 )
	    continue;
	  if ( i == j )
	    continue;
	  //	  os << j << "-" << i << " ";
	  if ( j == 0 )
	    os << "__";
	  else
	    os << ana[j-1].getWord();
	  os << " " << ana[j].getWord();
	  if ( j >= ana.size() - 1 )
	    os << " __";
	  else
	    os << " " << ana[j+1].getWord();
	  if ( i == 0 )
	    os << " __";
	  else
	    os << " " << ana[i-1].getWord();
	  os << " " << ana[i].getWord();
	  if ( i >= ana.size() -1 )
	    os << " __";
	  else
	    os << " " << ana[i+1].getWord();
	  os << " ";
	  if ( j == 0 )
	    os << "__";
	  else 
	    os << ana[j-1].getTagHead() ;
	  os << " " << ana[j].getTagHead();
	  if ( j >= ana.size() -1 )
	    os << " __";
	  else
	    os << " " << ana[j+1].getTagHead();
	  if ( i == 0 )
	    os << " __";
	  else
	    os << " " << ana[i-1].getTagHead();
	  os << " " << ana[i].getTagHead();
	  if ( i >= ana.size()-1 )
	    os << " __";
	  else
	    os << " " << ana[i+1].getTagHead();
	  os << " " << ana[j].getTagHead() << "^" << ana[i].getTagHead();
	  if ( j > i )
	    os << " LEFT " << j - i;
	  else
	    os << " RIGHT " << i - j;
	  os << " " << ana[i].getTagMods() << "^" << ana[j].getTagMods()
	     << " __" << endl;
	}
      }
    }
  };

  void createDir( const vector<mwuChunker::ana>& ana,
		    const string& fileName ){
    string pFile = fileName + ".dir.inst.1";
    ofstream os( pFile.c_str() );
    if ( os ){
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
      size_t pos;
      if ( ana.size() > 3 ){
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
      }
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

  void Parse( vector<mwuChunker::ana>& final_ana, const string& fileName ){
    if ( final_ana.size() < 3 ){
      cerr << "unable to parse an analisis with < 3 words" << endl;
      return;
    }
    timeval startTime;
    timeval endTime;
    createPairs( final_ana, fileName );
    createDir( final_ana, fileName );
    string resFileName = fileName + ".result"; 
    ofstream anaFile( fileName.c_str() );
    if ( anaFile ){
      saveAna( anaFile, final_ana );
      unlink( resFileName.c_str() );
      gettimeofday(&startTime,0);    
      string cmd = string("sh ") + BIN_PATH + "/prepareParser.sh " + fileName;
      // run some python scripts to prepare the input.
      system( cmd.c_str() ); 
      if ( !isInit ){
	cerr << "Parser is not initialized!" << endl;
	exit(1);
      }
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
#ifdef BOOST_PYTHON
      string script = "/home/sloot/usr/local/lib/python2.5/site-packages/csidp.py"; 
      try {
	object global = import("__main__").attr("__dict__");
	object o = exec_file( script.c_str(), global, global );
      }
      catch( error_already_set const & ){
	PyErr_Print();
      }
#else
      cmd = string("sh ") + BIN_PATH + "/finalizeParser.sh " + fileName;
      system( cmd.c_str() );  
#endif
      gettimeofday(&endTime,0);
      addTimeDiff( csiTime, startTime, endTime );
      ifstream resFile( resFileName.c_str() );
      if ( resFile )
	readAna( resFile, final_ana );
    }
  }
}
