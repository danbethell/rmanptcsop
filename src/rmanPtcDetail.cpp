/*
 * rmanPtcDetail.cpp
 *
 *  Created on: 18/01/2010
 *      Author: dan
 */

#include "rmanPtcDetail.h"
using namespace rmanPtcSop;

rmanPtcDetail::rmanPtcDetail() :
    redraw(false),
    nPoints(0),
    nChannels(0),
    datasize(0),
    display_probability(1.f),
    use_cull_bbox(false)
{
}

rmanPtcDetail::~rmanPtcDetail()
{
}

