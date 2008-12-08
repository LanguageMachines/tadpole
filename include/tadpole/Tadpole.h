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


#ifndef __DEMO_OPTIONS__
#define __DEMO_OPTIONS__

#define MAX_NAMELEN 2048 

#include <ctime>
#include <sys/time.h>

using namespace std;

string prefix( const std::string&, const std::string& );
string tokenize( const string& );
string linetokenize( const string& );
void decap( string &, const string &);

void showTimeSpan( ostream&, const string&, struct timeval& );
void addTimeDiff( struct timeval&, struct timeval&, struct timeval& );

extern string myOFS;
extern int tpDebug;

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
  string getName(void) {
    return Name;
  }
  void setName( const string& n) {
    Name = n;
  };

  string getTrainFile(void) {
    return TrainFile;
  };
  void setTrainFile( const string& n ) {
    TrainFile = n;
  };

  string getTreeFile(void) {
    return TreeFile;
  };

  void setTreeFile( const string& n ) {
    TreeFile = n;
  };

  string getOptStr(void) {
    return OptStr;
  };
  void setOptStr( const string& n ) {
    OptStr = n;
  };

 private:
  string Name;
  string TrainFile;
  string TreeFile;
  string OptStr;

};

#endif
