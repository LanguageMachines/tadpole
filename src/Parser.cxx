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

#include "config.h"
#include "timbl/TimblAPI.h"
#include "tadpole/Parser.h"

using namespace std;
using namespace Timbl;

namespace Parser {
  static TimblAPI *pairs = 0;
  static TimblAPI *dirs = 0;
  static TimblAPI *rels = 0;

  bool init( const string& dir, const string&, const string& fileName ){
    string cmd = string("rm ") + fileName + ".result";
    system( cmd.c_str() );
    pairs = new TimblAPI( "-a1 +D +vdb+di"  );
    pairs->GetInstanceBase( dir + "mbdp-tadpole-alpino.pairs.sampled.ibase" );
    dirs = new TimblAPI( "-a1 +D +vdb+di" );
    dirs->GetInstanceBase( dir + "mbdp-tadpole-alpino.dir.ibase" );
    rels = new TimblAPI( "-a1 +D +vdb+di" );
    rels->GetInstanceBase( dir + "mbdp-tadpole-alpino.rels.ibase" );
    return true;
  }
  
  void Parse( const string& fileName ){
    string cmd = string("sh ") + BIN_PATH + "/prepareParser.sh " + fileName;
    // run some python scripts to prepare the input.
    system( cmd.c_str() ); 
    if ( !pairs ){
      cerr << "Parser is not initialized!" << endl;
      exit(1);
    }
    pairs->Test( fileName + ".pairs.inst", "tadpoleParser.inst.out" );
    dirs->Test( fileName +".dir.inst", "tadpoleParser.dir.out" );
    rels->Test( fileName + ".rels.inst", "tadpoleParser.rels.out" );
    cmd = string("sh ") + BIN_PATH + "/finalizeParser.sh " + fileName;
    system( cmd.c_str() );  
  }
}
