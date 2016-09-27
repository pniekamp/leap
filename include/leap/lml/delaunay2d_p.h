//
// delaunay 2d triangulation
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#ifndef DELAUNAY2DP_HH
#define DELAUNAY2DP_HH

//|--------------------- delaunay 2d triangulation --------------------------
//|--------------------------------------------------------------------------

namespace leap { namespace lml { namespace Delaunay2d
{
  // Traits

  //|///////////////////// pos ////////////////////////////////////////////
  template<typename Site>
  struct pos
  {
    decltype(auto) operator()(Site const &site) const
    {
      return site;
    }
  };


  //////////////////////// less_xy //////////////////////////////////////////
  template<typename Site, class pos>
  bool less_xy(Site const &lhs, Site const &rhs)
  {
    auto const &a = pos()(lhs);
    auto const &b = pos()(rhs);

    return (get<0>(a) == get<0>(b)) ? (get<1>(a) < get<1>(b)) : (get<0>(a) < get<0>(b));
  }


  //////////////////////// equal_xy /////////////////////////////////////////
  template<typename Site, class pos>
  bool equal_xy(Site const &lhs, Site const &rhs)
  {
    auto const &a = pos()(lhs);
    auto const &b = pos()(rhs);

    return (get<0>(a) == get<0>(b)) && (get<1>(a) == get<1>(b));
  }


  //|--------------------- Edge ---------------------------------------------
  //|------------------------------------------------------------------------
  template<typename T>
  class Edge
  {
    public:

      T *org() { return site; }
      T *dst() { return sym(this)->site; }

      Edge *o_next() { return next; }
      Edge *o_prev() { return rot(rot(this)->next); }

      Edge *d_next() { return sym(sym(this)->next); }
      Edge *d_prev() { return tor(tor(this)->next); }

      Edge *l_next() { return rot(tor(this)->next); }
      Edge *l_prev() { return sym(next); }

      Edge *r_next() { return tor(rot(this)->next); }
      Edge *r_prev() { return sym(this)->next; }

      Edge *base() { return this - index; }

      T *site;

      int index;
      Edge *next;
  };


  //|///////////////////// sym //////////////////////////////////////////////
  template<typename T>
  Edge<T> *sym(Edge<T> *edge) // opposite
  {
    return (edge->index < 2) ? edge + 2 : edge - 2;
  }


  //|///////////////////// rot //////////////////////////////////////////////
  template<typename T>
  Edge<T> *rot(Edge<T> *edge) // clockwise
  {
    return (edge->index < 3) ? edge + 1 : edge - 3;
  }


  //|///////////////////// invrot ///////////////////////////////////////////
  template<typename T>
  Edge<T> *tor(Edge<T> *edge) // counter clockwise
  {
    return (edge->index > 0) ? edge - 1 : edge + 3;
  }


  //|--------------------- Mesh -------------------------------------------
  //|----------------------------------------------------------------------
  template<typename T, class pos = pos<T>>
  class Mesh
  {
    public:

      typedef T site_type;
      typedef std::vector<site_type> sites_type;

      typedef Edge<T> *edge_type;
      typedef std::vector<edge_type> edges_type;

    public:
      Mesh();
      Mesh(Mesh const &) = delete;
      Mesh(Mesh &&) = delete;
      ~Mesh();

      void add_site(T const &site);

      template<typename InputIterator>
      void add_sites(InputIterator f, InputIterator l);

      sites_type &sites() { return m_sites; }

    public:

      void triangulate();

      edges_type &edges() { return m_edges; }

    protected:

      Edge<T> *make_edge(T *org = NULL, T *dst = NULL);
      Edge<T> *make_edge(Edge<T> *a, Edge<T> *b);

      void splice_edges(Edge<T> *a, Edge<T> *b);

      void destroy_edge(Edge<T> *edge);

      bool leftof(T *site, Edge<T> *edge);
      bool rightof(T *site, Edge<T> *edge);
      bool incircle(T *a, T *b, T *c, T *d);

      void delaunay(size_t low, size_t high, Edge<T> **left, Edge<T> **right);

    private:

      std::vector<T> m_sites;

      std::vector<Edge<T>*> m_edges;
  };


  //|///////////////////// Mesh::Constructor/////////////////////////////////
  template<typename T, class pos>
  Mesh<T, pos>::Mesh()
  {
  }


  //|///////////////////// Mesh::Destructor//////////////////////////////////
  template<typename T, class pos>
  Mesh<T, pos>::~Mesh()
  {
    for(auto i = m_edges.begin(); i != m_edges.end(); ++i)
      delete[] *i;
  }


  //|///////////////////// Mesh::add_site ///////////////////////////////////
  /// add a site to a Mesh Object
  template<typename T, class pos>
  void Mesh<T, pos>::add_site(T const &site)
  {
    m_sites.push_back(site);
  }


  //|///////////////////// Mesh::add_points /////////////////////////////////
  /// add sites to a Mesh Object
  template<typename T, class pos>
  template<typename InputIterator>
  void Mesh<T, pos>::add_sites(InputIterator f, InputIterator l)
  {
    std::copy(f, l, std::back_inserter(m_sites));
  }


  //|///////////////////// Mesh::make_edge //////////////////////////////////
  /// make a quad-edge object
  template<typename T, class pos>
  Edge<T> *Mesh<T, pos>::make_edge(T *org, T *dst)
  {
    Edge<T> *quad = new Edge<T>[4];

    quad[0].next = &quad[0];
    quad[1].next = &quad[3];
    quad[2].next = &quad[2];
    quad[3].next = &quad[1];

    quad[0].index = 0;
    quad[1].index = 1;
    quad[2].index = 2;
    quad[3].index = 3;

    quad->site = org;
    sym(quad)->site = dst;

    m_edges.push_back(quad);

    return quad;
  }


  //|///////////////////// Mesh::destroy_edge ///////////////////////////////
  /// destroy a quad-edge object
  template<typename T, class pos>
  void Mesh<T, pos>::destroy_edge(Edge<T> *edge)
  {
    splice_edges(edge, edge->o_prev());
    splice_edges(sym(edge), sym(edge)->o_prev());

    m_edges.erase(std::find(m_edges.begin(), m_edges.end(), edge->base()));

    delete[] edge->base();
  }


  //|///////////////////// Mesh::splice_edges ///////////////////////////////
  /// splice two quad-edge objects
  template<typename T, class pos>
  void Mesh<T, pos>::splice_edges(Edge<T> *a, Edge<T> *b)
  {
    Edge<T> *p = rot(a->next);
    Edge<T> *q = rot(b->next);

    std::swap(a->next, b->next);
    std::swap(p->next, q->next);
  }


  //|///////////////////// Mesh::make_edge //////////////////////////////////
  template<typename T, class pos>
  Edge<T> *Mesh<T, pos>::make_edge(Edge<T> *a, Edge<T> *b)
  {
    Edge<T> *quad = make_edge(a->dst(), b->org());

    splice_edges(quad, a->l_next());
    splice_edges(sym(quad), b);

    return quad;
  }


  //|///////////////////// leftof ///////////////////////////////////////////
  template<typename T, class pos>
  bool Mesh<T, pos>::leftof(T *site, Edge<T> *edge)
  {
    return orientation(pos()(*site), pos()(*(edge->org())), pos()(*(edge->dst()))) > 0.0;
  }


  //|///////////////////// rightof //////////////////////////////////////////
  template<typename T, class pos>
  bool Mesh<T, pos>::rightof(T *site, Edge<T> *edge)
  {
    return orientation(pos()(*site), pos()(*(edge->dst())), pos()(*(edge->org()))) > 0.0;
  }


  //|///////////////////// incircle /////////////////////////////////////////
  template<typename T, class pos>
  bool Mesh<T, pos>::incircle(T *a, T *b, T *c, T *d)
  {
    if ((a == b) || (a == c) || (a == d) || (b == c) || (b == d) || (c == d))
      return false;

    auto const &ap = pos()(*a);
    auto const &bp = pos()(*b);
    auto const &cp = pos()(*c);
    auto const &dp = pos()(*d);

    auto x1 = get<0>(ap), y1 = get<1>(ap);
    auto x2 = get<0>(bp), y2 = get<1>(bp);
    auto x3 = get<0>(cp), y3 = get<1>(cp);
    auto x4 = get<0>(dp), y4 = get<1>(dp);

    auto da = ((y4-y1)*(x2-x3)+(x4-x1)*(y2-y3))*((x4-x3)*(x2-x1)-(y4-y3)*(y2-y1));
    auto db = ((y4-y3)*(x2-x1)+(x4-x3)*(y2-y1))*((x4-x1)*(x2-x3)-(y4-y1)*(y2-y3));

    return da > db;
  }


  //|///////////////////// delaunay /////////////////////////////////////////
  /// delaunay divide and conquer
  template<typename T, class pos>
  void Mesh<T, pos>::delaunay(size_t low, size_t high, Edge<T> **left, Edge<T> **right)
  {
    if (high - low == 2)
    {
      *left = make_edge(&m_sites[low], &m_sites[low+1]);
      *right = sym(*left);
    }

    else if (high - low == 3)
    {
      Edge<T> *p = make_edge(&m_sites[low], &m_sites[low+1]);
      Edge<T> *q = make_edge(&m_sites[low+1], &m_sites[low+2]);

      splice_edges(sym(p), q);

      auto direction = orientation(pos()(m_sites[low]), pos()(m_sites[low+1]), pos()(m_sites[low+2]));

      if (direction != 0.0)
      {
        Edge<T> *r = make_edge(q, p); 

        if (direction > 0.0)
        {
          *left = p;
          *right = sym(q);
        }
        else
        {
          *left = sym(r);
          *right = r;
        }
      }
      else
      {
        *left = p;
        *right = sym(q);
      }
    }

    else
    {
      size_t mid = (low + high)/2;

      Edge<T> *ldo, *ldi, *rdi, *rdo;

      delaunay(low, mid, &ldo, &ldi);
      delaunay(mid, high, &rdi, &rdo);

      while (true)
      {
        if (leftof(rdi->org(), ldi))
          ldi = ldi->l_next();

        else if (rightof(ldi->org(), rdi))
          rdi = rdi->r_prev();

        else
          break;
      }

      Edge<T> *base = make_edge(sym(rdi), ldi);

      if (ldi->org() == ldo->org())
        ldo = sym(base);

      if (rdi->org() == rdo->org())
        rdo = base;

      while (true)
      {
        Edge<T> *lcand = sym(base)->o_next();
        if (rightof(lcand->dst(), base))
        {
          while (incircle(base->dst(), base->org(), lcand->dst(), lcand->o_next()->dst()))
          {
            Edge<T> *tmp = lcand->o_next();
            destroy_edge(lcand);
            lcand = tmp;
          }
        }

        Edge<T> *rcand = base->o_prev();
        if (rightof(rcand->dst(), base))
        {
          while (incircle(base->dst(), base->org(), rcand->dst(), rcand->o_prev()->dst()))
          {
            Edge<T> *tmp = rcand->o_prev();
            destroy_edge(rcand);
            rcand = tmp;
          }
        }

        if (!rightof(lcand->dst(), base) && !rightof(rcand->dst(), base))
          break;

        if (!rightof(lcand->dst(), base) || (rightof(rcand->dst(), base) && incircle(lcand->dst(), lcand->org(), rcand->org(), rcand->dst())))
          base = make_edge(rcand, sym(base));
        else
          base = make_edge(sym(base), sym(lcand));
      }

      *left = ldo;
      *right = rdo;
    }
  }


  //|///////////////////// Mesh::triangulate ////////////////////////////////
  /// delaunay triangulation
  template<typename T, class pos>
  void Mesh<T, pos>::triangulate()
  {
    if (m_sites.size() < 3)
      return; // FlatInput

    std::sort(m_sites.begin(), m_sites.end(), &less_xy<T, pos>);

    m_sites.erase(unique(m_sites.begin(), m_sites.end(), &equal_xy<T, pos>), m_sites.end());

    Edge<T> *left, *right;

    delaunay(0, m_sites.size(), &left, &right);
  }


  //|///////////////////// triangulate //////////////////////////////////////
  /// delaunay triangulation
  template<typename Polygon>
  void triangulate(Mesh<typename Polygon::value_type> *mesh, std::vector<Polygon> const &polygons)
  {
    //
    // Add all points to the mesh
    //

    for(auto &polygon : polygons)
      mesh->add_sites(polygon.begin(), polygon.end());

    //
    // Triangulate mesh
    //

    mesh->triangulate();
  }

} } } // namespace Delaunay2d

#endif
