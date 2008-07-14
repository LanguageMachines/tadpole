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

#ifndef __MWU_CHUNKER__
#define __MWU_CHUNKER__

namespace mwuChunker {

  class ana {

    friend std::ostream& operator<< (std::ostream&, const ana& );

  public:
    ana() {
      word = "";
      tag = "";
      lemma = "";
      morphemes = "";
    }

    ana( const string& in) {
      vector<string> elems;
      size_t i = Timbl::split_at(in, elems, myOFS);
      if ( i > 3) {
	word = elems[0];
	tag = elems[1];
	lemma = elems[2];
	morphemes = elems[3];
      }
    }
    
    ~ana() {};
    
    void append( const string& s, const ana& add ){
      word += s + add.word;
      tag += s + add.tag;
      lemma += s + add.lemma;
      morphemes += s + add.morphemes;
    }

    string getTag() const {
      return tag;
    }

  private:
    string word;
    string tag;
    string lemma;
    string morphemes;
  };


  inline std::ostream& operator<< (std::ostream& os, const ana& a ){
    os << a.word << myOFS << a.tag << myOFS 
       << a.lemma << myOFS << a.morphemes;
    return os;
  }

  void init( const std::string&, const std::string& );
  void Classify(std::vector<std::string>&, std::vector<ana> &);

}

#endif
