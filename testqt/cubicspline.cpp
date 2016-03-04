//
// cubicspline.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <leap/lml/cubicspline.h>
#include <leap/lml/interpolation.h>
#include <QApplication>
#include <QWidget>
#include <QSlider>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include "qpointtraits.h"
#include <iostream>
#include <fstream>

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

    virtual void mousePressEvent(QMouseEvent *event);

    virtual void wheelEvent(QWheelEvent *event);

    virtual void timerEvent(QTimerEvent *event);

    virtual void paintEvent(QPaintEvent *event);

    QSlider *m_in;
    QSlider *m_fi;

    double m_x;
    double m_y;
    double m_scale;

    std::vector<QPointF> m_points;
};

MainWindow::MainWindow()
{
  m_x = 0.0;
  m_y = 0.0;
  m_scale = 0.5;

  m_in = new QSlider(Qt::Horizontal, this);
  m_in->setRange(-15000, 15000);
  m_in->setValue(0);
  m_in->resize(200, 25);
  m_in->move(10, 10);

  m_fi = new QSlider(Qt::Horizontal, this);
  m_fi->setRange(-15000, 15000);
  m_fi->setValue(0);
  m_fi->resize(200, 25);
  m_fi->move(10, 30);

  resize(800, 600);

  setMouseTracking(true);

  startTimer(1000.0/60.0);
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
  QTransform Q = QTransform().translate(0.5*width(), 0.5*height()).scale(m_scale*width(), -m_scale*width()).translate(-m_x, -m_y);

  m_points.push_back(Q.inverted().map(QPointF(event->pos())));

  sort(m_points.begin(), m_points.end(), [](QPointF const &lhs, QPointF const &rhs) { return lhs.x() < rhs.x(); });
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

  // Points

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(196, 196, 196));

  for(auto &point : m_points)
  {
    painter.drawEllipse(point, 0.01, 0.01);
  }

  // Cubic Interpolation

  if (m_points.size() > 1)
  {
    vector<double> xa;
    vector<double> ya;

    for(auto &point : m_points)
    {
      xa.push_back(point.x());
      ya.push_back(point.y());
    }

    QPainterPath path;

    path.moveTo(m_points.front());

    for(double x = xa.front(); x < xa.back(); x += 0.001)
    {
      path.lineTo(QPointF(x, interpolate<cubic>(xa, ya, x)));
    }

    path.lineTo(m_points.back());

    painter.setPen(QPen(Qt::darkBlue, 0));
    painter.setBrush(Qt::NoBrush);

    painter.drawPath(path);
  }

  // Cubic Spline

  if (m_points.size() > 1)
  {
    double in = m_in->value() / 1000.0;
    double fi = m_fi->value() / 1000.0;

    CubicSpline<QPointF> spline(m_points, in, fi);

    QPainterPath path;

    path.moveTo(m_points.front());

    for(double x = m_points.front().x(); x < m_points.back().x(); x += 0.001)
    {
      path.lineTo(QPointF(x, spline.value(x)));
    }

    path.lineTo(m_points.back());

    painter.setPen(QPen(Qt::black, 0));
    painter.setBrush(Qt::NoBrush);

    painter.drawPath(path);
  }

  // Cubic Spline Derivative

  if (m_points.size() > 1)
  {
    double in = m_in->value() / 1000.0;
    double fi = m_fi->value() / 1000.0;

    CubicSpline<QPointF> spline(m_points, in, fi);

    QPainterPath path;

    path.moveTo(QPointF(m_points.front().x(), spline.derivative(m_points.front().x())));

    for(double x = m_points.front().x(); x < m_points.back().x(); x += 0.001)
    {
      path.lineTo(QPointF(x, spline.derivative(x)));
    }

    path.lineTo(QPointF(m_points.back().x(), spline.derivative(m_points.back().x())));

    painter.setPen(QPen(Qt::lightGray, 0));
    painter.setBrush(Qt::NoBrush);

    painter.drawPath(QTransform().scale(1.0, 0.1).map(path));
  }
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
