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
      tagHead = "";
      tagMods = "";
      lemma = "";
      morphemes = "";
      parseNum = "0";
      parseTag = "";
      isMWU = false;
    }

    ana( const string& in) {
      vector<string> elems;
      size_t i = Timbl::split_at(in, elems, myOFS);
      if ( i > 3) {
	word = elems[0];
	tag = elems[1];
	vector<string> parts;
	int num = Timbl::split_at_first_of( elems[1], parts, "()" );
	if ( num < 1 )
	  throw "dead";
	else {
	  tagHead = parts[0];
	  if ( num > 1 ){
	    vector<string> fields;
	    int size = Timbl::split_at( parts[1], fields, "," );
	    for ( int j = 0; j < size; ++j ){
	      tagMods += fields[j];
	      if ( j < size-1 )
		tagMods += '|';
	    }
	  }
	  else
	    tagMods = "__";
	}
	lemma = elems[2];
	morphemes = elems[3];
      }
      isMWU = false;
    }
    
    ~ana() {};
    
    void append( const string& s, const ana& add ){
      word += s + add.word;
      tag += s + add.tag;
      tagHead += s + add.tagHead;
      tagMods += s + add.tagMods;
      lemma += s + add.lemma;
      morphemes += s + add.morphemes;
      isMWU = true;
    }

    string getTag() const {
      return tag;
    }

    string getTagHead() const {
      return tagHead;
    }

    string getTagMods() const {
      return tagMods;
    }

    string getWord() const {
      return word;
    }
    string getLemma() const {
      return lemma;
    }
    string getParseNum() const {
      return parseNum;
    }
    string getParseTag( ) const {
      return parseTag;
    }

    void setParseNum( const std::string& num) {
      parseNum = num;
    }
    void setParseTag( const std::string& tag ) {
      parseTag = tag;
    }

    const std::string formatMWU() const;

    bool isMwu() const { return isMWU; }
  private:
    string word;
    string tag;
    string tagHead;
    string tagMods;
    string lemma;
    string morphemes;
    string parseNum;
    string parseTag;
    bool isMWU;
  };


  void init( const std::string&, const std::string& );
  void Classify(std::vector<std::string>&, std::vector<ana> &);

  void saveAna( std::ostream& , const std::vector<ana> &);
  void readAna( std::istream& , std::vector<ana> &);
}

#endif
