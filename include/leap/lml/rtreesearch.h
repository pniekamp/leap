//
// r-tree search routines
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef RTREESEARCH_HH
#define RTREESEARCH_HH

#include <leap/lml/rtree.h>
#include <leap/lml/vector.h>
#include <leap/lml/geometry.h>
#include <cassert>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup leapdata Data Containors
 * \brief Data Containors
 *
**/

namespace leap { namespace lml
{
  namespace RTree
  {

    //|-------------------- bounded_iterator ---------------------------------
    //|-----------------------------------------------------------------------
    /**
     * \ingroup lmlrtree
     *
     * \brief RTree container bounded iterator
     *
    **/

    template<typename Iterator>
    class rtree_bounded_iterator : public std::iterator<std::forward_iterator_tag, typename Iterator::value_type::value_type>
    {
      public:

        typedef typename Iterator::bound_type bound_type;

      public:
        rtree_bounded_iterator()
          : m_item(0)
        {
        }

        rtree_bounded_iterator(Iterator first, bound_type const &searchbox)
          : m_item(0), m_iterator(first), m_searchbox(searchbox)
        {
          if (m_iterator != Iterator() && (m_iterator->size() == 0 || !contains(m_searchbox, dereference(m_iterator->at(m_item)))))
            ++(*this);
        }

        bool operator ==(rtree_bounded_iterator const &that) const { return (m_iterator == that.m_iterator) && (m_item == that.m_item); }
        bool operator !=(rtree_bounded_iterator const &that) const { return (m_iterator != that.m_iterator) || (m_item != that.m_item); }

        bool operator ==(Iterator const &that) const { return (m_iterator == that && m_item == 0); }
        bool operator !=(Iterator const &that) const { return (m_iterator != that || m_item != 0); }

        auto operator *() const -> decltype((std::declval<Iterator>()->at(0))) { return m_iterator->at(m_item); }
        auto operator ->() const -> decltype(&(std::declval<Iterator>()->at(0))) { return &m_iterator->at(m_item); }

        rtree_bounded_iterator &operator++()
        {
          ++m_item;

          while (m_iterator != Iterator())
          {
            if (m_item >= m_iterator->size())
            {
              m_item = 0;

              if (intersects(m_iterator.bound(), m_searchbox))
                m_iterator.descend();

              ++m_iterator;

              continue;
            }

            if (contains(m_searchbox, dereference(m_iterator->at(m_item))))
              break;

            ++m_item;
          }

          return *this;
        }

      private:

        size_t m_item;
        Iterator m_iterator;
        bound_type m_searchbox;
    };

    template<typename Iterator>
    rtree_bounded_iterator<Iterator> bounded_iterator(Iterator first, typename Iterator::bound_type const &searchbox)
    {
      return rtree_bounded_iterator<Iterator>(first, searchbox);
    }


    //|-------------------- nearest_neighbour ---------------------------------
    //|------------------------------------------------------------------------
    /**
     * \ingroup lmlrtree
     *
     * \brief RTree container nearest neighbour search
     *
    **/

    template<typename Item, size_t dimension, typename Point>
    Item *nearest_neighbour(basic_rtree<Item, dimension> &index, Point const &pt, typename basic_rtree<Item, dimension>::bound_type const &searchbox)
    {
      Item *nearest = nullptr;

      auto mindist = std::numeric_limits<decltype(distsqr(dereference(std::declval<Item&>()), pt))>::max();

      auto confine = searchbox;

      for(auto i = index.begin(); i != index.end(); ++i)
      {
        for(auto j = i->begin(); j != i->end(); ++j)
        {
          auto dist = distsqr(dereference(*j), pt);

          if (dist < mindist && contains(searchbox, dereference(*j)))
          {
            nearest = &*j;
            mindist = dist;
            confine = *intersection(make_bound(pt, std::sqrt(dist)), confine);
          }
        }

        if (intersects(i.bound(), confine))
          i.descend();
      }

      return nearest;
    }

    template<typename Item, size_t dimension, typename Point>
    Item *nearest_neighbour(basic_rtree<Item, dimension> &index, Point const &pt)
    {
      return nearest_neighbour(index, pt, bound_limits<typename basic_rtree<Item, dimension>::bound_type>::max());
    }

    template<typename Item, size_t dimension, typename Point>
    Item const *nearest_neighbour(basic_rtree<Item, dimension> const &index, Point const &pt, typename basic_rtree<Item, dimension>::bound_type const &searchbox)
    {
      return nearest_neighbour(const_cast<basic_rtree<Item, dimension>&>(index), pt, searchbox);
    }

    template<typename Item, size_t dimension, typename Point>
    Item const *nearest_neighbour(basic_rtree<Item, dimension> const &index, Point const &pt)
    {
      return nearest_neighbour(const_cast<basic_rtree<Item, dimension>&>(index), pt, bound_limits<typename basic_rtree<Item, dimension>::bound_type>::max());
    }

} // namespace rtree
} // namespace lml
} // namespace leap

#endif
