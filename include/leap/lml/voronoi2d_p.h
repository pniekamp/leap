//
// voronoi 2d diagram
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein may be used for any purpose provided
// this copyright notice is retained
//

#pragma once

#include "lml.h"
#include "point.h"
#include "delaunay2d_p.h"

//|--------------------- voronoi 2d diagram -------------------------------
//|------------------------------------------------------------------------

namespace leap { namespace lml { namespace Voronoi2d
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


  //|--------------------- Cell -------------------------------------------
  //|----------------------------------------------------------------------

  template<typename T, typename Alloc>
  class Cell
  {
    public:
      Cell(T const &site, Alloc const &allocator) noexcept
        : site(site),
          neighbours(allocator)
      {
        visited = false;
      }

      T site;

      struct Neighbour
      {
        Cell<T, Alloc> *cell;

        Vector2d boundary[2];
      };

      std::vector<Neighbour, typename std::allocator_traits<Alloc>::template rebind_alloc<Neighbour>> neighbours;

      bool visited;
  };


  //|///////////////////// circle_centre ////////////////////////////////////
  template<typename Point>
  Vector2d circle_centre(Point const &a, Point const &b, Point const &c)
  {
    double s = 0.5*((get<0>(b)-get<0>(c))*(get<0>(a)-get<0>(c)) - (get<1>(b)-get<1>(c))*(get<1>(c)-get<1>(a))) / ((get<0>(a)-get<0>(b))*(get<1>(c)-get<1>(a)) - (get<1>(b)-get<1>(a))*(get<0>(a)-get<0>(c)));

    return Vector2(0.5*(get<0>(a) + get<0>(b)) + s*(get<1>(b) - get<1>(a)), 0.5*(get<1>(a) + get<1>(b)) + s*(get<0>(a) - get<0>(b)));
  }


  //|--------------------- Voronoi ----------------------------------------
  //|----------------------------------------------------------------------

  template<typename T, class pos = pos<T>, typename Alloc = std::allocator<T>>
  class Voronoi
  {
    public:

      using allocator_type = Alloc;
      using cells_allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Cell<T, Alloc>>;

      using cell_type = Cell<T, Alloc>;
      using cells_type = std::vector<Cell<T, Alloc>, cells_allocator_type>;

    public:
      Voronoi(Alloc const &allocator = Alloc()) noexcept;
      Voronoi(Voronoi const &) = delete;
      Voronoi(Voronoi &&) noexcept = delete;

      void add_site(T const &site);

      template<typename InputIterator>
      void add_sites(InputIterator f, InputIterator l);

    public:

      void calculate();

      cells_type &cells() { return m_mesh.sites(); }

    protected:

      allocator_type m_allocator;

    private:

      struct cellpos
      {
        decltype(auto) operator()(Cell<T, Alloc> const &cell) const
        {
          return pos()(cell.site);
        }
      };

      Delaunay2d::Mesh<Cell<T, Alloc>, cellpos, cells_allocator_type> m_mesh;
  };


  //|///////////////////// Voronoi::Constructor//////////////////////////////
  template<typename T, class pos, typename Alloc>
  Voronoi<T, pos, Alloc>::Voronoi(Alloc const &allocator) noexcept
    : m_allocator(allocator),
      m_mesh(allocator)
  {
  }


  //|///////////////////// Voronoi::add_site ////////////////////////////////
  template<typename T, class pos, typename Alloc>
  void Voronoi<T, pos, Alloc>::add_site(T const &site)
  {
    m_mesh.add_site(Cell<T, Alloc>(site, m_allocator));
  }


  //|///////////////////// Voronoi::add_sites ////////////////////////////////
  template<typename T, class pos, typename Alloc>
  template<typename InputIterator>
  void Voronoi<T, pos, Alloc>::add_sites(InputIterator f, InputIterator l)
  {
    for(InputIterator i = f; i != l; ++i)
      m_mesh.add_site(Cell<T, Alloc>(*i, m_allocator));
  }


  //|///////////////////// Voronoi::calculate ///////////////////////////////
  /// voronoi diagram
  template<typename T, class pos, typename Alloc>
  void Voronoi<T, pos, Alloc>::calculate()
  {
    m_mesh.triangulate();

    for(auto i = m_mesh.edges().begin(); i != m_mesh.edges().end(); ++i)
    {
      for(auto *edge = (*i); edge < (*i)+4; edge += 2)
      {
        auto cell = edge->org();

        if (cell->visited)
          continue;

        auto *curr = edge;

        do
        {
          if (curr->l_next()->l_next() == curr->l_prev() && curr->r_prev()->r_prev() == curr->r_next())
          {
            typename Cell<T, Alloc>::Neighbour neighbour;

            neighbour.cell = curr->dst();
            neighbour.boundary[0] = circle_centre(pos()(curr->org()->site), pos()(curr->dst()->site), pos()(curr->r_prev()->dst()->site));
            neighbour.boundary[1] = circle_centre(pos()(curr->org()->site), pos()(curr->dst()->site), pos()(curr->l_next()->dst()->site));

            cell->neighbours.push_back(neighbour);
          }

          curr = curr->o_next();

        } while (curr != edge);

        cell->visited = true;
      }
    }
  }

} } } // namespace Voronoi2d
