//
// shape.h
//
// Copyright (C) Peter Niekamp
//
// This code remains the property of the copyright holder.
// The code contained herein is licensed for use without limitation
//

#ifndef SHAPE_HH
#define SHAPE_HH

#include <QPolygonF>
#include <QTransform>
#include <QDebug>
#include <cmath>

namespace shape
{

  QPolygonF rect(double width, double height)
  {
    QPolygonF R;

    R << QPointF(-width/2, -height/2);
    R << QPointF(width/2, -height/2);
    R << QPointF(width/2, height/2);
    R << QPointF(-width/2, height/2);

    return R;
  }


  QPolygonF horseshoe(double width, double height)
  {
    QPolygonF R;

    R << QPointF(-width/2, -height/2);
    R << QPointF(width/2, -height/2);
    R << QPointF(width/2, height/2);
    R << QPointF(-width/2, height/2);
    R << QPointF(-width/2, 0.7*height/2);
    R << QPointF(-0.7*width/2, 0.7*height/2);
    R << QPointF(-0.7*width/2, -0.7*height/2);
    R << QPointF(-width/2, -0.7*height/2);

    return R;
  }


  QPolygonF ellipse(double rx, double ry)
  {
    double da = 4.0 * 3.14159265358979323846/180.0;

    QPolygonF ellipse;

    for(double angle = 0; angle < 6.28; angle += da)
    {
      ellipse.push_back(QPointF(rx * cos(angle), ry * sin(angle)));
    }

    return ellipse;
  }


  QPolygonF star(double width, double height)
  {
    QPolygonF R;

    R << QPointF(0.0*width/2, 1.0*height/2);
    R << QPointF(-0.243697*width/2, 0.238938*height/2);
    R << QPointF(-1*width/2, 0.238938*height/2);
    R << QPointF(-0.394958*width/2, -0.238938*height/2);
    R << QPointF(-0.613445*width/2, -1.0*height/2);
    R << QPointF(0*width/2, -0.637168*height/2);
    R << QPointF(0.613445*width/2, -1.0*height/2);
    R << QPointF(0.394958*width/2, -0.238938*height/2);
    R << QPointF(1.0*width/2, 0.238938*height/2);
    R << QPointF(0.243697*width/2, 0.238938*height/2);

    return R;
  }

  QPolygonF ted(double width, double height)
  {
    QPolygonF R;

    R << QPointF(280.35714, -648.79075);
    R << QPointF(286.78571, -662.8979);
    R << QPointF(263.28607, -661.17871);
    R << QPointF(262.31092, -671.41548);
    R << QPointF(250.53571, -677.00504);
    R << QPointF(250.53571, -683.43361);
    R << QPointF(256.42857, -685.21933);
    R << QPointF(297.14286, -669.50504);
    R << QPointF(289.28571, -649.50504);
    R << QPointF(285.0, -631.6479);
    R << QPointF(285.0, -608.79075);
    R << QPointF(292.85714, -585.21932);
    R << QPointF(306.42857, -563.79075);
    R << QPointF(323.57143, -548.79075);
    R << QPointF(339.28571, -545.21932);
    R << QPointF(357.85714, -547.36218);
    R << QPointF(375.0, -550.21932);
    R << QPointF(391.42857, -568.07647);
    R << QPointF(404.28571, -588.79075);
    R << QPointF(413.57143, -612.36218);
    R << QPointF(417.14286, -628.07647);
    R << QPointF(438.57143, -619.1479);
    R << QPointF(438.03572, -618.96932);
    R << QPointF(437.5, -609.50504);
    R << QPointF(426.96429, -609.86218);
    R << QPointF(424.64286, -615.57647);
    R << QPointF(419.82143, -615.04075);
    R << QPointF(420.35714, -605.04075);
    R << QPointF(428.39286, -598.43361);
    R << QPointF(437.85714, -599.68361);
    R << QPointF(443.57143, -613.79075);
    R << QPointF(450.71429, -610.21933);
    R << QPointF(431.42857, -575.21932);
    R << QPointF(405.71429, -550.21932);
    R << QPointF(372.85714, -534.50504);
    R << QPointF(349.28571, -531.6479);
    R << QPointF(346.42857, -521.6479);
    R << QPointF(346.42857, -511.6479);
    R << QPointF(350.71429, -496.6479);
    R << QPointF(367.85714, -476.6479);
    R << QPointF(377.14286, -460.93361);
    R << QPointF(385.71429, -445.21932);
    R << QPointF(388.57143, -404.50504);
    R << QPointF(360.0, -352.36218);
    R << QPointF(337.14286, -325.93361);
    R << QPointF(330.71429, -334.50504);
    R << QPointF(347.14286, -354.50504);
    R << QPointF(337.85714, -370.21932);
    R << QPointF(333.57143, -359.50504);
    R << QPointF(319.28571, -353.07647);
    R << QPointF(312.85714, -366.6479);
    R << QPointF(350.71429, -387.36218);
    R << QPointF(368.57143, -408.07647);
    R << QPointF(375.71429, -431.6479);
    R << QPointF(372.14286, -454.50504);
    R << QPointF(366.42857, -462.36218);
    R << QPointF(352.85714, -462.36218);
    R << QPointF(336.42857, -456.6479);
    R << QPointF(332.85714, -438.79075);
    R << QPointF(338.57143, -423.79075);
    R << QPointF(338.57143, -411.6479);
    R << QPointF(327.85714, -405.93361);
    R << QPointF(320.71429, -407.36218);
    R << QPointF(315.71429, -423.07647);
    R << QPointF(314.28571, -440.21932);
    R << QPointF(325.0, -447.71932);
    R << QPointF(324.82143, -460.93361);
    R << QPointF(317.85714, -470.57647);
    R << QPointF(304.28571, -483.79075);
    R << QPointF(287.14286, -491.29075);
    R << QPointF(263.03571, -498.61218);
    R << QPointF(251.60714, -503.07647);
    R << QPointF(251.25, -533.61218);
    R << QPointF(260.71429, -533.61218);
    R << QPointF(272.85714, -528.43361);
    R << QPointF(286.07143, -518.61218);
    R << QPointF(297.32143, -508.25504);
    R << QPointF(297.85714, -507.36218);
    R << QPointF(298.39286, -506.46932);
    R << QPointF(307.14286, -496.6479);
    R << QPointF(312.67857, -491.6479);
    R << QPointF(317.32143, -503.07647);
    R << QPointF(322.5, -514.1479);
    R << QPointF(325.53571, -521.11218);
    R << QPointF(327.14286, -525.75504);
    R << QPointF(326.96429, -535.04075);
    R << QPointF(311.78571, -540.04075);
    R << QPointF(291.07143, -552.71932);
    R << QPointF(274.82143, -568.43361);
    R << QPointF(259.10714, -592.8979);
    R << QPointF(254.28571, -604.50504);
    R << QPointF(251.07143, -621.11218);
    R << QPointF(250.53571, -649.1479);
    R << QPointF(268.1955, -654.36208);

    return QTransform().scale(width/(-325.934 - -685.219), height/(-325.934 - -685.219)).translate(-(250.536 + 450.714)/2, -(-685.219 + -325.934)/2).map(R);
  }

  QPolygonF knot(double width, double height)
  {
    QPolygonF R;

    R << QPointF(-width/2, height/2);
    R << QPointF(width/2, -height/2);
    R << QPointF(width/2, 0.0);
    R << QPointF(-width/2, 0.0);
    R << QPointF(-width/2, -height/2);
    R << QPointF(width/2, height/2);

    return R;
  }

  QPolygonF complex(double width, double height)
  {
    QPolygonF R;

    R << QPointF(-0.5*width, -0.5*height);
    R << QPointF(0.3*width, -0.5*height);
    R << QPointF(-0.0*width, 0.1*height);
    R << QPointF(-0.2*width, -0.4*height);
    R << QPointF(-0.35*width, -0.35*height);
    R << QPointF(0.5*width, 0.2*height);
    R << QPointF(-0.45*width, 0.2*height);
    R << QPointF(0.35*width, -0.35*height);
    R << QPointF(-0.1*width, 0.5*height);
    R << QPointF(0.1*width, 0.5*height);

    return R;
  }

} // namespace

#endif
