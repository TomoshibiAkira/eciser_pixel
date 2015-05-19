#ifndef RASTERHANDLER_H
#define RASTERHANDLER_H

#define DEFAULT_WINDOW 3
#define DEFAULT_COLOR_THRESHOLD 1
#define DEFAULT_FITTING_COLOR_THRESHOLD 3
#define DEFAULT_SEARCH_DIAMETER 7
#define DIMENSIONS 3
#define MAX_COLORS 5000000
#define MAX_PIXELS 5000
#define NEAREST_POINTS 1
#define ERROR_BOUNDS 0

#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QFile>
#include <QTextStream>
#include <ANN/ANN.h>

class RasterHandler : public QObject
{
    Q_OBJECT

public:
    RasterHandler();
    RasterHandler(int, double , int, double fcthres);
    ~RasterHandler();
    void setOriginal(QString);
    void setWindow(int);
    void setColorThreshold(double);
    void setFittingColorThreshold(double);
    void setDebugON();
    void setSearchDiameter(int);
    const QImage &getOriginal();
    const QImage &getRastered();
    int getWindow();
    int getSearchDiameter();
    double getColorThreshold();
    double getFittingColorThreshold();
    bool isLoaded();
    bool isRastered();

private:

    // private parameters & flags
    QImage _original, _raster;
    bool _rastered, _loaded, _debug;
    QByteArray found;
    int _window, _sdiam;
    double _cthres, _fcthres;
    QList<QColor> _c;

    // ANN related
    void buildANNS();
    void readANNpoint(ANNpoint, QColor);
    ANNpointArray dataPts;
    ANNpoint queryPt;
    ANNidxArray nnIdx;
    ANNdistArray dists;
    ANNkd_tree* kdTree;

    // debugging files
    QFile p_debug, f_debug;
    QTextStream debug_out;

    // rasterization related
    QList<QColor> *search(QPoint, int);
    QColor search2(QPoint);
    QColor search3(QPoint);
    bool posJudge(QPoint);

    // calculation related
    void convertColorToVector(QColor, double*);
    void vectorMinus(double*, double*, double*);
    double vectorDotProduct(double*);
    double vectorDotProduct(double*, double*);

    // main step
    void getShapeColor();
    void recolorization();

signals:
    void processPercentage(int);
    void statusUpdate(QString);
    void finished();

public slots:
    void raster();
};

#endif // RASTERHANDLER_H
