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
#include "tadpole/Parser.h"

using namespace std;
using namespace Timbl;

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
    bool happy = false;
    cerr << "initiating parser ... " << endl;
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
    return happy;
  }
  
  void Parse( const string& fileName ){
    string cmd = string("sh ") + BIN_PATH + "/prepareParser.sh " + fileName;
    // run some python scripts to prepare the input.
    system( cmd.c_str() ); 
    if ( !isInit ){
      cerr << "Parser is not initialized!" << endl;
      exit(1);
    }
    pairs->Test( fileName + ".pairs.inst", "tadpoleParser.inst.out" );
    dir->Test( fileName +".dir.inst", "tadpoleParser.dir.out" );
    rels->Test( fileName + ".rels.inst", "tadpoleParser.rels.out" );
    cmd = string("sh ") + BIN_PATH + "/finalizeParser.sh " + fileName;
    system( cmd.c_str() );  
  }
}
