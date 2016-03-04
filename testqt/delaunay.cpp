//
// delaunay.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include "shape.h"
#include <leap/lml/geometry.h>
#include <leap/lml/geometry2d.h>
#include <leap/lml/delaunay2d_p.h>
#include <leap/lml/voronoi2d_p.h>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QElapsedTimer>
#include "qpointtraits.h"
#include <iostream>
#include <fstream>

#include <QDebug>

using namespace std;
using namespace leap;
using namespace leap::lml;


////////////// load_shape //////////////////////
QPolygonF load_shape(string const &path)
{
  ifstream fin(path.c_str());
  if (!fin)
    throw runtime_error("Unable to open file: " + path);

  string buffer;

  getline(fin, buffer);
  getline(fin, buffer);
  getline(fin, buffer);

  int count = ato<int>(buffer);

  QPolygonF shape;

  for(int i = 0; i < count; ++i)
  {
    getline(fin, buffer);

    auto values = atov<double>(buffer);

    shape << QPointF(values[0], values[1]);
  }

  double minx = std::numeric_limits<double>::max();
  double maxx = std::numeric_limits<double>::lowest();
  double miny = std::numeric_limits<double>::max();
  double maxy= std::numeric_limits<double>::lowest();

  for(auto &pt : shape)
  {
    minx = min(minx, pt.x());
    maxx = max(maxx, pt.x());
    miny = min(miny, pt.y());
    maxy = max(maxy, pt.y());
  }

  return QTransform().scale(1/(maxx-minx), 1/(maxy-miny)).translate(-(minx + maxx)/2, -(miny + maxy)/2).map(shape);
}


////////////// random //////////////////////////
QPolygonF random(double width, double height, int count)
{
  QPolygonF random;

  for(int i = 0; i < count; ++i)
  {
    random << QPointF(width * (rand() % 10000 - 5000)/10000.0, height* (rand() % 10000 - 5000)/10000.0);
  }

  return random;
}


//--------------------- Main Window -----------------------------------------
//---------------------------------------------------------------------------

class MainWindow : public QWidget
{
  public:
    MainWindow();

  protected:

    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void wheelEvent(QWheelEvent *event);

    virtual void timerEvent(QTimerEvent *event);

    virtual void paintEvent(QPaintEvent *event);

    double m_x;
    double m_y;
    double m_scale;

    QPointF m_MousePos;

    QElapsedTimer m_clock;

  private:

    QPolygonF m_shape;
};

MainWindow::MainWindow()
{
//  m_shape = shape::rect(0.75, 0.75);
//  m_shape = shape::ellipse(0.25, 0.25);
  m_shape = shape::ted(0.75, 0.75);
//  m_shape = load_shape("s.wlr");

//  simplify(m_shape, 1e-7);

  m_x = 0.0;
  m_y = 0.0;
  m_scale = 0.5;

  resize(800, 600);

  setMouseTracking(true);

  startTimer(1000.0/60.0);
}


void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
  m_MousePos = event->pos();
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
  double q = 1.0 + (event->angleDelta().y() * 0.001);

  QPointF locus = QTransform().translate(m_x, m_y).scale(1/m_scale/width(), -1/m_scale/height()).translate(-0.5*width(), -0.5*height()).map(QPointF(event->pos()));

  m_scale = m_scale * q;

  m_x = locus.x() - (locus.x() - m_x)/q;
  m_y = locus.y() - (locus.y() - m_y)/q;
}


void MainWindow::timerEvent(QTimerEvent *event)
{
  update();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing);

  // Painter Transform

  QTransform Q = QTransform().translate(0.5*width(), 0.5*height()).scale(m_scale*width(), -m_scale*width()).translate(-m_x, -m_y);

  painter.setTransform(Q);

  // Painter Grid

  painter.setPen(QPen(QColor(222, 222, 222), 0));

  for(double x = -1.0; x < 1.0; x += 0.1)
    painter.drawLine(QLineF(x, -1, x, 1));

  for(double y = -1.0; y < 1.0; y += 0.1)
    painter.drawLine(QLineF(-1, y, 1, y));

  painter.setPen(QPen(QColor(196, 196, 196), 0));

  painter.drawLine(0, -1, 0, 1);
  painter.drawLine(-1, 0, 1, 0);

  // Shape

  painter.setPen(QPen(QColor(172, 172, 172), 0));

  painter.drawPolygon(m_shape);

#if 1

  // Delaunay

  vector<QPolygonF> polygons;

  polygons.push_back(m_shape);

  long s = clock();

  Delaunay2d::Mesh<QPointF> delaunay;

  Delaunay2d::triangulate(&delaunay, polygons);

  long e = clock();

  qDebug() << (e - s)*1000/CLOCKS_PER_SEC << "milliseconds";

  for(auto &quadedge: delaunay.edges())
  {
    for(auto *edge = quadedge; edge < quadedge+4; edge += 2)
    {
      // Only Triangles
      if (edge->l_next()->l_next() != edge->l_prev())
        continue;

      // Only First Edge (defined as being lowest pointer value)
      if (edge->l_next() < edge || edge->l_prev() < edge)
        continue;

      if (contains(m_shape, centroid(*edge->org(), *edge->dst(), *edge->l_next()->dst())))
        painter.setBrush(QColor(0, 0, 0, 32));
      else
        painter.setBrush(QColor(0, 0, 0, 8));

      painter.drawPolygon(QPolygonF() << *edge->org() << *edge->dst() << *edge->l_next()->dst());
    }
  }

  painter.setPen(QPen(Qt::black, 0));
  painter.setBrush(Qt::NoBrush);

  painter.drawPolygon(m_shape);

#endif

#if 1

  // Voronoi

  Voronoi2d::Voronoi<QPointF> voronoi;

  voronoi.add_site(QPointF(-1, -1));
  voronoi.add_site(QPointF(-1, 1));
  voronoi.add_site(QPointF(1, -1));
  voronoi.add_site(QPointF(1, 1));

  voronoi.add_sites(m_shape.begin(), m_shape.end());

  voronoi.calculate();

  for(auto &cell: voronoi.cells())
  {
    painter.setPen(QPen(Qt::darkBlue, 0));
    painter.setBrush(Qt::darkBlue);
    painter.drawEllipse(cell.site, 0.005, 0.005);

    QPolygonF border;

    for(auto &neighbor : cell.neighbours)
    {
      border << QPointF(neighbor.boundary[0](0), neighbor.boundary[0](1));
      border << QPointF(neighbor.boundary[1](0), neighbor.boundary[1](1));
    }

    painter.setPen(QPen(Qt::darkBlue, 0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(border);
  }

#endif

  // Text

  painter.resetTransform();
  painter.setPen(Qt::black);
  painter.drawText(QPointF(20, height()-20), QString::number(Q.inverted().map(m_MousePos).x(), 'f', 10) + "," + QString::number(Q.inverted().map(m_MousePos).y(), 'f', 10));
}


//|//////////////////// main ////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QApplication::setStyle("fusion");

  try
  {
    MainWindow window;

    window.show();

    return app.exec();
  }
  catch(const exception &e)
  {
    cout << "** " << e.what() << endl;
  }
}
