/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.dynamomd.org
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include <boost/range.hpp>
#include <iterator>
#include <tr1/memory>

namespace magnet { namespace xml { class Node; class XmlStream; } }

namespace dynamo { 
  using std::tr1::shared_ptr;
  class Simulation; 
  class Particle;

  class Range
  {
  public:

    class iterator
    {
      friend class Range;

      iterator(unsigned long nPos, const Range* nRangePtr):
	pos(nPos), rangePtr(nRangePtr) {}

    public:

      inline bool operator==(const iterator& nIT) const
      { return nIT.pos == pos; }

      inline bool operator!=(const iterator& nIT) const
      { return nIT.pos != pos; }

      inline iterator operator+(const unsigned long& i) const
      { return iterator(pos+i, rangePtr); }

      inline iterator operator-(const unsigned long& i) const
      { return iterator(pos-i, rangePtr); }
    
      inline iterator& operator++()
      { ++pos; return *this; }

      inline iterator& operator++(int)
      { ++pos; return *this; }

      inline size_t operator*() const
      { return (*rangePtr)[pos]; }

      typedef size_t difference_type;
      typedef size_t value_type;
      typedef size_t reference;
      typedef size_t* pointer;
      typedef std::bidirectional_iterator_tag iterator_category;

    private:
      size_t pos;
      const Range* rangePtr;
    };

    typedef iterator const_iterator;

    virtual ~Range() {};

    virtual bool isInRange(const Particle&) const = 0;

    virtual void operator<<(const magnet::xml::Node&) = 0;  

    virtual unsigned long size() const = 0;

    virtual unsigned long operator[](unsigned long) const = 0;

    virtual unsigned long at(unsigned long) const = 0;

    static Range* getClass(const magnet::xml::Node&, const dynamo::Simulation * Sim);

    inline friend magnet::xml::XmlStream& operator<<(magnet::xml::XmlStream& XML,
						     const Range& range)
    { range.outputXML(XML); return XML; }

    iterator begin() const { return Range::iterator(0, this); }
    iterator end() const { return Range::iterator(size(), this); }

    inline bool empty() const { return begin() == end(); }

  protected:

    virtual void outputXML(magnet::xml::XmlStream& ) const = 0;    
  };
}
