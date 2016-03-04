//
// polygon set op
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef POLYGONSETOPP_HH
#define POLYGONSETOPP_HH


//|--------------------- polygon set impl -----------------------------------
//|--------------------------------------------------------------------------

namespace leap { namespace lml { namespace PolygonSetOp
{
  enum class Op
  {
    Union,
    Intersection,
    Difference
  };


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
        Q = 0x02,           // Q Point
        Start = 0x04,       // Start Node
        Intersect = 0x08,   // Intersection
        En = 0x100,         // Forward Enter Other Polygon
        Ex = 0x200,         // Forward Exit Other Polygon
        Branch = (En | Ex), // Branch Mask
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

          Node<T> &operator *() { return *m_node; }
          Node<T> *operator ->() { return &*m_node; }

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
      Graph(size_t n, size_t m);
      Graph(Graph const &) = delete;
      Graph(Graph &&) = delete;

      template<typename InputIterator>
      void push_p(InputIterator f, InputIterator l);

      template<typename InputIterator>
      void push_q(InputIterator f, InputIterator l);

      void join();

      nodes_type &p() { return m_p; }
      nodes_type &q() { return m_q; }

      intersects_type &intersectsp() { return m_pi; }
      intersects_type &intersectsq() { return m_qi; }

      events_type &events() { return m_events; }

    protected:

      Node<T> make_node(int flags, T const &site);

      Node<T> *splice_node(Node<T> *after, Node<T> *node);

      Node<T> *bypass_node(Node<T> *node);

      void add_intersect(T const &site, Node<T> *ip, Node<T> *iq, double alphap, double alphaq);

      void add_intersects(Node<T> *ip, Node<T> *jp, Node<T> *iq, Node<T> *jq);

    private:

      std::vector<Node<T>> m_p;
      std::vector<Node<T>> m_q;

      std::vector<Intersect<T>> m_pi;
      std::vector<Intersect<T>> m_qi;

      std::vector<Event> m_events;
  };


  //|///////////////////// Graph::Constructor////////////////////////////////
  template<typename T>
  Graph<T>::Graph(size_t n, size_t m)
  {
    m_p.reserve(n);
    m_q.reserve(m);
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


  //|///////////////////// Graph::push_q ////////////////////////////////////
  template<typename T>
  template<typename InputIterator>
  void Graph<T>::push_q(InputIterator f, InputIterator l)
  {
    size_t m = m_q.size();

    for(auto i = f; i != l; ++i)
    {
      m_q.push_back(make_node(Node<T>::Q, *i));
    }

    size_t n = m_q.size() - m;

    for(size_t k = m; k < m+n; ++k)
    {
      m_q[k].next = &m_q[m + (k - m + 1) % n];
      m_q[k].prev = &m_q[m + (k - m - 1 + n) % n];
    }

    if (n != 0)
    {
      m_q[m].flags |= Node<T>::Start;
      m_events.push_back({ &m_q[m], std::make_tuple(Node<T>::Q, &m_q[m], 0, nullptr) });
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

    intersect.flags = Node<T>::Q | Node<T>::Intersect;
    intersect.node = iq;
    intersect.alpha = alphaq;

    m_qi.push_back(intersect);
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

    if (equal_xy(a1, b1) && equal_xy(a2, b2))
    {
      add_intersect(a1, ip, iq, 0, 0);
      return;
    }

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

    for(auto &pt : m_q)
      sweepevents.push_back(&pt);

    std::sort(sweepevents.begin(), sweepevents.end(), [](Node<T> *lhs, Node<T> *rhs) { return less_xy(lhs->site, rhs->site); });

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

            if (evt->flags & Node<T>::P && seg->flags & Node<T>::Q)
              add_intersects(evt, evt->next, seg, seg->next);

            else if (evt->flags & Node<T>::Q && seg->flags & Node<T>::P)
              add_intersects(seg, seg->next, evt, evt->next);
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

            if (evt->flags & Node<T>::P && seg->flags & Node<T>::Q)
              add_intersects(evt->prev, evt, seg, seg->next);

            else if (evt->flags & Node<T>::Q && seg->flags & Node<T>::P)
              add_intersects(seg, seg->next, evt->prev, evt);
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

    for(size_t i = 0; i < m_pi.size(); ++i)
    {
      m_pi[i].nc = 1;
      m_pi[i].neighbors[0] = &m_qi[i];

      m_qi[i].nc = 1;
      m_qi[i].neighbors[0] = &m_pi[i];

      m_events.push_back({ &m_pi[i], std::make_tuple(Node<T>::P, m_pi[i].node, m_pi[i].alpha, m_qi[i].node) });
      m_events.push_back({ &m_qi[i], std::make_tuple(Node<T>::Q, m_qi[i].node, m_qi[i].alpha, m_pi[i].node) });
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




  //|--------------------- Set Operations -----------------------------------
  //|------------------------------------------------------------------------


  //|///////////////////// node_shared //////////////////////////////////////
  template<typename T>
  bool node_shared(Node<T> const *i, Node<T> const *j)
  {
    if ((i->flags & Node<T>::Intersect) && (j->flags & Node<T>::Intersect))
    {
      auto ii = static_cast<Intersect<T> const *>(i);
      auto ji = static_cast<Intersect<T> const *>(j);

      for(size_t k = 0; k < ii->nc; ++k)
      {
        if (ii->neighbors[k] == ji)
          return true;
      }
    }

    return false;
  }


  //|///////////////////// edge_inother /////////////////////////////////////
  template<typename Polygon, typename T>
  bool edge_inother(Polygon const &p, Polygon const &q, Node<T> const *i, Node<T> const *j)
  {
    return contains((i->flags & Node<T>::P) ? q : p, (i->site + j->site)/2);
  }


  //|///////////////////// edge_direction ///////////////////////////////////
  template<typename T>
  auto edge_direction(Node<T> const *i, Node<T> const *j)
  {
    return angle(i->site, j->site);
  }


  //|///////////////////// traversal_flag ///////////////////////////////////
  template<typename Polygon, typename T>
  long traversal_flag(Polygon const &p, Polygon const &q, Node<T> *curr, Op op)
  {
    enum EdgeFlag { OnFwd=1, OnBck=2, In=3, Out=4 };

    if (curr->next == curr)
      return 0;

    int nf = 0;

    if ((curr->flags & Node<T>::Intersect) && (curr->next->flags & Node<T>::Intersect))
    {
      auto ii = static_cast<Intersect<T> const *>(curr);

      for(size_t k = 0; k < ii->nc; ++k)
      {
        if (node_shared(ii->next, ii->neighbors[k]->prev))
          nf = OnBck;

        if (node_shared(ii->next, ii->neighbors[k]->next))
          nf = OnFwd;
      }
    }

    if (nf == 0)
    {
      nf = edge_inother(p, q, curr, curr->next) ? In : Out;
    }

    switch (op)
    {
      case Op::Union:
        if (nf == In) return Node<T>::Ex;
        if (nf == Out) return Node<T>::En;
        if (nf == OnFwd) return Node<T>::En;
        break;

      case Op::Intersection:
        if (nf == Out) return Node<T>::Ex;
        if (nf == In) return Node<T>::En;
        if (nf == OnFwd) return Node<T>::En;
        break;

      case Op::Difference:
        if ((curr->flags & Node<T>::P) && nf == Out) return Node<T>::En;
        if ((curr->flags & Node<T>::P) && nf == In) return Node<T>::Ex;
        if ((curr->flags & Node<T>::Q) && nf == Out) return Node<T>::Ex;
        if ((curr->flags & Node<T>::Q) && nf == In) return Node<T>::En;
        if (nf == OnFwd) return Node<T>::En;
        break;
    }

    return 0;
  }


  //|///////////////////// traverse ///////////////////////////////////////
  template<typename Ring, typename T>
  auto traverse(Ring *result, Node<T> *node, int direction)
  {
    auto area = coord_type_t<T>(0);

    while (!(node->flags & Node<T>::Visited))
    {
      node->flags |= Node<T>::Visited;

      if (node->flags & Node<T>::Intersect)
      {
        auto ii = static_cast<Intersect<T>*>(node);

        for(size_t k = 0; k < ii->nc; ++k)
        {
          if (node_shared(ii->next, ii->neighbors[k]->next))
            ii->neighbors[k]->flags |= Node<T>::Visited;
        }
      }

      do
      {
        result->push_back(node->site);

        area += get<0>(node->prev->site)*get<1>(node->site) - get<0>(node->site)*get<1>(node->prev->site);

        node = node->next;

      } while (!(node->flags & (Node<T>::Start | Node<T>::Intersect)));

      if (node->flags & Node<T>::Intersect)
      {
        auto entry = edge_direction(node, node->prev);

        for(size_t n = 0; n < 2; ++n)
        {
          auto ki = static_cast<Intersect<T>*>(node);

          auto bestexit = fmod2((edge_direction(ki, ki->next) - entry) * direction, 2*pi<coord_type_t<T>>());

          for(auto k = ki->neighbors; k != ki->neighbors + ki->nc; ++k)
          {
            auto exit = fmod2((edge_direction(*k, (*k)->next) - entry) * direction, 2*pi<coord_type_t<T>>());

            if (exit < bestexit)
              std::tie(bestexit, node) = std::tie(exit, *k);
          }

          if (node == ki || static_cast<Intersect<T>*>(node)->nc <= 1)
            break;
        }
      }
    }

    return fcmp(area, decltype(area)(0)) ? 0 : area/2;
  }


  //|///////////////////// setop ////////////////////////////////////////////
  /// polygon set operation
  template<typename MultiRing, typename Polygon, typename Point>
  void polygon_setop(MultiRing *result, Graph<Point> &graph, Polygon const &p, Polygon const &q, Op op)
  {
#if 0
qDebug() << "Graph";
for(auto &evt : graph.events())
{
  if (!(evt.node->flags & 0x100000))
  {
    Node<Point> *node = evt.node;

    do
    {
      auto flags = node->flags | traversal_flag(p, q, node, op);

      qDebug() << ((flags & Node<Point>::P) ? "P" : "Q") << ((flags & Node<Point>::Start) ? "S" : " ") << ((flags & Node<Point>::Intersect) ? "I" : " ") << ((flags & Node<Point>::En) ? "E" : " ") << ((flags & Node<Point>::Ex) ? "X" : " ") << node->site;

      node->flags |= 0x100000;

      node = node->next;

    } while (node != evt.node);

    qDebug() << "-";
  }
}
#endif

    for(auto &evt : graph.events())
    {
      if (evt.node->flags & Node<Point>::Intersect)
      {
        Intersect<Point> *ii = static_cast<Intersect<Point>*>(evt.node);

        for(size_t j = 0; j < ii->nc; ++j)
        {
          for(size_t k = j+1; k < ii->nc; ++k)
          {
            if (edge_direction(ii->neighbors[j], ii->neighbors[j]->prev) == edge_direction(ii->neighbors[k], ii->neighbors[k]->next) || edge_direction(ii->neighbors[j], ii->neighbors[j]->next) == edge_direction(ii->neighbors[k], ii->neighbors[k]->prev))
            {
              std::swap(ii->neighbors[j]->prev->next, ii->neighbors[k]->prev->next);
              std::swap(ii->neighbors[j]->prev, ii->neighbors[k]->prev);

              std::remove(ii->neighbors[j]->neighbors, ii->neighbors[j]->neighbors + ii->neighbors[j]->nc--, ii);
              std::remove(ii->neighbors[k]->neighbors, ii->neighbors[k]->neighbors + ii->neighbors[k]->nc--, ii);

              std::remove(ii->neighbors, ii->neighbors + ii->nc--, ii->neighbors[j]);
              std::remove(ii->neighbors, ii->neighbors + ii->nc--, ii->neighbors[k]);
            }
          }
        }
      }
    }

    int direction = 0;

    switch (op)
    {
      case Op::Union:
        direction = ring_traits<Polygon>::orientation;
        break;

      case Op::Intersection:
        direction = -ring_traits<Polygon>::orientation;
        break;

      case Op::Difference:
        direction = -ring_traits<Polygon>::orientation;
        break;
    }

    for(auto &evt : graph.events())
    {
      if (evt.node->flags & Node<Point>::Visited)
        continue;

      if (traversal_flag(p, q, evt.node, op) == Node<Point>::En)
      {
        typename MultiRing::value_type ring;

        if (traverse(&ring, evt.node, direction) != 0)
        {
          result->push_back(std::move(ring));
        }
      }
    }
  }

} } } // namespace PolygonSetOp


#endif
