/* Copyright (C) 2017 MariaDB Corporaton

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

#include <sstream>
#include <cstring>
#include <typeinfo>
#include "regr_sxy.h"
#include "bytestream.h"
#include "objectreader.h"

using namespace mcsv1sdk;

class Add_regr_sxy_ToUDAFMap
{
public:
    Add_regr_sxy_ToUDAFMap()
    {
        UDAFMap::getMap()["regr_sxy"] = new regr_sxy();
    }
};

static Add_regr_sxy_ToUDAFMap addToMap;

// Use the simple data model
struct regr_sxy_data
{
    uint64_t	cnt;
    double      sumx;
    double      sumy;
    double      sumxy;  // sum of x * y
};


mcsv1_UDAF::ReturnCode regr_sxy::init(mcsv1Context* context,
                                       ColumnDatum* colTypes)
{
    if (context->getParameterCount() != 2)
    {
        // The error message will be prepended with
        // "The storage engine for the table doesn't support "
        context->setErrorMessage("regr_sxy() with other than 2 arguments");
        return mcsv1_UDAF::ERROR;
    }

    context->setUserDataSize(sizeof(regr_sxy_data));
    context->setResultType(CalpontSystemCatalog::DOUBLE);
    context->setColWidth(8);
    context->setScale(DECIMAL_NOT_SPECIFIED);
    context->setPrecision(0);
    context->setRunFlag(mcsv1sdk::UDAF_IGNORE_NULLS);
    return mcsv1_UDAF::SUCCESS;

}

mcsv1_UDAF::ReturnCode regr_sxy::reset(mcsv1Context* context)
{
    struct  regr_sxy_data* data = (struct regr_sxy_data*)context->getUserData()->data;
    data->cnt = 0;
    data->sumx = 0.0;
    data->sumy = 0.0;
    data->sumxy = 0.0;
    return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode regr_sxy::nextValue(mcsv1Context* context, ColumnDatum* valsIn)
{
    static_any::any& valIn_y = valsIn[0].columnData;
    static_any::any& valIn_x = valsIn[1].columnData;
    struct  regr_sxy_data* data = (struct regr_sxy_data*)context->getUserData()->data;
    double valx = 0.0;
    double valy = 0.0;

    valx = convertAnyTo<double>(valIn_x);
    valy = convertAnyTo<double>(valIn_y);

    // For decimal types, we need to move the decimal point.
    uint32_t scaley = valsIn[0].scale;

    if (valy != 0 && scaley > 0)
    {
        valy /= pow(10.0, (double)scaley);
    }

    data->sumy += valy;

    // For decimal types, we need to move the decimal point.
    uint32_t scalex = valsIn[1].scale;

    if (valx != 0 && scalex > 0)
    {
        valx /= pow(10.0, (double)scalex);
    }

    data->sumx += valx;

    data->sumxy += valx*valy;

    ++data->cnt;
    
    return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode regr_sxy::subEvaluate(mcsv1Context* context, const UserData* userDataIn)
{
    if (!userDataIn)
    {
        return mcsv1_UDAF::SUCCESS;
    }

    struct regr_sxy_data* outData = (struct regr_sxy_data*)context->getUserData()->data;
    struct regr_sxy_data* inData = (struct regr_sxy_data*)userDataIn->data;

    outData->sumx += inData->sumx;
    outData->sumy += inData->sumy;
    outData->sumxy += inData->sumxy;
    outData->cnt += inData->cnt;

    return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode regr_sxy::evaluate(mcsv1Context* context, static_any::any& valOut)
{
    struct regr_sxy_data* data = (struct regr_sxy_data*)context->getUserData()->data;
    double N = data->cnt;
    if (N > 0)
    {
        double sumx = data->sumx;
        double sumy = data->sumy;
        double sumxy = data->sumxy;

        double covar_pop = (sumxy - ((sumx * sumy) / N)) / N;
        double regr_sxy = data->cnt * covar_pop;
        valOut = regr_sxy;
    }
    return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode regr_sxy::dropValue(mcsv1Context* context, ColumnDatum* valsDropped)
{
    static_any::any& valIn_y = valsDropped[0].columnData;
    static_any::any& valIn_x = valsDropped[1].columnData;
    struct regr_sxy_data* data = (struct regr_sxy_data*)context->getUserData()->data;

    double valx = 0.0;
    double valy = 0.0;

    valx = convertAnyTo<double>(valIn_x);
    valy = convertAnyTo<double>(valIn_y);

    // For decimal types, we need to move the decimal point.
    uint32_t scaley = valsDropped[0].scale;

    if (valy != 0 && scaley > 0)
    {
        valy /= pow(10.0, (double)scaley);
    }

    data->sumy -= valy;

    // For decimal types, we need to move the decimal point.
    uint32_t scalex = valsDropped[1].scale;

    if (valx != 0 && scalex > 0)
    {
        valx /= pow(10.0, (double)scalex);
    }

    data->sumx -= valx;

    data->sumxy -= valx*valy;
    --data->cnt;

    return mcsv1_UDAF::SUCCESS;
}

