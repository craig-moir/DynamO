/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2009  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "cell.hpp"
#ifndef DYNAMO_CONDOR
# include <boost/iostreams/device/file.hpp>
# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/filter/bzip2.hpp>
# include <boost/iostreams/chain.hpp>
namespace io = boost::iostreams;
#endif
#include <boost/filesystem.hpp>
#include <boost/progress.hpp>
#include <boost/lexical_cast.hpp>
#include "../../extcode/xmlParser.h"
#include "../../datatypes/vector.xml.hpp"

struct CUFile: public CUCell
{
  CUFile(Vector  ndimensions, std::string fn, CUCell* nextCell):
    CUCell(nextCell),
    dimensions(ndimensions),
    fileName(fn)
  {}

  Vector  dimensions;
  std::string fileName;
  std::vector<Vector  > particleCache;
 
  virtual void initialise() 
  { 
#ifndef DYNAMO_CONDOR
    uc->initialise();
    //Open the file for XML parsing
    XMLNode xMainNode;

    if (std::string(fileName.end()-4, fileName.end()) == ".xml")
      {
	if (!boost::filesystem::exists(fileName))
	  D_throw() << "Could not open XML configuration file";
	
	std::cout << "Uncompressed XML input file " << fileName << " loading";
	xMainNode=XMLNode::openFileHelper(fileName.c_str(), "DYNAMOconfig");
      }
    else if (std::string(fileName.end()-8, fileName.end()) == ".xml.bz2")
      {
	if (!boost::filesystem::exists(fileName))
	  D_throw() << "Could not open XML configuration file";
	
	io::filtering_istream inputFile;
	inputFile.push(io::bzip2_decompressor());
	inputFile.push(io::file_source(fileName));
	//Copy file to a string
	std::string line, fileString;
	
	std::cout << "Bzip compressed XML input file found\nDecompressing file "
		 << fileName;
	
	while(getline(inputFile,line)) 
	  {
	  fileString.append(line);
	  fileString.append("\n");
	  }
	
	std::cout << "File Decompressed, parsing XML";
	fflush(stdout);
	
	XMLNode tmpNode = XMLNode::parseString(fileString.c_str());
	xMainNode = tmpNode.getChildNode("DYNAMOconfig");
      }
    else
      D_throw() << "Unrecognised extension for input file";
    
    std::cout << "Parsing XML file";
    XMLNode xSubNode = xMainNode.getChildNode("ParticleData");

    if (xSubNode.isAttributeSet("AttachedBinary")
	&& (std::toupper(xSubNode.getAttribute("AttachedBinary")[0]) == 'Y'))
      D_throw() << "This packer only works on XML config files without binary data,"
		<< " please unscramble using dynamod --text";

    unsigned long nPart = xSubNode.nChildNode("Pt");
    
    std::cout << "Loading Particle Data ";
    fflush(stdout);
    
    int xml_iter = 0;
    boost::progress_display prog(nPart);
    
    for (unsigned long i = 0; i < nPart; i++)
      {
	XMLNode xBrowseNode = xSubNode.getChildNode("Pt", &xml_iter);
	Vector  posVector;
	posVector << xBrowseNode.getChildNode("P");
	
	particleCache.push_back(posVector);
	
	++prog;
      }	

    //The file has been loaded now, just clean up the positions
    Vector  centreOPoints(0,0,0);
    BOOST_FOREACH(const Vector & vec, particleCache)
      centreOPoints += vec;

    centreOPoints /= particleCache.size();

    BOOST_FOREACH(Vector & vec, particleCache)
      vec -= centreOPoints;

    BOOST_FOREACH(Vector & vec, particleCache)
      for (size_t iDim(0); iDim < NDIM; ++iDim)
	vec[iDim] *= dimensions[iDim];
#else
    D_throw() << "Cannot use the file cell when compiled for CONDOR";
#endif
  }

  virtual std::vector<Vector  > placeObjects(const Vector & centre)
  {
    std::vector<Vector  > retval;

    BOOST_FOREACH(const Vector & position, particleCache)
      BOOST_FOREACH(const Vector & vec, uc->placeObjects(position + centre))
      retval.push_back(vec);

    return retval;
  }
};
