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


#ifndef __DEMO_OPTIONS__
#define __DEMO_OPTIONS__

#define MAX_NAMELEN 2048 

#include <set>
#include "tadpole/unicode_utils.h"

std::string prefix( const std::string&, const std::string& );
bool existsDir( const std::string& );
std::string tokenize( const std::string& );
std::string linetokenize( const std::string& );
void decap( std::string &, const std::string &);
void decap( UnicodeString &, const std::string &);

void getFileNames( const std::string&, std::set<std::string>& );

extern std::string myOFS;
extern int tpDebug;
extern bool keepIntermediateFiles;
extern bool doServer;

class TimerBlock{
public:
  Common::Timer parseTimer;
  Common::Timer tokTimer;
  Common::Timer mblemTimer;
  Common::Timer mbmaTimer;
  Common::Timer mwuTimer;
  Common::Timer tagTimer;
  Common::Timer prepareTimer;
  Common::Timer pairsTimer;
  Common::Timer relsTimer;
  Common::Timer dirTimer;
  Common::Timer csiTimer;
};


class DemoOptions {
 public:
  DemoOptions(){
    Name = "";
    TrainFile = "";
    TreeFile = "";
    OptStr = "";

  };
  ~DemoOptions(){};

  //methods
  std::string getName(void) {
    return Name;
  }
  void setName( const std::string& n) {
    Name = n;
  };

  std::string getTrainFile(void) {
    return TrainFile;
  };
  void setTrainFile( const std::string& n ) {
    TrainFile = n;
  };

  std::string getTreeFile(void) {
    return TreeFile;
  };

  void setTreeFile( const std::string& n ) {
    TreeFile = n;
  };

  std::string getOptStr(void) {
    return OptStr;
  };
  void setOptStr( const std::string& n ) {
    OptStr = n;
  };

 private:
  std::string Name;
  std::string TrainFile;
  std::string TreeFile;
  std::string OptStr;

};

#endif
