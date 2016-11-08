//
// polygon simplify
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef POLYGONSIMPLIFYP_HH
#define POLYGONSIMPLIFYP_HH


//|--------------------- polygon simplify impl ------------------------------
//|--------------------------------------------------------------------------

namespace leap { namespace lml { namespace PolygonSimplify
{

  //////////////////////// less_xy //////////////////////////////////////////
  template<typename T>
  bool less_xy(T const &lhs, T const &rhs)
  {
    return (get<0>(lhs) == get<0>(rhs)) ? (get<1>(lhs) < get<1>(rhs)) : (get<0>(lhs) < get<0>(rhs));
  }

  //////////////////////// equal_xy /////////////////////////////////////////
  template<typename T>
  bool equal_xy(T const &lhs, T const &rhs)
  {
    return (get<0>(lhs) == get<0>(rhs)) && (get<1>(lhs) == get<1>(rhs));
  }


  //|--------------------- Node ---------------------------------------------
  //|------------------------------------------------------------------------

  template<typename T>
  class Node
  {
    public:

      enum NodeFlags
      {
        P = 0x01,           // P Point
        Start = 0x04,       // Start Node
        Intersect = 0x08,   // Intersection
        Visited = 0x1000,
      };

    public:

      T site;

      long flags;

      Node<T> *next;
      Node<T> *prev;
  };


  //|--------------------- Intersect ----------------------------------------
  //|------------------------------------------------------------------------

  template<typename T>
  class Intersect : public Node<T>
  {
    public:

      double alpha;

      Node<T> *node;

      size_t nc;
      Intersect<T> *neighbors[4];
  };



  //|--------------------- Graph --------------------------------------------
  //|------------------------------------------------------------------------

  template<typename T>
  class Graph
  {
    public:

      struct Event
      {
        Node<T> *node;

        std::tuple<int, void*, double, void*> ordering;
      };

      typedef Node<T> node_type;
      typedef std::vector<Node<T>> nodes_type;
      typedef std::vector<Intersect<T>> intersects_type;
      typedef std::vector<Event> events_type;

      class normal_iterator : public std::iterator<std::forward_iterator_tag, node_type>
      {
        public:
          normal_iterator() : m_node(nullptr) { }
          explicit normal_iterator(Node<T> *start) : m_head(start), m_node(start) { }

          bool operator ==(normal_iterator const &that) const { return m_node == that.m_node; }
          bool operator !=(normal_iterator const &that) const { return m_node != that.m_node; }

          Node<T> &operator *() const { return *m_node; }
          Node<T> *operator ->() const { return &*m_node; }

          normal_iterator &operator++()
          {
            m_node = (m_node->next != m_head) ? m_node->next : nullptr;

            return *this;
          }

        private:

          Node<T> *m_head;
          Node<T> *m_node;
      };

    public:
      Graph(size_t n);
      Graph(Graph const &) = delete;
      Graph(Graph &&) = delete;

      template<typename InputIterator>
      void push_p(InputIterator f, InputIterator l);

      void join();

      nodes_type &p() { return m_p; }

      intersects_type &intersectsp() { return m_pi; }

      events_type &events() { return m_events; }

    protected:

      Node<T> make_node(int flags, T const &site);

      Node<T> *splice_node(Node<T> *after, Node<T> *node);

      Node<T> *bypass_node(Node<T> *node);

      void add_intersect(T const &site, Node<T> *ip, Node<T> *iq, double alphap, double alphaq);

      void add_intersects(Node<T> *ip, Node<T> *jp, Node<T> *iq, Node<T> *jq);

    private:

      std::vector<Node<T>> m_p;

      std::vector<Intersect<T>> m_pi;

      std::vector<Event> m_events;
  };


  //|///////////////////// Graph::Constructor////////////////////////////////
  template<typename T>
  Graph<T>::Graph(size_t n)
  {
    m_p.reserve(n);
  }


  //|///////////////////// Graph::push_p ////////////////////////////////////
  template<typename T>
  template<typename InputIterator>
  void Graph<T>::push_p(InputIterator f, InputIterator l)
  {
    size_t m = m_p.size();

    for(auto i = f; i != l; ++i)
    {
      m_p.push_back(make_node(Node<T>::P, *i));
    }

    size_t n = m_p.size() - m;

    for(size_t k = m; k < m+n; ++k)
    {
      m_p[k].next = &m_p[m + (k - m + 1) % n];
      m_p[k].prev = &m_p[m + (k - m - 1 + n) % n];
    }

    if (n != 0)
    {
      m_p[m].flags |= Node<T>::Start;
      m_events.push_back({ &m_p[m], std::make_tuple(Node<T>::P, &m_p[m], 0, nullptr) });
    }
  }


  //|///////////////////// Graph::make_node /////////////////////////////////
  template<typename T>
  Node<T> Graph<T>::make_node(int flags, T const &site)
  {
    Node<T> node;

    node.flags = flags;
    node.site = site;

    return node;
  }


  //|///////////////////// Graph::splice_node ///////////////////////////////
  template<typename T>
  Node<T> *Graph<T>::splice_node(Node<T> *after, Node<T> *node)
  {
    node->next = after->next;
    node->prev = after;

    after->next->prev = node;
    after->next = node;

    return node;
  }


  //|///////////////////// Graph::bypass_node ///////////////////////////////
  template<typename T>
  Node<T> *Graph<T>::bypass_node(Node<T> *node)
  {
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->next = node;
    node->prev = node;

    return node->prev;
  }


  //|///////////////////// Graph::add_intersect /////////////////////////////
  template<typename T>
  void Graph<T>::add_intersect(T const &site, Node<T> *ip, Node<T> *iq, double alphap, double alphaq)
  {
    Intersect<T> intersect;

    intersect.site = site;

    intersect.flags = Node<T>::P | Node<T>::Intersect;
    intersect.node = ip;
    intersect.alpha = alphap;

    m_pi.push_back(intersect);

    intersect.flags = Node<T>::P | Node<T>::Intersect;
    intersect.node = iq;
    intersect.alpha = alphaq;

    m_pi.push_back(intersect);
  }


  //|///////////////////// Graph::add_intersects ////////////////////////////
  /// intersection of two half closed line segments
  template<typename T>
  void Graph<T>::add_intersects(Node<T> *ip, Node<T> *jp, Node<T> *iq, Node<T> *jq)
  {
    auto &a1 = ip->site;
    auto &a2 = jp->site;
    auto &b1 = iq->site;
    auto &b2 = jq->site;

    if (equal_xy(a1, a2) || equal_xy(b1, b2))
      return;

    if (equal_xy(a1, b2) && equal_xy(a2, b1))
      return;

    auto is = intersection_robust(a1, a2, b1, b2);

    if (is.segseg() && is.s < 1 && is.t < 1)
    {
      add_intersect(*is, ip, iq, is.s, is.t);
    }

    if (is.overlap())
    {
      add_intersect(is.region->first, ip, iq, dist(a1, is.region->first) / dist(a1, a2), dist(b1, is.region->first) / dist(b1, b2));
      add_intersect(is.region->second, ip, iq, dist(a1, is.region->second) / dist(a1, a2), dist(b1, is.region->second) / dist(b1, b2));
    }
  }


  //|///////////////////// Graph::join //////////////////////////////////////
  template<typename T>
  void Graph<T>::join()
  {
    //
    // Find Intersections
    //

    std::vector<Node<T>*> sweepevents;

    for(auto &pt : m_p)
      sweepevents.push_back(&pt);

    std::sort(sweepevents.begin(), sweepevents.end(), [](Node<T> *a, Node<T> *b) { return less_xy(a->site, b->site); });

    std::vector<Node<T>*> segments;

    constexpr const auto epsilon = 10*std::numeric_limits<coord_type_t<T>>::epsilon();
    for(auto i = sweepevents.begin(); i != sweepevents.end(); )
    {
      auto start = i;

      do
      {
        auto &evt = *i;

        if (less_xy(evt->site, evt->next->site))
        {
          auto ylo = std::min(get<1>(evt->site), get<1>(evt->next->site)) - epsilon;
          auto yhi = std::max(get<1>(evt->site), get<1>(evt->next->site)) + epsilon;

          for(auto &seg : segments)
          {
            if (std::min(get<1>(seg->site), get<1>(seg->next->site)) > yhi || std::max(get<1>(seg->site), get<1>(seg->next->site)) < ylo)
              continue;

            add_intersects(evt, evt->next, seg, seg->next);
          }

          segments.push_back(evt);
        }

        if (less_xy(evt->site, evt->prev->site))
        {
          auto ylo = std::min(get<1>(evt->site), get<1>(evt->prev->site)) - epsilon;
          auto yhi = std::max(get<1>(evt->site), get<1>(evt->prev->site)) + epsilon;

          for(auto &seg : segments)
          {
            if (std::min(get<1>(seg->site), get<1>(seg->next->site)) > yhi || std::max(get<1>(seg->site), get<1>(seg->next->site)) < ylo)
              continue;

            add_intersects(evt->prev, evt, seg, seg->next);
          }

          segments.push_back(evt->prev);
        }

        ++i;

      } while (i != sweepevents.end() && get<0>((*i)->site) < get<0>((*std::prev(i))->site) + epsilon);

      for(auto k = start; k != i; ++k)
      {
        auto &evt = *k;

        if (less_xy(evt->next->site, evt->site))
        {
          segments.erase(std::find(segments.begin(), segments.end(), evt));
        }

        if (less_xy(evt->prev->site, evt->site))
        {
          segments.erase(std::find(segments.begin(), segments.end(), evt->prev));
        }
      }
    }


    //
    // Intersection Events
    //

    for(size_t i = 0; i < m_pi.size(); i += 2)
    {
      m_pi[i].nc = 1;
      m_pi[i].neighbors[0] = &m_pi[i+1];

      m_pi[i+1].nc = 1;
      m_pi[i+1].neighbors[0] = &m_pi[i];

      m_events.push_back({ &m_pi[i], std::make_tuple(Node<T>::P, m_pi[i].node, m_pi[i].alpha, m_pi[i+1].node) });
      m_events.push_back({ &m_pi[i+1], std::make_tuple(Node<T>::P, m_pi[i+1].node, m_pi[i+1].alpha, m_pi[i].node) });
    }

    sort(m_events.begin(), m_events.end(), [](Event const &lhs, Event const &rhs) { return lhs.ordering < rhs.ordering; });


    //
    // Splice Intersections
    //

    for(auto i = m_events.rbegin(); i != m_events.rend(); ++i)
    {
      if (i->node->flags & Node<T>::Intersect)
      {
        splice_node(static_cast<Intersect<T>*>(i->node)->node, i->node);
      }
    }

    //
    // Collapse Coincidence Near Intersections
    //

    for(auto &evt : m_events)
    {
      if (evt.node->flags & Node<T>::Intersect)
      {
        auto ii = static_cast<Intersect<T>*>(evt.node);

        if (!(ii->prev->flags & Node<T>::Intersect) && coincident(ii->prev->site, ii->site))
        {
          bypass_node(ii->prev);
        }

        if (!(ii->next->flags & Node<T>::Intersect) && coincident(ii->site, ii->next->site))
        {
          bypass_node(ii->next);
        }
      }
    }

    for(auto &evt : m_events)
    {
      if ((evt.node->flags & Node<T>::Intersect) && (evt.node->next->flags & Node<T>::Intersect) && coincident(evt.node->site, evt.node->next->site))
      {
        auto ii = static_cast<Intersect<T>*>(evt.node);
        auto ji = static_cast<Intersect<T>*>(evt.node->next);

        if (ii->neighbors[0] == ji->neighbors[0]->next || ii->neighbors[0]->next == ji->neighbors[0])
        {
          ii->flags &= ~Node<T>::Intersect;
          ii->neighbors[0]->flags &= ~Node<T>::Intersect;

          bypass_node(ii);
          bypass_node(ii->neighbors[0]);
        }
      }
    }

    for(auto &evt : m_events)
    {
      if ((evt.node->flags & Node<T>::Intersect) && (evt.node->next->flags & Node<T>::Intersect) && coincident(evt.node->site, evt.node->next->site))
      {
        auto ii = static_cast<Intersect<T>*>(evt.node);
        auto ji = static_cast<Intersect<T>*>(evt.node->next);

        for(size_t k = 0; k < ii->nc && ji->nc < extentof(ji->neighbors); ++k)
        {
          ii->neighbors[k]->site = ji->site;

          ii->neighbors[k]->nc = std::remove(ii->neighbors[k]->neighbors, ii->neighbors[k]->neighbors + ii->neighbors[k]->nc, ii) - ii->neighbors[k]->neighbors;
          ii->neighbors[k]->nc = std::remove(ii->neighbors[k]->neighbors, ii->neighbors[k]->neighbors + ii->neighbors[k]->nc, ji) - ii->neighbors[k]->neighbors;
          ii->neighbors[k]->neighbors[ii->neighbors[k]->nc++] = ji;

          ji->nc = std::remove(ji->neighbors, ji->neighbors + ji->nc, ii->neighbors[k]) - ji->neighbors;
          ji->neighbors[ji->nc++] = ii->neighbors[k];
        }

        ii->flags &= ~Node<T>::Intersect;

        bypass_node(ii);
      }
    }

    m_events.erase(std::remove_if(m_events.begin(), m_events.end(), [](Event const &i) { return !(i.node->flags & (Node<T>::Start | Node<T>::Intersect)); }), m_events.end());
  }


  //|--------------------- Ring ---------------------------------------------
  //|------------------------------------------------------------------------

  template<typename T>
  struct Ring
  {
    coord_type_t<T> area;

    typename Graph<T>::normal_iterator begin;
    typename Graph<T>::normal_iterator end;

    int orientation;
    int winding;
  };


  //|///////////////////// make_ring ////////////////////////////////////////
  template<typename Container, typename T>
  auto make_ring(Ring<T> const &ring)
  {
    Container result;

    for(auto k = ring.begin; k != ring.end; ++k)
      result.push_back(k->site);

    return result;
  }


  //|--------------------- Simplify Operation -------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// edge_orientation /////////////////////////////////
  template<typename T>
  int edge_orientation(Node<T> const *i, Node<T> const *j)
  {
    auto base = orientation(i->prev->site, i->site, i->next->site);
    auto lower = orientation(i->prev->site, i->site, j->site);
    auto upper = orientation(i->site, i->next->site, j->site);

    if (base >= 0 && (lower > 0 && upper > 0))
      return 1;

    if (base > 0 && (lower < 0 || upper < 0))
      return -1;

    if (base <= 0 && (lower < 0 && upper < 0))
      return -1;

    if (base < 0 && (lower > 0 || upper > 0))
      return 1;

    return 0;
  }


  //|///////////////////// edge_crosses /////////////////////////////////////
  template<typename T>
  int edge_crosses(Node<T> const *i, Node<T> const *j)
  {
    auto forward = edge_orientation(i, j->next);

    if (forward == 0)
      return 0;

    auto reverse = edge_orientation(i, j->prev);

    if (reverse == 0)
    {
      if (coincident(i->prev->site, j->prev->site))
      {
        for(auto ii = i->prev, jj = j->prev; ii != i && jj != j && reverse == 0; ii = ii->prev, jj = jj->prev)
          reverse = edge_orientation(ii, jj->prev);
      }

      if (coincident(i->next->site, j->prev->site))
      {
        for(auto ii = i->next, jj = j->prev; ii != i && jj != j && reverse == 0; ii = ii->next, jj = jj->prev)
          reverse = edge_orientation(ii, jj->prev);
      }
    }

    return (forward * reverse < 0) ? reverse : 0;
  }


  //|///////////////////// traverse ///////////////////////////////////////
  template<typename T>
  auto traverse(Node<T> *node)
  {
    auto start = node;

    auto area = coord_type_t<T>(0);

    while (!(node->flags & Node<T>::Visited))
    {
      node->flags |= Node<T>::Visited;

      do
      {
        area += get<0>(node->prev->site)*get<1>(node->site) - get<0>(node->site)*get<1>(node->prev->site);

        node = node->next;

      } while (!(node->flags & (Node<T>::Start | Node<T>::Intersect)));
    }

    std::swap(node->prev->next, start->prev->next);
    std::swap(node->prev, start->prev);

    return fcmp(area, decltype(area)(0)) ? 0 : area/2;
  }


  //|///////////////////// simplify /////////////////////////////////////////
  /// polygon simplify operation
  template<typename MultiRing, typename Point>
  void polygon_simplify(MultiRing *result, Graph<Point> &graph)
  {
    //
    // Split Graph
    //

    for(auto &evt : graph.events())
    {
      if (evt.node->flags & Node<Point>::Intersect)
      {
        auto ii = static_cast<Intersect<Point>*>(evt.node);

        for(size_t k = 0; k < ii->nc; ++k)
        {
          auto ki = ii->neighbors[k];

          if (edge_crosses(ii, ki))
          {
            std::swap(ki->prev->next, ii->prev->next);
            std::swap(ki->prev, ii->prev);
          }
        }
      }
    }

    for(auto &evt : graph.events())
    {
      if (evt.node->flags & Node<Point>::Intersect)
      {
        for(auto i = evt.node->next; i != evt.node; i = i->next)
        {
          if (coincident(evt.node->site, i->site))
          {
            std::swap(evt.node->prev->next, i->prev->next);
            std::swap(evt.node->prev, i->prev);
            break;
          }
        }
      }
    }

    //
    // Extract rings with winding
    //

    Node<Point> *base = graph.events()[0].node;
    Node<Point> *left = base;

    for(auto &evt : graph.events())
    {
      for(auto i = typename Graph<Point>::normal_iterator(evt.node); i != typename Graph<Point>::normal_iterator(); ++i)
      {
        if (less_xy(i->site, left->site))
        {
          left = &*i;
          base = evt.node;
        }
      }
    }

    std::vector<std::tuple<Node<Point>*, int>> work(1, std::make_tuple(base, 0));

    while (work.size() != 0)
    {
      auto entry = work.back();

      work.resize(work.size() - 1);

      auto node = get<0>(entry);

      if (node->flags & Node<Point>::Visited)
        continue;

      auto area = traverse(node);

      if (area != 0)
      {
        result->resize(result->size() + 1);
        result->back().area = std::abs(area);
        result->back().begin = typename Graph<Point>::normal_iterator(node);
        result->back().end = typename Graph<Point>::normal_iterator();
        result->back().orientation = sign(area);
        result->back().winding = get<1>(entry) + sign(area);
      }

      for(auto i = typename Graph<Point>::normal_iterator(node); i != typename Graph<Point>::normal_iterator(); ++i)
      {
        if (i->flags & Node<Point>::Intersect)
        {
          auto ii = static_cast<Intersect<Point>*>(&*i);

          for(size_t k = 0; k < ii->nc; ++k)
          {
            auto ki = ii->neighbors[k];

            if (ki->flags & Node<Point>::Visited)
              continue;

            auto forward = edge_orientation(ii, ki->next);

            if (forward != 0)
              work.push_back(std::make_tuple(ki, get<1>(entry) + (forward * area > 0) * sign(area)));
          }
        }
      }
    }
  }


} } } // namespace PolygonSimplify


#endif
