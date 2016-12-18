//
// rtree.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <iostream>
#include <leap/lml/vector.h>
#include <leap/lml/rtree.h>
#include <leap/lml/rtreesearch.h>
#include <leap/lml/octree.h>
#include <leap/lml/geometry.h>
#include <leap/lml/io.h>
#include <fstream>

using namespace std;
using namespace leap;
using namespace leap::lml;

class Object
{
  public:
    Object(double x, double y, double w = 0.0, double h = 0.0)
      : m_x(x), m_y(y), m_w(w), m_h(h)
    {
    }

    Vector2d pos() const { return Vector2(m_x, m_y); }

    Bound2d box() const { return make_bound(Vector2(m_x, m_y), Vector2(m_x+m_w, m_y+m_h)); }

    double m_x, m_y, m_w, m_h;
};


auto distsqr(Object const &object, Vector2d const &pt)
{
  return distsqr(object.pos(), pt);
}

auto contains(Bound2d const &bound, Object const &object)
{
  return contains(bound, object.pos());
}


//|//////////////////// DumpTest ////////////////////////////////////////////
template<typename Tree>
void DumpTest(Tree const &tree)
{
  int nodes = 0;
  int items = 0;

  for(auto i = tree.begin(); i != tree.end(); ++i)
  {
    ++nodes;

    for(auto j = i.items().begin(); j != i.items().end(); ++j)
      ++items;

    i.descend();
  }

  cout << "  " << nodes << " nodes, " << items << " items\n";
}


//|//////////////////// RTreeTest1 //////////////////////////////////////////
void RTreeTest1()
{
  RTree2d<Object> tree;

  for(auto k = bounded_iterator(tree.begin(), make_bound(Vector2(0.0, 0.0), 0.0)); k != tree.end(); ++k)
  {
    cout << " ** Error on Empty Bounded Iteration\n";
  }

  tree.insert(Object(10, 5));
  tree.insert(Object(20, 15));
  tree.insert(Object(20, 7));
  tree.insert(Object(25, 11));
  tree.insert(Object(30, 14));
  tree.insert(Object(25, 17));
  tree.insert(Object(35, 21));
  tree.insert(Object(40, 29, 5, 5));

  for(int i = 0; i < 50; ++i)
    tree.insert(Object(rand() % 50, rand() % 50));

  DumpTest(tree);

  int items = 0;
  cout << "  Bounded: ";
  for(auto k = bounded_iterator(tree.begin(), make_bound(Vector2(0.0, 0.0), 15.0)); k != tree.end(); ++k)
  {
    cout << "(" << k->m_x << "," << (*k).m_y << ") ";

    ++items;
  }

  cout << ": " << items << " items\n";

  distance(bounded_iterator(tree.begin(), make_bound(Vector2(0.0, 0.0), 15.0)), bounded_iterator(tree.end(), {}));

  auto nearest = nearest_neighbour(tree, Vector2(10.0, 10.0));

  cout << "  Nearest: " << "(" << nearest->m_x << "," << nearest->m_y << ")\n";

  RTree2d<Object> const tree2 = tree;

  auto nearest2 = nearest_neighbour(tree2, Vector2(10.0, 10.0));

  cout << "  Nearest: " << "(" << nearest2->m_x << "," << nearest2->m_y << ")\n";

  DumpTest(tree2);
}


//|//////////////////// RTreeTest2 //////////////////////////////////////////
void RTreeTest2()
{
  vector<Object> objects;
  for(int i = 0; i < 100; ++i)
    objects.push_back(Object(rand() % 50, rand() % 50));

  RTree2d<Object const *> tree;

  for(auto &object : objects)
    tree.insert(&object);

  DumpTest(tree);

  int items = 0;
  cout << "  Bounded: ";
  for(auto k = bounded_iterator(tree.begin(), make_bound(Vector2(0.0, 0.0), 15.0)); k != tree.end(); ++k)
  {
    cout << "(" << (*k)->m_x << "," << (*k)->m_y << ") ";

    ++items;
  }

  cout << ": " << items << " items\n";

  distance(bounded_iterator(tree.begin(), make_bound(Vector2(0.0, 0.0), 15.0)), bounded_iterator(tree.end(), {}));

  auto nearest = nearest_neighbour(tree, Vector2(10.0, 10.0));

  cout << "  Nearest: " << "(" << (*nearest)->m_x << "," << (*nearest)->m_y << ")\n";

  while (nearest)
  {
//    cout << "  Nearest: " << "(" << (*nearest)->m_x << "," << (*nearest)->m_y << ")" << " : " << dist((*nearest)->pos(), Vector2(10.0, 10.0)) << "\n";

    tree.remove(*nearest);

    nearest = nearest_neighbour(tree, Vector2(10.0, 10.0));
  }
}


template<typename Iterator>
void RecursiveDumpTest(Iterator node, int *nodes, int *items)
{
  *nodes += 1;
  *items += node.items().size();

  for(size_t i = 0; i < node.children(); ++i)
  {
    RecursiveDumpTest(node.child(i), nodes, items);
  }
}


//|//////////////////// OcTreeTest //////////////////////////////////////////
void OcTreeTest()
{
  OcTree2d<Object> tree(Bound2d({ -1000, -1000 }, { 1000, 1000 }));

  cout << "  " << tree.world() << endl;

  for(int i = 0; i < 58; ++i)
    tree.insert(Object(rand() % 50, rand() % 50));

  DumpTest(tree);

  OcTree2d<Object> const tree2 = tree;

  DumpTest(tree2);

  int nodes = 0;
  int items = 0;
  RecursiveDumpTest(tree.begin(), &nodes, &items);
  cout << "  " << nodes << " nodes, " << items << " items\n";
}


//|//////////////////// RTreeTest ///////////////////////////////////////////
void RTreeTest()
{
  cout << "Test RTree\n";

  RTreeTest1();

  RTreeTest2();

  cout << endl;

  cout << "Test OcTree\n";

  OcTreeTest();

  cout << endl;
}
