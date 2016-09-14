#ifndef BAIDUPROJECTION_H
#define BAIDUPROJECTION_H

#include "mercatorprojection.h"
#include <qmath.h>
#include <QObject>

namespace projections {

class BaiduProjection: public MercatorProjection, public QObject
{
public:
    BaiduProjection();
    virtual QString Type(){return "BaiduMercatorProjection";}
/*    virtual Size TileSize() const;
    virtual double Axis() const;
    virtual double Flattening()const;
    virtual internals::PointLatLng FromPixelToLatLng(const int &x,const int &y,const int &zoom);
    virtual  Size GetTileMatrixMinXY(const int &zoom);
    virtual  Size GetTileMatrixMaxXY(const int &zoom);*/
    virtual core::Point FromLatLngToPixel(double lat, double lng, int const& zoom);
    void FromGPSLatLngToBaiduLatLng(const internals::PointLatLng&, internals::PointLatLng&, const int zoom);
    void FromBaiduLatLngToPixel(const internals::PointLatLng&, Point&, const int zoom);
private:
    void FromPixelToBaiduLatLng(const Point&, internals::PointLatLng&, const int zoom);
    void RequestConvertService(const int&, const internals::PointLatLng&, internals::PointLatLng&, const int zoom);
    inline int Random(int low, int high) { return low + qrand() % (high - low); }
};

}

#endif // BAIDUPROJECTION_H
