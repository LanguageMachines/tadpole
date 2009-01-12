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
                                    
#ifndef __MDMA_MOD__
#define __MBMA_MOD__

using namespace std;

namespace Mbma {

class MBMAana {
  friend std::ostream& operator<< ( std::ostream& , const MBMAana& );
 public:
  MBMAana() {
    tag = "";
    infl = "";
    morphemes = "";
    description = "";    
  };
  MBMAana( const string& in) {
    vector<string> elems;
    size_t i = Timbl::split_at(in, elems, " " );
    if ( i > 3) {
      tag = elems[0];
      infl = elems[1];
      morphemes = elems[2];
      for ( size_t j =3; j < i; ++j){
	description += elems[j];
	if ( (j + 1) < i ) 
	  description += " ";
      }
    }
  };

  
  ~MBMAana() {};
  
  string getTag() const {
    return tag;
  };

  string getInflection() const {
    return infl;
  };
 
  string getMorph() const {
    return morphemes;
  };
 
 private:
  string tag;
  string infl;
  string morphemes;
  string description;

};

 inline std::ostream& operator<< ( std::ostream& os, const MBMAana& a ){
   os <<"\t" << a.tag << " " 
      << a.infl << " >>>> " << a.morphemes << " <<<< [[[[ "
      << a.description << " ]]]] ";
   return os;
 }
	      
  void init( DemoOptions *MOpts = NULL);
  void init( const std::string&, const std::string& );

  void Classify( const string&, vector<MBMAana> &);
}

#endif
