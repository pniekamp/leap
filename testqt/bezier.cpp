//
// bezier.cpp
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#include <leap/lml/bezier.h>
#include <QApplication>
#include <QWidget>
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

    double m_x;
    double m_y;
    double m_scale;

    double m_time;

    std::vector<QPointF> m_points;
};

MainWindow::MainWindow()
{
  m_x = 0.0;
  m_y = 0.0;
  m_scale = 0.5;

  m_time = 0.0f;

  resize(800, 600);

  setMouseTracking(true);

  startTimer(1000.0/60.0);
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
  QTransform Q = QTransform().translate(0.5*width(), 0.5*height()).scale(m_scale*width(), -m_scale*width()).translate(-m_x, -m_y);

  m_points.push_back(Q.inverted().map(QPointF(event->pos())));
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
  m_time += 0.01;

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

  // Bezier

  if (m_points.size() > 1)
  {
    Bezier<QPointF> bezier(m_points);

    QPainterPath path;

    path.moveTo(bezier.value(0.0f));

    for(float t = 0; t < 1.0; t += 0.001)
    {
      path.lineTo(bezier.value(t));
    }

    path.lineTo(bezier.value(1.0f));

    painter.setPen(QPen(Qt::black, 0));
    painter.setBrush(Qt::NoBrush);

    painter.drawPath(path);

    for(size_t i = 0; i < m_points.size()-1; ++i)
    {
      painter.drawEllipse(translate(m_points[i], bezier.controls()[2*i]), 0.01, 0.01);
      painter.drawEllipse(translate(m_points[i+1], bezier.controls()[2*i+1]), 0.01, 0.01);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 164));

    painter.drawEllipse(bezier.value(fmod(m_time, 1.0)), 0.01, 0.01);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 164, 0));

    painter.drawEllipse(bezier.value(remap(bezier, fmod(m_time, 1.0)*bezier.length())), 0.01, 0.01);
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
  catch(exception &e)
  {
    cout << "** " << e.what() << endl;
  }
}
