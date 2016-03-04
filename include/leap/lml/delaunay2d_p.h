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


  //|///////////////////// rightof //////////////////////////////////////////
  template<typename T>
  bool rightof(T *site, Edge<T> *edge)
  {
    return orientation(*site, *(edge->dst()), *(edge->org())) > 0.0;
  }


  //|///////////////////// leftof ///////////////////////////////////////////
  template<typename T>
  bool leftof(T *site, Edge<T> *edge)
  {
    return orientation(*site, *(edge->org()), *(edge->dst())) > 0.0;
  }


  //|///////////////////// incircle /////////////////////////////////////////
  template<typename Point>
  bool incircle(Point *a, Point *b, Point *c, Point *d)
  {
    if ((a == b) || (a == c) || (a == d) || (b == c) || (b == d) || (c == d))
      return false;

    double x1 = get<0>(*a), y1 = get<1>(*a);
    double x2 = get<0>(*b), y2 = get<1>(*b);
    double x3 = get<0>(*c), y3 = get<1>(*c);
    double x4 = get<0>(*d), y4 = get<1>(*d);
    
    double da = ((y4-y1)*(x2-x3)+(x4-x1)*(y2-y3))*((x4-x3)*(x2-x1)-(y4-y3)*(y2-y1));
    double db = ((y4-y3)*(x2-x1)+(x4-x3)*(y2-y1))*((x4-x1)*(x2-x3)-(y4-y1)*(y2-y3));

    return da > db;
  }


  //|--------------------- Mesh -------------------------------------------
  //|----------------------------------------------------------------------
  template<typename T>
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

      void delaunay(int low, int high, Edge<T> **left, Edge<T> **right);

    private:

      std::vector<T> m_sites;

      std::vector<Edge<T>*> m_edges;
  };


  //|///////////////////// Mesh::Constructor/////////////////////////////////
  template<typename T>
  Mesh<T>::Mesh()
  {
  }


  //|///////////////////// Mesh::Destructor//////////////////////////////////
  template<typename T>
  Mesh<T>::~Mesh()
  {
    for(auto i = m_edges.begin(); i != m_edges.end(); ++i)
      delete[] *i;
  }


  //|///////////////////// Mesh::add_site ///////////////////////////////////
  /// add a site to a Mesh Object
  template<typename T>
  void Mesh<T>::add_site(T const &site)
  {
    m_sites.push_back(site);
  }


  //|///////////////////// Mesh::add_points /////////////////////////////////
  /// add sites to a Mesh Object
  template<typename T>
  template<typename InputIterator>
  void Mesh<T>::add_sites(InputIterator f, InputIterator l)
  {
    std::copy(f, l, std::back_inserter(m_sites));
  }


  //|///////////////////// Mesh::make_edge //////////////////////////////////
  /// make a quad-edge object
  template<typename T>
  Edge<T> *Mesh<T>::make_edge(T *org, T *dst)
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
  template<typename T>
  void Mesh<T>::destroy_edge(Edge<T> *edge)
  {
    splice_edges(edge, edge->o_prev());
    splice_edges(sym(edge), sym(edge)->o_prev());

    m_edges.erase(std::find(m_edges.begin(), m_edges.end(), edge->base()));

    delete[] edge->base();
  }


  //|///////////////////// Mesh::splice_edges ///////////////////////////////
  /// splice two quad-edge objects
  template<typename T>
  void Mesh<T>::splice_edges(Edge<T> *a, Edge<T> *b)
  {
    Edge<T> *p = rot(a->next);
    Edge<T> *q = rot(b->next);

    std::swap(a->next, b->next);
    std::swap(p->next, q->next);
  }


  //|///////////////////// Mesh::make_edge //////////////////////////////////
  template<typename T>
  Edge<T> *Mesh<T>::make_edge(Edge<T> *a, Edge<T> *b)
  {
    Edge<T> *quad = make_edge(a->dst(), b->org());

    splice_edges(quad, a->l_next());
    splice_edges(sym(quad), b);

    return quad;
  }


  //|///////////////////// delaunay /////////////////////////////////////////
  /// delaunay divide and conquer
  template<typename T>
  void Mesh<T>::delaunay(int low, int high, Edge<T> **left, Edge<T> **right)
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

      auto direction = orientation(m_sites[low], m_sites[low+1], m_sites[low+2]);

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
      int mid = (low + high)/2;

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
  template<typename T>
  void Mesh<T>::triangulate()
  {
    if (m_sites.size() < 3)
      return; // FlatInput

    std::sort(m_sites.begin(), m_sites.end(), &less_xy<T>);

    m_sites.erase(unique(m_sites.begin(), m_sites.end(), &equal_xy<T>), m_sites.end());

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
