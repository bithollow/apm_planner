#include "baiduprojection.h"
#include <qeventloop.h>
#include <qtimer.h>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkAccessManager>
#include <QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

namespace projections {

namespace {
    static const int CONVERT_TYPE_WGS84_TO_BD09LL  = 1;
    static const int CONVERT_TYPE_BD09LL_TO_WGS84  = 2;
    static const int CONVERT_TYPE_GCJ02_TO_BD09LL  = 3;
    static const int CONVERT_TYPE_BD09LL_TO_GCJ02  = 4;
    static const int CONVERT_TYPE_BD09LL_TO_BD09MC = 11;
    static const int CONVERT_TYPE_BD09MC_TO_BD09LL = 12;
}
BaiduProjection::BaiduProjection()
{

}

Point BaiduProjection::FromLatLngToPixel(double lat, double lng, const int &zoom)
{
    Point ret;// = Point.Empty;

    internals::PointLatLng src(lat, lng);
    internals::PointLatLng dst;
    FromGPSLatLngToBaiduLatLng(src, dst, zoom);
    src = dst;
    FromBaiduLatLngToPixel(src, ret, zoom);

    return ret;
}

/*
internals::PointLatLng BaiduProjection::FromPixelToLatLng(const int &x, const int &y, const int &zoom)
{
    internals::PointLatLng ret;// = internals::PointLatLng.Empty;

    Size s = GetTileMatrixSizePixel(zoom);
    double mapSizeX = s.Width();
    double mapSizeY = s.Height();

    double xx = (Clip(x, 0, mapSizeX - 1) / mapSizeX) - 0.5;
    double yy = 0.5 - (Clip(y, 0, mapSizeY - 1) / mapSizeY);

    ret.SetLat(90 - 360 * atan(exp(-yy * 2 * M_PI)) / M_PI);
    ret.SetLng(360 * xx);

    return ret;
}
*/
void BaiduProjection::FromGPSLatLngToBaiduLatLng(const internals::PointLatLng& src, internals::PointLatLng& dst, const int zoom)
{
    BaiduProjection::RequestConvertService(CONVERT_TYPE_WGS84_TO_BD09LL, src, dst, zoom);
}

void BaiduProjection::FromBaiduLatLngToPixel(const internals::PointLatLng& src, Point& dst, const int zoom)
{
    internals::PointLatLng mc;
    Point ret;

    BaiduProjection::RequestConvertService(CONVERT_TYPE_BD09LL_TO_BD09MC, src, mc, zoom);

    ret.SetX(qFloor(mc.Lng() * qPow(2, zoom - 18)));
    ret.SetY(qFloor(mc.Lat() * qPow(2, zoom - 18)));
    //ret.SetX(qFloor(ret.X() / 256));
    //ret.SetY(qFloor(ret.Y() / 256));
    ret.Empty = false;

    dst = ret;
}

void BaiduProjection::FromPixelToBaiduLatLng(const Point& src, internals::PointLatLng& dst, const int zoom)
{
   // BaiduProjection::RequestConvertService(CONVERT_TYPE_BD09MC_TO_BD09LL, src, dst, zoom);
}

void BaiduProjection::RequestConvertService(const int& type, const internals::PointLatLng& src, internals::PointLatLng& dst, const int zoom)
{
    QByteArray result;
    QNetworkReply *reply;
    QNetworkRequest qheader;
    QNetworkAccessManager network;
    QEventLoop q;
    QTimer tT;

    QByteArray UserAgent = QString("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:%1.0) Gecko/%2%3%4 Firefox/%5.0.%6").arg(QString::number(Random(3,14)), QString::number(Random(QDate().currentDate().year() - 4, QDate().currentDate().year())), QString::number(Random(11,12)), QString::number(Random(10,30)), QString::number(Random(3,14)), QString::number(Random(1,10))).toLatin1();
    QString url = QString("http://api.map.baidu.com/geoconv/v1/?ak=68wKAeXgOwmGsHVOwIVvQYeQnYlsy8Xd&coords=%1,%2&from=%3&to=%4&output=json");
    tT.setSingleShot(true);
    connect(&network, SIGNAL(finished(QNetworkReply*)), &q, SLOT(quit()));
    connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()));

    qheader.setRawHeader("User-Agent",UserAgent);
    qheader.setRawHeader("Accept","*/*");

    switch(type){
    case CONVERT_TYPE_WGS84_TO_BD09LL:
       qheader.setUrl(url.arg(src.Lng()).arg(src.Lat()).arg(1).arg(5));
       qDebug()<< QString("BaiduMap convert type WGS84 => BD09LL!");
       break;
    case CONVERT_TYPE_BD09LL_TO_BD09MC:
       qheader.setUrl(url.arg(src.Lng()).arg(src.Lat()).arg(5).arg(6));
       qDebug()<< QString("BaiduMap convert type BD09LL => BD09MC!");
       break;
    case CONVERT_TYPE_BD09MC_TO_BD09LL:
       qheader.setUrl(url.arg(src.Lng()).arg(src.Lat()).arg(6).arg(5));
       qDebug()<< QString("BaiduMap convert type BD09MC => BD09LL!");
        break;
    case CONVERT_TYPE_GCJ02_TO_BD09LL:
       qheader.setUrl(url.arg(src.Lng()).arg(src.Lat()).arg(3).arg(5));
       qDebug()<< QString("BaiduMap convert type GCJ02 => BD09LL!");
        break;
    case CONVERT_TYPE_BD09LL_TO_WGS84:
    case CONVERT_TYPE_BD09LL_TO_GCJ02:
    default:
        dst = src;
        qDebug()<< QString("BaiduMap convert type error! (%1, %2) => (%3, %4)").arg(src.Lng()).arg(src.Lat()).arg(dst.Lng()).arg(dst.Lat());
        return;
    }

    QNetworkProxyFactory::setUseSystemConfiguration(true);
    reply=network.get(qheader);
    tT.start(5000);
    q.exec();

    if(!tT.isActive()){
        dst = src;
        qDebug()<< QString("BaiduMap convert request is not active! (%1, %2) => (%3, %4)").arg(src.Lng()).arg(src.Lat()).arg(dst.Lng()).arg(dst.Lat());
        return;
    }
    tT.stop();

    if((reply->error()!=QNetworkReply::NoError)) {
        dst = src;
        qDebug()<< QString("BaiduMap convert response error! (%1, %2) => (%3, %4)").arg(src.Lng()).arg(src.Lat()).arg(dst.Lng()).arg(dst.Lat());
        return;
    }
    result=reply->readAll();
    reply->deleteLater();
    QJsonDocument doc = QJsonDocument::fromJson(result);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        int status = obj["status"].toInt();
        if (status == 0) {
            QJsonArray  array = obj["result"].toArray();
            QJsonObject pst = array[0].toObject();
            dst.SetLng(pst["x"].toDouble());
            dst.SetLat(pst["y"].toDouble());
            qDebug()<< QString("BaiduMap convert(Lng, Lat) OK! (%1, %2) => (%3, %4)").arg(src.Lng()).arg(src.Lat()).arg(dst.Lng()).arg(dst.Lat());
        } else {
            dst = src;
            qDebug()<< QString("BaiduMap convert status error! (%1, %2) => (%3, %4)").arg(src.Lng()).arg(src.Lat()).arg(dst.Lng()).arg(dst.Lat());
        }
    }

    return;
}

}
