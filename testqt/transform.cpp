//
// transform.cpp
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
#include <leap/lml/geometry.h>
#include <leap/lml/bound.h>
#include <QApplication>
#include <QWidget>
#include <QSlider>
#include <QPainter>
#include <QElapsedTimer>
#include "qpointtraits.h"
#include <iostream>

#include <QDebug>

using namespace std;
using namespace leap;
using namespace leap::lml;

//--------------------- Main Window -----------------------------------------
//---------------------------------------------------------------------------

class MainWindow : public QWidget
{
  public:
    MainWindow();

  protected:

    virtual void timerEvent(QTimerEvent *event);

    virtual void paintEvent(QPaintEvent *event);

    QSlider *m_dx;
    QSlider *m_dy;

    QSlider *m_sx;
    QSlider *m_sy;

    QElapsedTimer m_clock;

  private:

    QPolygonF m_shape;

    Bound2d m_bound;

    double m_time;
};

MainWindow::MainWindow()
{
  m_shape = shape::ted(1, 1);
  m_bound = make_bound(m_shape.begin(), m_shape.end());

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

  resize(800, 600);

  m_clock.start();

  startTimer(1000.0/60.0);
}


void MainWindow::timerEvent(QTimerEvent *event)
{
  double dt = m_clock.restart() / 1000.0;

  m_time += dt;


  update();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing);

  // Painter Transform

  painter.translate(0.5*width(), 0.5*height());
  painter.scale(0.5*width(), -0.5*width());

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

  double dx = m_dx->value() / 1000.0;
  double dy = m_dy->value() / 1000.0;
  double sx = m_sx->value() / 1000.0;
  double sy = m_sy->value() / 1000.0;

  Matrix3d M = AffineMatrix(ScaleMatrix(sx, sy) * RotationMatrix(m_time), { dx, dy });

  QPolygonF shape;
  for(auto const &i : m_shape)
    shape << transform(M, i);

  painter.setPen(QPen(Qt::black, 0));
  painter.setBrush(QColor(0, 0, 0, 32));

  painter.drawPolygon(shape);

  // OBB

  painter.setPen(QPen(Qt::cyan, 0));
  painter.setBrush(Qt::NoBrush);

  QPolygonF obb;
  obb << transform(M, QPointF(m_bound.low(0), m_bound.low(1)));
  obb << transform(M, QPointF(m_bound.high(0), m_bound.low(1)));
  obb << transform(M, QPointF(m_bound.high(0), m_bound.high(1)));
  obb << transform(M, QPointF(m_bound.low(0), m_bound.high(1)));

  painter.drawPolygon(obb);

  // AABB

  Bound2d aabb = transform(M, m_bound);

  painter.setPen(QPen(Qt::blue, 0));
  painter.setBrush(Qt::NoBrush);

  painter.drawRect(QRectF(aabb.low(0), aabb.low(1), aabb.high(0) - aabb.low(0), aabb.high(1) - aabb.low(1)));

  // Text

  painter.resetTransform();
  painter.setPen(Qt::black);
  painter.drawText(QPointF(width()-50, 40), QString::number(angle(Vector2(0.0, 0.0), transform(M, Vector2(1.0, 0.0)))));
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
