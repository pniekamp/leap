//
// polygonsetop.cpp
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

const double rad2deg = 180/pi<double>();
const double deg2rad = pi<double>()/180;


////////////// spiral //////////////////////////
QPolygonF spiral(double r1, double r2, double width)
{
  double dr = (r2 - r1) / 300.0;
  double da = 4.0 * deg2rad;

  QPolygonF spiral;

  for(double radius = r1, angle = 0; radius < r2; radius += dr, angle += da)
  {
    auto a = Vector2(radius * cos(angle), radius * sin(angle));
    auto b = Vector2((radius+dr) * cos(angle+da+da/4), (radius+dr) * sin(angle+da+da/4));

    auto oab = normalise(Vector2(b(1) - a(1), a(0) - b(0)));

    auto a1 = Vector2(a(0) + width/2*oab(0), a(1) + width/2*oab(1));
    auto a2 = Vector2(a(0) - width/2*oab(0), a(1) - width/2*oab(1));

    auto b1 = Vector2(b(0) + width/2*oab(0), b(1) + width/2*oab(1));
    auto b2 = Vector2(b(0) - width/2*oab(0), b(1) - width/2*oab(1));

    QPolygonF sect = QPolygonF() << QPointF(a2(0), a2(1)) << QPointF(a1(0), a1(1)) << QPointF(b1(0), b1(1)) << QPointF(b2(0), b2(1));

    spiral = boolean_union(spiral, sect)[0];
  }

  simplify(spiral, 1e-6);

  return spiral;
}


////////////// fan /////////////////////////////
QPolygonF fan(double rx, double ry, double radius)
{
  double da = 10.0 * deg2rad;

  QPolygonF fan = shape::ellipse(rx, ry);

  for(double angle = 0; angle < 6.28; angle += da)
  {
    QPolygonF sect = QPolygonF() << QPointF(0, 0) << QPointF(radius*cos(angle), radius*sin(angle)) << QPointF(0.8*radius*cos(angle+da), 0.8*radius*sin(angle+da));

    fan = boolean_union(fan, sect)[0];
  }

  return fan;
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


////////////// parse_case //////////////////////
void parse_case(QPolygonF *p, QPolygonF *q, string str[2])
{
  auto a = atov<double>(str[0].substr(9));

  for(size_t i = 0; i < a.size()-2; i += 2)
    p->push_back(QPointF(a[i], a[i+1]));

  auto b = atov<double>(str[1].substr(9));

  for(size_t i = 0; i < b.size()-2; i += 2)
    q->push_back(QPointF(b[i], b[i+1]));

  reverse(p->begin(), p->end());
  reverse(q->begin(), q->end());
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

    QSlider *m_dx;
    QSlider *m_dy;

    QSlider *m_sx;
    QSlider *m_sy;

    QPushButton *m_play;

    double m_x;
    double m_y;
    double m_scale;

    QPointF m_MousePos;

    QElapsedTimer m_clock;

  private:

    QPolygonF m_shape1;
    QPolygonF m_shape2;

    double m_time;
};

MainWindow::MainWindow()
{
//  m_shape1 = shape::rect(0.75, 0.75);
//  m_shape1 = shape::ellipse(0.25, 0.25);
  m_shape1 = spiral(0.1, 0.5, 0.05);

//  m_shape2 = shape::rect(0.75, 0.75);
//  m_shape2 = shape::horseshoe(0.75, 0.75);
  m_shape2 = shape::star(0.75, 0.75);
//  m_shape2 = shape::ted(0.75, 0.75);
//  m_shape2 = shape::ellipse(0.25, 0.25);
//  m_shape2 = shape::knot(0.25, 0.25);
//  m_shape2 = shape::complex(0.75, 0.75);
//  m_shape2 = random(1.0, 1.0, 16);
//  m_shape2 = fan(0.15, 0.15, 0.4);

  m_x = 0.0;
  m_y = 0.0;
  m_scale = 0.5;

  m_dx = new QSlider(Qt::Horizontal, this);
  m_dx->setRange(-1000, 1000);
  m_dx->setValue(0);
  m_dx->resize(200, 25);
  m_dx->move(10, 10);

  m_dy = new QSlider(Qt::Horizontal, this);
  m_dy->setRange(-1000, 1000);
  m_dy->setValue(0);
  m_dy->resize(200, 25);
  m_dy->move(10, 30);

  m_sx = new QSlider(Qt::Horizontal, this);
  m_sx->setRange(1, 1500);
  m_sx->setValue(1000);
  m_sx->resize(200, 25);
  m_sx->move(10, 50);

  m_sy = new QSlider(Qt::Horizontal, this);
  m_sy->setRange(1, 1500);
  m_sy->setValue(1000);
  m_sy->resize(200, 25);
  m_sy->move(10, 70);

  m_play = new QPushButton("O", this);
  m_play->setCheckable(true);
  m_play->resize(25, 25);
  m_play->move(10, 98);

  resize(800, 600);

  m_clock.start();

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
  double dt = m_clock.restart() / 1000.0;

  if (m_play->isChecked())
  {
    m_time += dt;
  }

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

  // Shape 1

  Matrix3d M1 = AffineMatrix(RotationMatrix(m_time*0.1), { 0.0, 0.0 });

  QPolygonF shape1;
  for(auto const &i : m_shape1)
    shape1 << transform(M1, i);

  painter.setPen(QPen(QColor(172, 172, 172), 0));

  painter.drawPolygon(shape1);

  // Shape 2

  double dx = m_dx->value() / 1000.0;
  double dy = m_dy->value() / 1000.0;
  double sx = m_sx->value() / 1000.0;
  double sy = m_sy->value() / 1000.0;

  Matrix3d M2 = AffineMatrix(RotationMatrix(m_time*0.2)*ScaleMatrix(sx, sy), { dx, dy });

  QPolygonF shape2;
  for(auto const &i : m_shape2)
    shape2 << transform(M2, i);

  painter.setPen(QPen(QColor(172, 172, 172), 0));

  painter.drawPolygon(shape2);

#if 1
  // Union

  vector<QPolygonF> U = boolean_union(shape1, shape2);

  painter.setPen(QPen(Qt::black, 0));
  painter.setBrush(QColor(0, 0, 0, 32));

  QPainterPath UU;

  for(auto const &i : U)
  {
    UU.addPolygon(i);
    UU.closeSubpath();
  }

  painter.drawPath(UU);
#endif

#if 0
  // Intersection

  long s = clock();

  vector<QPolygonF> I = boolean_intersection(shape1, shape2);

  long e = clock();

  qDebug() << (e - s)*1000/CLOCKS_PER_SEC << "milliseconds";

  painter.setPen(QPen(Qt::blue, 0));
  painter.setBrush(QColor(0, 0, 120, 32));

  QPainterPath II;

  for(auto const &i : I)
  {
    II.addPolygon(i);
    II.closeSubpath();    
  }

  painter.drawPath(II);
#endif

#if 0

  // Difference

  vector<QPolygonF> D = boolean_difference(shape1, shape2);

  painter.setPen(QPen(Qt::blue, 0));
  painter.setBrush(QColor(0, 0, 120, 32));

  QPainterPath DD;

  for(auto const &i : D)
  {
    DD.addPolygon(i);
    DD.closeSubpath();
  }

  painter.drawPath(DD);

#endif

#if 0

  // Simplify

  vector<QPolygonF> S = boolean_simplify(shape2);

  painter.setPen(QPen(QColor(222, 0, 64, 196), 0));
  painter.setBrush(QColor(222, 0, 64, 32));

  QPainterPath SS;

  for(auto const &i : S)
  {
    SS.addPolygon(i);
    SS.closeSubpath();
  }

  painter.drawPath(SS);

  painter.drawPolygon(QTransform().translate(0.7, -0.4).scale(0.3, 0.3).map(shape2), Qt::WindingFill);
  painter.drawPolygon(QTransform().translate(-0.7, -0.4).scale(0.3, 0.3).map(shape2), Qt::OddEvenFill);

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
  catch(exception &e)
  {
    cout << "** " << e.what() << endl;
  }
}
