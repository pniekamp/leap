//
// octree
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include <leap/util.h>
#include <leap/lml/bound.h>
#include <functional>
#include <vector>
#include <stack>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <iterator>
#include <utility>

/**
 * \namespace leap::lml
 * \brief Leap Math Library containing mathmatical routines
 *
**/

/**
 * \defgroup leapdata OcTree Container
 * \brief OcTree Containor
 *
**/

namespace leap { namespace lml
{
  namespace OcTree
  {
    // Traits

    //|///////////////////// bound //////////////////////////////////////////
    template<typename Item>
    decltype(auto) bound(Item const &item)
    {
      return item.box();
    }

    //|///////////////////// box ////////////////////////////////////////////
    template<typename Item>
    struct box
    {
      decltype(auto) operator()(Item const &item) const
      {
        return bound(dereference(item));
      }
    };


    //|-------------------- OcTree --------------------------------------------
    //|------------------------------------------------------------------------
    /**
     * \ingroup leapdata
     *
     * \brief OcTree container
     *
    **/

    template<typename Item, size_t dimension, class box = box<Item>, typename Alloc = std::allocator<Item>>
    class basic_octree
    {
        template<typename Iterator>
        class normal_iterator
        {
          public:

            using item_type = Item;
            using bound_type = std::decay_t<decltype(box()(std::declval<Item &>()))>;

            using value_type = item_type;
            using pointer = item_type *;
            using reference = item_type &;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

          public:
            normal_iterator();
            explicit normal_iterator(Iterator const &start);

            template<typename Iter>
            normal_iterator(normal_iterator<Iter> const &that);

            bool operator ==(normal_iterator const &that) const { return m_node == that.m_node; }
            bool operator !=(normal_iterator const &that) const { return m_node != that.m_node; }

            bound_type const &bound() const { return m_node->bound; }

            auto &items() const { return m_node->items; }

            size_t children() const { return m_node->nodes.size(); }
            normal_iterator child(size_t i) const { return normal_iterator(&m_node->nodes[i]); }

            normal_iterator &operator++();

            void descend();

          private:

            Iterator m_node;

            bool m_descend;

            template<typename T> friend class normal_iterator;
        };

        class Node;

      public:

        using item_type = Item;
        using value_type = Item;
        using allocator_type = Alloc;
        using item_allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Item>;
        using node_allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
        using bound_type = std::decay_t<decltype(box()(std::declval<Item &>()))>;

        using iterator = normal_iterator<Node *>;
        using const_iterator = normal_iterator<Node const *>;

      public:
        basic_octree(Alloc const &allocator = Alloc()) noexcept;
        basic_octree(bound_type const &world, Alloc const &allocator = Alloc()) noexcept;

        template<typename InputIterator>
        explicit basic_octree(InputIterator first, InputIterator last, Alloc const &allocator = Alloc());

        void clear(bound_type const &world);

        template<typename Q>
        void insert(Q &&item);

        void remove(Item const &item);

      public:

        bound_type const &world() const { return m_root.bound; }

        iterator begin() { return iterator(&m_root); }
        const_iterator begin() const  { return const_iterator(&m_root); }

        iterator end() { return iterator(); }
        const_iterator end() const  { return const_iterator(); }

      private:

        class Node
        {
          public:
            Node(Node *parent, allocator_type const &allocator)
              : m_parent(parent), bound(bound_limits<bound_type>::min()), items(allocator), nodes(allocator)
            {
            }

            Node(Node const &that)
              : m_parent(that.m_parent), bound(that.bound), items(that.items), nodes(that.nodes)
            {
              for(auto &node : nodes)
                node.m_parent = this;
            }

            template<typename Q>
            void insert(Q &&item, bound_type const &itembox);

            void remove(Item const &item, bound_type const &searchbox);

          public:

            Node *m_parent;

            bound_type bound;

            std::vector<Item, item_allocator_type> items;
            std::vector<Node, node_allocator_type> nodes;
        };

      private:

        Node m_root;
    };


    //|///////////////////// OcTree::normal_iterator ////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Iterator>
    basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator>::normal_iterator()
    {
      m_node = nullptr;
      m_descend = false;
    }


    //|///////////////////// OcTree::normal_iterator ////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Iterator>
    basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator>::normal_iterator(Iterator const &start)
    {
      m_node = start;
      m_descend = false;
    }


    //|///////////////////// OcTree::normal_iterator ////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Iterator>
    template<typename Iter>
    basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator>::normal_iterator(normal_iterator<Iter> const &that)
    {
      m_node = that.m_node;
      m_descend = that.m_descend;
    }


    //|///////////////////// OcTree::normal_iterator ////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Iterator>
#if _MSC_VER
    typename basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator> &basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator>::operator++()
#else
    typename basic_octree<Item, dimension, box, Alloc>::template normal_iterator<Iterator> &basic_octree<Item, dimension, box, Alloc>::normal_iterator<Iterator>::operator++()
#endif
  {
      if (m_descend)
      {
        m_node = &m_node->nodes[0];

        m_descend = false;

        return *this;
      }

      while (m_node != nullptr)
      {
        if (m_node->m_parent != nullptr && m_node != &m_node->m_parent->nodes.back())
        {
          ++m_node;

          m_descend = false;

          return *this;
        }

        m_node = m_node->m_parent;
      }

      return *this;
    }


    //|///////////////////// OcTree::normal_iterator ////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename NodeType>
    void basic_octree<Item, dimension, box, Alloc>::normal_iterator<NodeType>::descend()
    {
      if (m_node->nodes.size() != 0)
        m_descend = true;
    }


    //|///////////////////// OcTree::Constructor ////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    basic_octree<Item, dimension, box, Alloc>::basic_octree(Alloc const &allocator) noexcept
      : m_root(nullptr, allocator)
    {
    }


    //|///////////////////// OcTree::Constructor ////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    basic_octree<Item, dimension, box, Alloc>::basic_octree(bound_type const &world, Alloc const &allocator) noexcept
      : m_root(nullptr, allocator)
    {
      m_root.bound = world;
    }


    //|///////////////////// OcTree::Constructor ////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename InputIterator>
    basic_octree<Item, dimension, box, Alloc>::basic_octree(InputIterator first, InputIterator last, Alloc const &allocator)
      : m_root(nullptr, allocator)
    {
      auto world = bound_limits<bound_type>::min();

      for(InputIterator i = first; i != last; ++i)
        world = expand(world, box()(*i));

      clear(world);

      for(InputIterator i = first; i != last; ++i)
        insert(*i);
    }


    //|///////////////////// OcTree::clear //////////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    void basic_octree<Item, dimension, box, Alloc>::clear(bound_type const &world)
    {
      m_root.items.clear();
      m_root.nodes.clear();;
      m_root.bound = world;
    }


    //|///////////////////// OcTree::insert /////////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Q>
    void basic_octree<Item, dimension, box, Alloc>::insert(Q &&item)
    {
      m_root.insert(std::forward<Q>(item), box()(std::forward<Q>(item)));
    }


    //|///////////////////// OcTree::remove /////////////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    void basic_octree<Item, dimension, box, Alloc>::remove(Item const &item)
    {
      m_root.remove(item, box()(item));
    }

    // bound_quadrant
    template<typename Bound>
    struct bound_quadrant_impl
    {
      template<size_t i>
      static constexpr auto size(size_t quadrant, Bound const &bound)
      {
        return ((((quadrant >> i) & 1) << 1) * (high<i>(bound) - low<i>(bound)) - (high<i>(bound) - low<i>(bound)))/2;
      }

      template<size_t i>
      static constexpr auto centre(Bound const &bound)
      {
        return (low<i>(bound) + high<i>(bound))/2;
      }

      template<size_t... Indices>
      static constexpr auto bound(size_t quadrant, Bound const &bound, index_sequence<Indices...>)
      {
        return Bound({ std::min(centre<Indices>(bound), centre<Indices>(bound) + size<Indices>(quadrant, bound))... },
                     { std::max(centre<Indices>(bound), centre<Indices>(bound) + size<Indices>(quadrant, bound))... });
      }

      static constexpr Bound bound(size_t quadrant, Bound const &bound)
      {
        return bound_quadrant_impl::bound(quadrant, bound, make_index_sequence<0, Bound::size()>());
      }
    };

    //|///////////////////// OcTree::Node::insert ///////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    template<typename Q>
    void basic_octree<Item, dimension, box, Alloc>::Node::insert(Q &&item, bound_type const &itembox)
    {
      if (nodes.empty())
      {
        items.push_back(std::forward<Q>(item));

        if (items.size() >= 16)
        {
          constexpr int n = (1 << dimension);

          nodes.resize(n, Node(this, nodes.get_allocator()));

          for(int i = 0; i < n; ++i)
          {
            nodes[i].bound = bound_quadrant_impl<bound_type>::bound(i, bound);
          }

          auto olditems = std::move(items);

          for(auto &olditem : olditems)
            insert(std::move(olditem), box()(olditem));
        }
      }
      else
      {
        for(auto &node : nodes)
        {
          if (contains(node.bound, itembox))
          {
            node.insert(std::forward<Q>(item), itembox);
            return;
          }
        }

        items.push_back(std::forward<Q>(item));
      }
    }


    //|///////////////////// OcTree::Node::remove ///////////////////////////
    template<typename Item, size_t dimension, class box, typename Alloc>
    void basic_octree<Item, dimension, box, Alloc>::Node::remove(Item const &item, bound_type const &searchbox)
    {
      if (!intersects(bound, searchbox))
        return;

      if (items.size() != 0)
      {
        auto j = std::find(items.begin(), items.end(), item);

        if (j != items.end())
        {
          items.erase(j);
        }
      }

      if (nodes.size() != 0)
      {
        for(auto &node : nodes)
          node.remove(item, searchbox);

        size_t count = 0;
        for(auto &node : nodes)
          count += node.items.size() + node.nodes.size();

        if (count == 0)
          nodes.clear();
      }
    }


    /**
     * \name OcTree Operations
     * \ingroup leapdata
     * General operations on OcTrees
     * @{
    **/

    //|///////////////////// make_index /////////////////////////////////////
    template<size_t dimension, typename InputIterator>
    basic_octree<typename std::iterator_traits<InputIterator>::pointer, dimension> make_index(InputIterator first, InputIterator last)
    {
      basic_octree<typename std::iterator_traits<InputIterator>::pointer, dimension> result;

      for(auto i = first; i != last; ++i)
        result.insert(&*i);

      return result;
    }

    /**
     *  @}
    **/

  } // namespace OcTree

  /**
   * \name Misc OcTree
   * \ingroup lmloctree
   * Oc-Tree helpers
   * @{
  **/

  template<typename T>
  using OcTree2d = OcTree::basic_octree<T, 2>;

  template<typename T>
  using OcTree3d = OcTree::basic_octree<T, 3>;

} // namespace lml
} // namespace leap
