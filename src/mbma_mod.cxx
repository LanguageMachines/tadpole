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

/* mbma - CGI / C script for handling MBMA demo i/o


   handles both form fetching and socket communications

   ILK / Antal, 18 May 1999 

   Contains code snippets from sockhelp.c and sockhelp.h by Vic Metcalfe 
   and URL parsing by Sam Thomas. Thanks Bertjan and Jakub. 
*/

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <timbl/TimblAPI.h>

#include "tadpole/Tadpole.h"
#include "tadpole/unicode_utils.h"
#include "tadpole/mbma_mod.h"

using namespace std;
using namespace Timbl;

int mbaDebug = 0; // debugging on (1) or off (0)

namespace Mbma {
  const long int LEFT =  6; // left context
  const long int RIGHT = 6; // right context
  const unsigned int NRTAGS = 13; // number of CELEX POS tags
  const int NRINFS = 21; // number of inflection types

  enum mbmaMode { normalMode, daringMode, lemmatizerMode };
  enum mbmaMode mode = daringMode;
  
  string punctuation = "?...,:;\\'`(){}[]%#+-_=/!";

  string MTreeFilename = "dm.igtree"; //default tree name
  TimblAPI *MTree = 0;
  string sep = " "; // "&= " for cgi 

  // std. stuff to parse command line, read settingsfile, etc.

  bool readsettings( const string& cDir, const string& fname) {
    ifstream setfile( fname.c_str(), ios::in);
    if(setfile.bad()){
      return false;
    }
    char SetBuffer[512];
    char tmp[MAX_NAMELEN];
    while(setfile.getline(SetBuffer,511,'\n')) {
      switch (SetBuffer[0]) {
	/*
	  case 'n':
	  sscanf(SetBuffer, "n %s", tmp);
	  //      cout << "name supplied: " << tmp << endl;
	  DOpts->setName(tmp);
	  //      cout << "name stored: " << DOpts->getName();
	  break;
	*/
      case 'd':
	sscanf(SetBuffer, "d %d", &mbaDebug );
	break;
      case 't':
	sscanf(SetBuffer, "t %s", tmp);
	MTreeFilename = prefix( cDir, tmp );
	break;
      case 'm':{
	int itmp;
	sscanf(SetBuffer, "m %d", &itmp);
	switch ( itmp ){
	case 0: mode = normalMode;
	  break;
	case 1: mode = daringMode;
	  break;
	case 2: mode = lemmatizerMode;
	  break;
	}
      }
	break;
      default:
	*Log(theErrLog) << "Unknown option in settingsfile, (ignored)\n"
	       << SetBuffer << " ignored." <<endl;
	break;
      }
    }
    return true;
  }
  
  void init(DemoOptions *MOpts) {
    if (MOpts) {
      MTreeFilename = MOpts->getTreeFile();
    }
    
    //Read in (igtree) data
    MTree = new TimblAPI("-a 1 +vs -vf");
    MTree->GetInstanceBase(MTreeFilename);
    return;
  }
  
  void init( const string& cDir, const string& fname) {
    *Log(theErrLog) << "Initiating morphological analyzer...\n";
    mbaDebug = tpDebug;
    if ( !readsettings( cDir, fname) ) {
      *Log(theErrLog) << "Cannot read MBMA settingsfile " << fname << endl;
      exit(1);
    }
    
    //Read in (igtree) data
    MTree = new TimblAPI("-a 1 +vs -vf");
    MTree->GetInstanceBase(MTreeFilename);
    return;
  }
  
  void cleanUp(){
    // *Log(theErrLog) << "cleaning up MBMA stuff " << endl;
    delete MTree;
    MTree = 0;
  }
  /* mbma stuff */
  
  size_t make_instance( const UnicodeString& word, 
			vector<UnicodeString> &insts) {
    if (mbaDebug > 2)
      cout << "word: " << word << "\twl : " << word.length() << endl;
    for( long i=0; i < word.length(); ++i ) {
      if (mbaDebug > 2)
	cout << "itt #:" << i << endl;
      UnicodeString inst;
      for ( long j=i ; j <= i + RIGHT + LEFT; ++j ) {
	if (mbaDebug > 2)
	  cout << " " << j-LEFT << ": ";
	if ( j < LEFT || j >= word.length()+LEFT )
	  inst += "_,";
	else {
	  if (word[j-LEFT] == ',' )
	    inst += "C";
	  else
	    inst += word[j-LEFT];
	  inst += ",";
	}
	if (mbaDebug > 2)
	  cout << " : " << inst << endl;
      }
      inst += "?";
      if (mbaDebug > 2)
	cout << "inst #" << i << " : " << inst << endl;
      // classify res
      insts.push_back( inst );
      // store res
    }
    //return stored classes
    return insts.size();
  }
  
  string extract_from( const string& line, char from ){
    size_t pos = line.find( from );
    if ( pos != string::npos )
      return line.substr( pos+1 );
    else
      return "";
  }
  
  string extract( const string& line, size_t start, char to ){
    return line.substr( start, line.find( to , start ) - start );
  }
  
  string extract( const string& line, size_t start, const string& to ){
    return line.substr( start, line.find_first_of( to , start ) - start );
  }
  
  string find_class( int step, size_t pos,
		     const vector<string>& classes, int nranal ){
    string result;
    if ( nranal > 1 ){
      if ( classes[pos].find("|") != string::npos ) { 
	int thisnranal=1;
	for ( size_t l=0; l< classes[pos].length(); l++)
	  if (classes[pos][l]=='|')
	    thisnranal++;
	if (thisnranal==nranal) { 
	  result = "";
	  size_t l=0;
	  for ( int m=0; m<step; m++) { 
	    while (classes[pos][l]!='|')
	      l++;
	    l++;
	  }
	  while ((l< classes[pos].length())&&
		 (classes[pos][l]!='|')) { 
	    result += classes[pos][l];
	    l++;
	  }
	}
	else {
	  cerr << "PANIEK!!!!" << endl;
	  cerr << "blijkbaar komt het komt voor" << endl;
	  cerr << "Stuur de voorbeeldzin bar Ko!" << endl;
	  result = "0";
	}
      }
      else
	result = classes[pos];
    }
    else
      result = classes[pos];
    return result;
  }
  
  bool check_spell_change( string& this_class, 
			   string& deletestring,
			   string& insertstring ){
    /* spelling changes? */
    
    size_t pos = this_class.find("+");
    if ( pos != string::npos ) { 
      pos++;
      if (this_class[pos]=='D') { // delete operation 
	deletestring = extract( this_class, pos+1, '/' );
      }
      else if (this_class[pos]=='I') {  //insert operation
	insertstring = extract( this_class, pos+1, '/' );
      }
      else if (this_class[pos]=='R') { // replace operation 
	deletestring = extract( this_class, pos+1, '>' );
	pos += deletestring.length()+1;
	insertstring = extract( this_class, pos+1, '/' );
      }
      /* spelling change done; remove from this_class */
      string temp = extract( this_class, 0, '+' );
      pos =  this_class.find("/");
      if ( pos != string::npos )
	temp += this_class.substr( pos );
      this_class = temp;
    }
    if ( deletestring == "eeer" )
      deletestring = "eer";
    /* exceptions */
    bool eexcept = false;
    if ( deletestring == "ere" ){ 
      deletestring = "er";
      eexcept = true;
    }
    return eexcept;
  }
  
  
  bool Step1( int step, string& previoustag,
	      UnicodeString& output, const UnicodeString& word, int nranal,
	      const vector<string>& classes, const string& basictags ) {
    output ="[";
    size_t tobeignored=0;
    bool infpresent = false;
    bool lemmasuppress = false;
    string this_class;
    for ( long k=0; k< word.length(); ++k ) { 
      this_class = find_class( step, k, classes, nranal );
      string deletestring;
      string insertstring;
      bool eexcept = check_spell_change( this_class, deletestring, insertstring);
      if ( mode == lemmatizerMode )
	eexcept = false;
      char test_1 = classes[k][0];
      char test_2;
      if ( k< word.length()-1 )
	test_2 = classes[k+1][0];
      else
	test_2 = '#';
      
      if (!tobeignored)
	tobeignored=0;
      if ( !( mode==lemmatizerMode &&
	      k>0 &&
	      ( basictags.find(test_1) != string::npos ||
		basictags.find(test_2) != string::npos ||
		( test_1 == '0' && test_2 == '0' ))
	      ) ){
	// insert the deletestring :-) 
	output += UTF8ToUnicode( deletestring );
	// delete the insertstring :-) 
	if (( !tobeignored ) &&
	    ( insertstring != "ge" ) &&
	    ( insertstring != "be" ) )
	  tobeignored = insertstring.length();
      }
      
      /* encountering POS tag */
      if ( basictags.find(this_class[0]) != string::npos &&
	   this_class != "PE" ) { 
	lemmasuppress = false;
	if (k>0) { 
	  output += "]" + UTF8ToUnicode(previoustag) + "[";
	}
	previoustag = this_class;
      }
      else { 
	if ( this_class[0] !='0' ){ 
	  if (k>0) { 
	    output += "]" + UTF8ToUnicode(previoustag);
	  }
	  output += "[";
	  previoustag = "i";
	  if (!( mode==lemmatizerMode &&
		 ( insertstring =="ge" ||
		   insertstring == "be" ) ))
	    previoustag += this_class;
	  if (mode==lemmatizerMode) { 
	    lemmasuppress = true;
	    infpresent = true;
	  }
	}
      }
      /* copy the next character */
      if ( eexcept ) { 
	output += "e";
	eexcept = false;
      }
      
      if (!( tobeignored || lemmasuppress )) { 
	output += word[k];
      }
      else
	if (tobeignored)
	  tobeignored--;
      
    }
    output += "]" + UTF8ToUnicode(previoustag);
    
    /* without changes, but with inflection */
    if ( this_class.find("/") != string::npos &&
	 this_class != "E/P" ) { 
      string inflection = "i";
      inflection += extract_from( this_class, '/' );
      output += "[]";
      output += UTF8ToUnicode(inflection);
    }
    
    /* remove initial double brackets; ooh this is crappy */
    if ((output[0]=='[')&&
	(output[1]=='[') )
      output.remove(0,1);
    return infpresent;
  }
  
  size_t two_hooks_left( const string& s, size_t start ){
    size_t m = start;
    while ( m>0 && s[m]!=']' )
      m--;
    while ( m>0 && s[m]==']' )
      m--;
    while ( m>0 && s[m]!=']' )
      m--;
    return ++m;
  }
  
  bool fix_left( const string& affixleft, string& output, 
		 size_t m, size_t& insertleft ) {
    bool result = true;
    if ( !affixleft.empty() ) { 
      for ( size_t l=0; l< affixleft.length() && result ; ++l ) { 
	m = two_hooks_left( output, m );
	size_t mempos=m;
	string a_class = extract( output, m, '[' );
	m += a_class.length();
	if ( a_class[0] != affixleft[l] ) { 
	  /* overrule the tag of the left-affix: this is
	     usually a better guess than the tag originally
	     predicted! */
	  if ( a_class.length() == affixleft.length()) { 
	    if (mbaDebug){
	      cout << "correcting class " << a_class
		   << " to " << affixleft;
	      cout << endl;
	    }
	    // code updated heavy but it seems we never come here???
	    cout << "HEY, at some seldomly tested point now!!!" << endl;
	    output.replace( mempos, affixleft.length(), affixleft );
	    // old version
	    //	  for ( size_t k = mempos; k < mempos + affixleft.length(); ++k )
	    //	    output[k]=affixleft[k-mempos];
	  }
	  else /* fail to do anything link-wise */
	    result = false;
	}
	if ( result ) { 
	  /* update insertleft */
	  /* first move up to the first ']' */
	  while ((output[insertleft]!=']')&&
		 (insertleft>0))
	    insertleft--;
	  insertleft--;
	  /* then go to the beginning of the next chunk,
	     which may be nested */
	  int bracketlevel=1;
	  while ((bracketlevel!=0)&&
		 (insertleft>0)) { 
	    if (output[insertleft]=='[')
	      bracketlevel--;
	    if (output[insertleft]==']')
	      bracketlevel++;
	    insertleft--;
	  }
	  if (insertleft>0)
	    insertleft++;
	  if (mbaDebug){
	    cout << affixleft[l] 
		 << " found on the left! updated insertleft to "
		 << insertleft;
	    cout << endl;
	  }
	}
      }
    }
    return result;
  }
  
  bool fix_right( const string& affixright, const string& output, 
		  size_t& insertright ){
    bool result = true;
    if ( !affixright.empty() ) { 
      insertright++;
      for ( size_t l=0;  l< affixright.length() && result ; ++l ) { 
	int bracketlevel = 1;
	while ( bracketlevel !=0 &&
		insertright < output.length() ) { 
	  if (output[insertright]==']')
	    bracketlevel--;
	  else if (output[insertright]=='[')
	    bracketlevel++;
	  insertright++;
	}
	string a_class = extract( output, insertright, '[' );
	insertright += a_class.length();
	if ( a_class[0]!=affixright[l]) { 
	  result = false;
	}
	if (affixright[l])
	  if (mbaDebug){
	    cout << affixright[l] 
		 << " found on the right! updated insertright to "
		 << insertright;
	    cout << endl;
	  }
      }
    }
    return result;
  }
  
  void Step3( string& output, string& affixleft ){
    // resolve all clearly resolvable selection tags of affixes 
    bool resolved = false;
    while (!resolved) { 
      int nrchanges=0;
      size_t k=0;
      while ( k < output.length() ){ 
	size_t insertleft = k;
	k = output.find( ']', k );
	if ( k == string::npos )
	  break;
	k = output.find_first_not_of( ']', k );
	if ( k == string::npos )
	  break;
	size_t changetag = k;
	string a_class = extract( output, k, '[' );
	k += a_class.length();
	if ( a_class.find("_") != string::npos ) { 
	  size_t insertright=k;
	  if (mbaDebug){
	    cout << "starting with insertleft " << insertleft
		 <<" and insertright " << insertright;
	    cout << endl;
	  }
	  string affixtarget = extract( a_class, 0, '_' );
	  size_t l = affixtarget.length() + 1;
	  affixleft = extract( a_class, l, '*' );
	  l += affixleft.length() + 1;
	  string affixright =  extract( a_class, l, '<' );
	  if (mbaDebug){
	    cout << "thisclass >" << a_class 
		 << "<, target >" << affixtarget 
		 << "<, left >" << affixleft 
		 << "<, right >" << affixright << "<";
	    cout << endl;
	  }
	  /* go left */
	  bool affixleftok = fix_left( affixleft, output, k, insertleft );
	  /* go right */
	  
	  bool affixrightok = fix_right( affixright, output, insertright );
	  
	  /* when the affix selections match its context, 
	     rework the bracketing and the labeling */
	  if ( affixleftok && affixrightok ) { 
	    nrchanges++;
	    string helpoutput;
	    for ( size_t i=0; i < output.length(); ++i ) { 
	      if ( i == insertleft )
		helpoutput += "[";
	      if ( i == changetag )
		while ( output[i]!='[' &&
			i < output.length() )
		  ++i;
	      if ( i == insertright ) { 
		helpoutput += string("]") + affixtarget[0];
	      }
	      helpoutput += output[i];
	    }
	    output = helpoutput;
	    if (mbaDebug){
	      cout << "intermediate analysis 2: " << output;
	      cout << endl;
	    }
	  }
	  else
	    if (mbaDebug){
	      cout << "affix selection matching failed. will try later.";
	      cout << endl;
	    }
	}
      }
      if ( output.find("_") != string::npos  &&
	   nrchanges > 0 )
	resolved = false;
      else
	resolved = true;
    }
  }
  
  
  string change_tag( const char ch ){
    string newtag;
    switch( ch ){
    case 'm':
    case 'e':
    case 'd':
    case 'G':
    case 'D':
      newtag = "N";
      break;
    case 'P':
    case 'C':
    case 'S':
    case 'E':
      newtag = "A";
      break;
    case 'i':
    case 'p':
    case 't':
    case 'v':
    case 'g':
    case 'a':
      newtag = "V";
      break;
    default:
      break;
    }
    return newtag;
  }
  
  void Step4( string& output, 
	      const string& affixleft, const string& basictags ) {
    // resolve all clearly resolvable implicit selections of inflectional tags
    size_t pos = 0;
    while (  pos < string::npos ){
      pos = output.find( ']', pos );
      if ( pos != string::npos )
	pos = output.find_first_not_of( "]", pos ); 
      if ( pos != string::npos ){
	string this_class = extract( output, pos, '[' );
	pos += this_class.length();
	if ( this_class[0]=='i') { 
	  if (mbaDebug){
	    cout << "starting with insertright " << pos;
	    cout << endl;
	    cout << "thisclass >" << this_class << "<" << endl;
	  }	  
	  bool affixleftok = true;
	  if ( !affixleft.empty() ) { 
	    for ( size_t l=0; ( l< affixleft.length() && affixleftok ); ++l ) { 
	      size_t m = two_hooks_left( output, pos );
	      this_class = extract( output, m, '[' );
	    }
	  }
	  
	  /* given the specific selections of certain inflections,
	     change the tag!  */
	  string newtag = change_tag( this_class[1] );
	  
	  /* apply the change. Remember, the idea is that an inflection is
	     far more certain of the tag of its predecessing morpheme than
	     the morpheme itself. This is not always the case, but it works  */
	  if ( !newtag.empty() ) {
	    if ( mbaDebug  ){
	      cout << "selects " << newtag;
	      cout << endl;
	    }
	    /* go left */
	    size_t m = two_hooks_left( output, pos );
	    if ( basictags.find(output[m]) != string::npos )
	      output[m]=newtag[0];
	  }
	}
      }
    }
    if (mbaDebug){
      cout << "intermediate analysis 4: " << output;
      cout << endl;
    }
  }
  
  // initialise tag/inf names and codes - stoopid 
  
  string tagname_code [][2] = 
    { { "noun", "N" },
      { "adjective", "A" },
      { "quantifier/numeral", "Q" },
      { "verb", "V" },
      { "article", "D" },
      { "pronoun", "O" },
      { "adverb", "B" },
      { "preposition", "P" },
      { "conjunction", "Y" },
      { "interjection", "I" },
      { "unanalysed", "X" },
      { "expression part", "Z" },
      { "proper noun", "PN" }
    };

  string infname_code [][2] =
    { 
      { "separated", "s" },
      { "singular", "e" },
      { "plural", "m" },
      { "diminutive", "d" },
      { "genitive", "G" },
      { "dative", "D" },
      { "positive", "P" },
      { "comparative", "C" },
      { "superlative", "S" },
      { "suffix-e", "E" },
      { "infinitive", "i" },
      { "participle", "p" },
      { "present tense", "t" },
      { "past tense", "v" },
      { "1st person", "1" },
      { "2nd person", "2" },
      { "inversed", "I" },
      { "3rd person", "3" },
      { "imperative", "g" },
      { "subjunctive", "a" },
      { "", "X" }
    };    
  
  void add_last( const string& output, string& superoutput ){
  /* go back to the last non-inflectional
     tag and stick it at the end */
    size_t pos = output.length()-1;
    bool found = false;
    string thisclass;
    while (!found) { 
      while ( output[pos]!=']' &&
	      pos > 0 )
	--pos;
      size_t m=pos;
      while ( output[pos]==']' &&
	      pos < output.length() )
	++pos;
      thisclass = extract( output, pos, "[]" );
      if (mbaDebug){
	cout << "final tag " << thisclass;;
	cout << endl;
      }
      if (thisclass[0]!='i')
	found=true;
      else { 
	pos = m;
	while ( output[pos]==']' &&
		pos >0 )
	  --pos;
      }
    }
    if ( thisclass.find("_") != string::npos ) {
      string affixtarget = extract( thisclass, 0, '_' );
      size_t l=0;
      while ((l<NRTAGS)&&( tagname_code[l][1] != affixtarget ) )
	l++;
      string tmp;
      if (l==NRTAGS) {
	if (mbaDebug)
	  cout << "added X 4" << endl;
	
	tmp += "X ";
	superoutput += " unknown ";
      }
      else {
	tmp += tagname_code[l][1] + string(" ");	    
	if (mbaDebug)
	  cout << "added (4) " << tagname_code[l][0] << endl;
	
	superoutput += " ";
	superoutput += tagname_code[l][0];
      }
      tmp += superoutput;
      superoutput = tmp;
      if (mbaDebug)
        cout << "superoutput now (5): |" << superoutput << "|" << endl;
    }
    else { 
      if ( thisclass.find("/") != string::npos ) { 
	thisclass = extract( thisclass, 0, '/' );
      }
      size_t l=0;
      while ((l<NRTAGS)&& tagname_code[l][1] != thisclass )
	l++;
      string tmp;
      if (l==NRTAGS) {
	if (mbaDebug)
	  cout << "added X 5" << endl;
	
	tmp += "X ";
	superoutput += " unknown ";
      }
      else {
	tmp += tagname_code[l][1] + " ";
	if (mbaDebug)
	  cout << "added (5) " << tagname_code[l][0] << endl;
	
	superoutput += " ";
	superoutput += tagname_code[l][0];
      }
      tmp += superoutput;
      superoutput = tmp;
    }
  }
  
  string inflectAndAffix( const string& output ){
    
    string result;
    size_t k=0;
    while ( k< output.length() ) { 
      string tmp1 = extract( output, k, ']' );
      result += tmp1;
      if (mbaDebug)
	cout << "result now (1): |" << result << "|" << endl;
      k += tmp1.length();
      while ( output[k]==']' &&
	      k< output.length() ) { 
	result += output[k];
	k++;
      }
      string this_class = extract(output, k, "[]" );
      k += this_class.length();
      if (mbaDebug) {
	cout << "unpacking thisclass "<< this_class;
	cout << endl;
      }
      if ( !this_class.empty() &&
	   this_class.find("_") == string::npos ) { 
	if ((this_class[0]=='i') && (result[0]=='['))
	  {
	    //if (!((this_class == "ipv") &&
	    //	     (result[0]=='p')&&
	    //	     (result[1]=='v')))
	    {
	      string tmp;
	      for ( size_t l=1; l< this_class.length(); l++) {
		if (this_class[l]!='/') { 
		  int m=0;
		  while ((m<NRINFS)&&
			 this_class[l]!=infname_code[m][1][0] )
		    m++;
		  if (m==NRINFS) {
		    if (mbaDebug)
		      cout << "added X 1" << endl;
		    tmp += "X";
		    result += "unknown ";
		  }
		  else { 
		    // BJ: prepend tags for later processing
		    if (mbaDebug)
		      cout << "added (1) " << infname_code[m][0] << endl;
		    tmp += infname_code[m][1];
		    //result += infnames[m] + " ";
		  }
		}
	      }
	      tmp += string(" ") + result;
	      result = tmp;
	    }
	    if (mbaDebug)	
	      cout << "result now (2): |" << result << "|" << endl;
	  }
	else { 
	  if (mode==daringMode) { 
	    if ( this_class.find("/") != string::npos ) { 
	      this_class = extract( this_class, 0, '/' );
	    }
	    size_t l=0;
	    while ((l<NRTAGS)&& tagname_code[l][1] != this_class )
	      l++;
	    string tmp;
	    if (l==NRTAGS) {
	      if (mbaDebug)
		cout << "added X 2" << endl;
	      
	      result += " unknown ";
	      tmp += "X ";
	    }
	    else { 
	      tmp += tagname_code[l][1] + " ";
	      if (mbaDebug)
		cout << "added (2) " << tagname_code[l][0] << endl;
	      
	      // BJ: brackets added, looks OK, but not sure
	      result += " " + tagname_code[l][0];
	    }
	    tmp += result;
	    result = tmp;
	    if (mbaDebug)
	      cout << "result now (3): |" << result << "|" << endl;
	  }
	}
      }
      else { 
	if ( this_class.find("_") != string::npos &&
	     (mode==daringMode)) {
	  string affixtarget = extract( this_class, 0, '_' );
	  size_t l=0;
	  while ((l<NRTAGS)&&( tagname_code[l][1] != affixtarget ) )
	    l++;
	  string tmp;
	  if (l==NRTAGS) {
	    if (mbaDebug)
	      cout << "added X 3" << endl;
	    tmp += "X ";
	    result += " unknown ";
	  }
	  else { 
	    tmp += tagname_code[l][1] + string(" ");
	    
	    if (mbaDebug)
	      cout << "added (3) " << tagname_code[l][0] << endl;
	    
	    result += " ";
	    result += tagname_code[l][0];
	  }
	  tmp += result;
	  result = tmp;
	  if (mbaDebug)
	    cout << "result now (4): |" << result << "|" << endl;
	}
      }
    }
    
    // go back to the last non-inflectional
    // tag and stick it at the end
    add_last( output, result );
    return result;
  }

  string finalize( const UnicodeString& word, 
		   const string& in, bool infpresent ){
    string line = in;
    // remove empty bracketed elements 
    string::size_type pos = line.find( "[]" );
    while ( pos != string::npos ){
      line.erase(pos,2);
      pos = line.find( "[]" );
    }
    if (mode==lemmatizerMode) { 
      if (!infpresent) { 
	cout << " ";
	cout << word << " ";
	size_t pos1 = line.find( '<' );
	while ( pos1 < line.length() ){ 
	  cout << line[pos1];
	  ++pos1;
	}
      }
      else { 
	cout << "  ";
	for ( size_t pos1=0; pos1 < line.length(); ++pos1 )
	  if ( !( line[pos1]=='[' ||
		  line[pos1]==']' ) )
	    cout << line[pos1];
      }
      cout << endl;
    }
    else {
      line += "\n";
    }
    return line;
  }
  
  /* "based on mbma.c" is an understatement. The postprocessing is going
     to be a copy'n'paste from mbma.c for now, since nobody understands
     the orig code anymore, and there is no time to relearn it now. So
     it's just going to have a new, and more flexible
     front-end/wrapper/whatever.
  */
  
  string mbma_bb( const UnicodeString& word, const vector<string>& classes ) { 
    
    string basictags = "NAQVDOBPYIXZ";
    
    /* determine the number of alternative analyses */
    int nranal=1;
    for ( long j=0; j< word.length(); j++)
      if ( classes[j].find("|") != string::npos ) { 
	int thisnranal=1;
	for ( size_t k=0; k< classes[j].length(); ++k )
	  if (classes[j][k]=='|')
	    thisnranal++;
	if (thisnranal>nranal)
	  nranal=thisnranal;
      }
    
    /* then for each analysis, produce a labeled bracketed string */
    
    /* produce a basic bracketed string, taking care of
       insertions, deletions, and replacements */
    
    string result;
    for ( int step=nranal-1; step>=0; --step ) { 
      UnicodeString uOutput;
      string previoustag;
      bool infpresent = Step1( step, previoustag, uOutput, word, 
			       nranal, classes, basictags );
      string output = UnicodeToUTF8( uOutput);
      if (mbaDebug){
	cout << "intermediate analysis 1: " << output;
	cout << endl;
      }
      
      string affixleft;
      if (mode==daringMode)
	Step3( output, affixleft );
      if (mbaDebug){
	cout << "intermediate analysis 3: " << output;
	cout << endl;
      }
      
      // STEP 4
      Step4( output, affixleft, basictags );
      /* finally, unpack the tag and inflection names to make everything readable */
      result += inflectAndAffix( output );
      result = finalize( word, result, infpresent );
      
    }
    //  }
    if (mbaDebug)
      cout << "bj_out: " << result << endl << endl;
    return result;
  }
  
  void Classify( const string& inword,vector<MBMAana> &res) {
    if (mbaDebug)
      cout << "mbma::Classify starting with " << inword << endl;
    vector<string> wstr;
    string tag;
    string word;
    int num = split_at( inword, wstr, "/");
    
    // be robust against words with forward slashes
    if ( num > 1 ){
      tag = wstr[num-1];
      word = "";
      for (int i=0; i<num-2; i++)
        word += wstr[i] + '/';
      word += wstr[num-2];
    }
    else {
      //fixed handling of slashes (Maarten van Gompel)
      word = "/";
      tag = wstr[0];
      //tag = wstr[1];
      //word = wstr[0];
    }
    if (mbaDebug)
      cout << "word: " << word << " tag: " << tag << endl;
    
    UnicodeString uWord = word.c_str();
    decap( uWord, tag);
    
    vector<UnicodeString> uInsts;
    size_t num_insts = make_instance( uWord, uInsts );
    vector<string> classes;
    for( size_t i=0; i < num_insts; ++i ) {
      string cl = UnicodeToUTF8(uInsts[i]);
      string ans;
      MTree->Classify( cl, ans);
      classes.push_back( ans);
    }
    // fix for 1st char class ==0
    if ( classes[0] == "0" )
      classes[0] = "X";
    
    string bb_out = mbma_bb( uWord, classes );
    vector<string> anas;
    size_t n_anas = split_at( bb_out, anas, "\n");
    for ( size_t i=0; i < n_anas; i++) {
      MBMAana tmp(anas[i]);
      res.push_back( tmp);
    }
  }
  
  ostream& operator<< ( ostream& os, const MBMAana& a ){
    os <<"\t" << a.tag << " " 
       << a.infl << " >>>> " << a.morphemes << " <<<< [[[[ "
       << a.description << " ]]]] ";
    return os;
  }
  
}

