//
// polygonbrush.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include "shape.h"
#include <leap/lml/vector.h>
#include <leap/lml/matrix.h>
#include <leap/lml/matrixconstants.h>
#include <leap/lml/bound.h>
#include <leap/lml/geometry2d.h>
#include <QApplication>
#include <QWidget>
#include <QSlider>
#include <QPushButton>
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

namespace leap { namespace lml
{
  template<>
  struct ring_traits<std::vector<QPolygonF>>
  {
    static constexpr size_t dimension = 2;

    static constexpr int orientation = 1;
  };

} }


//|///////////////////// contains ///////////////////////////////////////
bool contains(vector<QPolygonF> const &polygons, QPointF const &pt)
{
  size_t count = 0;

  for(auto &polygon : polygons)
    count += contains(polygon, pt);

  return count & 1;
}


//|///////////////////// setop //////////////////////////////////////////
void setop(vector<QPolygonF> *result, vector<QPolygonF> const &p, vector<QPolygonF> const &q, PolygonSetOp::Op op)
{
  using namespace PolygonSetOp;

  int pc = 0;
  for(auto &pts : p)
    pc += pts.size();

  int qc = 0;
  for(auto &pts : q)
    qc += pts.size();

  Graph<QPointF> graph(pc, qc);

  for(auto &pts : p)
  {
    switch (op)
    {
      case Op::Union:
        graph.push_p(pts.begin(), pts.end());
        break;

      case Op::Intersection:
        graph.push_p(pts.begin(), pts.end());
        break;

      case Op::Difference:
        graph.push_p(pts.begin(), pts.end());
        break;
    }
  }

  for(auto &pts : q)
  {
    switch (op)
    {
      case Op::Union:
        graph.push_q(pts.begin(), pts.end());
        break;

      case Op::Intersection:
        graph.push_q(pts.begin(), pts.end());
        break;

      case Op::Difference:
        graph.push_q(std::reverse_iterator<decltype(pts.end())>(pts.end()), std::reverse_iterator<decltype(pts.begin())>(pts.begin()));
        break;
    }
  }

  graph.join();

  polygon_setop(result, graph, p, q, op);
}


//--------------------- Main Window -----------------------------------------
//---------------------------------------------------------------------------

class MainWindow : public QWidget
{
  public:
    MainWindow();

  protected:

    void apply();

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void wheelEvent(QWheelEvent *event);

    virtual void timerEvent(QTimerEvent *event);

    virtual void paintEvent(QPaintEvent *event);

    double m_x;
    double m_y;
    double m_scale;

    QPointF m_MousePos;
    QPointF m_MousePressPos;

    QElapsedTimer m_clock;

  private:

    PolygonSetOp::Op m_op;

    QPolygonF m_brush;

    vector<QPolygonF> m_shape;

    double m_time;
};

MainWindow::MainWindow()
{
  m_x = 0.0;
  m_y = 0.0;
  m_scale = 0.5;

  m_op = PolygonSetOp::Op::Union;

  m_brush = shape::ellipse(20, 20);

  resize(800, 600);

  m_clock.start();

  setMouseTracking(true);

  startTimer(1000.0/60.0);
}


void MainWindow::apply()
{
  std::vector<QPolygonF> result;

  setop(&result, m_shape, { m_brush }, m_op);

  m_shape = result;
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
  m_MousePressPos = event->pos();

  if (event->modifiers() & Qt::ShiftModifier)
    m_op = PolygonSetOp::Op::Difference;

  apply();

  update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
  m_MousePressPos = QPointF();

  m_op = PolygonSetOp::Op::Union;
}


void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
  m_MousePos = event->pos();

  QTransform Q = QTransform().translate(0.5*width(), 0.5*height()).scale(m_scale*width(), m_scale*width()).translate(-m_x, -m_y);

  m_brush = Q.inverted().map(QTransform().translate(m_MousePos.x(), m_MousePos.y()).map(shape::ellipse(20, 20)));

  if (!m_MousePressPos.isNull())
    apply();

  update();
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
  double q = 1.0 + (event->angleDelta().y() * 0.001);

  QPointF locus = QTransform().translate(m_x, m_y).scale(1/m_scale/width(), 1/m_scale/height()).translate(-0.5*width(), -0.5*height()).map(QPointF(event->pos()));

  m_scale = m_scale * q;

  m_x = locus.x() - (locus.x() - m_x)/q;
  m_y = locus.y() - (locus.y() - m_y)/q;

  update();
}


void MainWindow::timerEvent(QTimerEvent *event)
{
}


void MainWindow::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing);

  // Painter Transform

  QTransform Q = QTransform().translate(0.5*width(), 0.5*height()).scale(m_scale*width(), m_scale*width()).translate(-m_x, -m_y);

  painter.setTransform(Q);

  // Brush

  painter.setPen(QPen(Qt::lightGray, 0));
  painter.setBrush(Qt::NoBrush);

  painter.drawPolygon(m_brush);

  // Shape

  painter.setPen(QPen(Qt::darkGray, 0));
  painter.setBrush(QColor(222, 222, 222, 222));

  QPainterPath UU;

  for(auto const &i : m_shape)
  {
    UU.addPolygon(i);
    UU.closeSubpath();
  }

  painter.drawPath(UU);
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
