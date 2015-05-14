#include "rasterhandler.h"

RasterHandler::RasterHandler()
{
    //initialization

    _loaded = false;
    _rastered = false;
    _debug = false;

    //ANN init

    queryPt = annAllocPt(DIMENSIONS);
    dataPts = annAllocPts(MAX_COLORS, DIMENSIONS);
    nnIdx = new ANNidx[NEAREST_POINTS];
    dists = new ANNdist[NEAREST_POINTS];

    //default setting

    _window = DEFAULT_WINDOW;
    _cthres = DEFAULT_COLOR_THRESHOLD;
    _fcthres = DEFAULT_FITTING_COLOR_THRESHOLD;
    _sdiam = DEFAULT_SEARCH_DIAMETER;

}

RasterHandler::RasterHandler(int window, double cthres, int sdiam, double fcthres)
{
    setWindow(window);
    setColorThreshold(cthres);
    setSearchDiameter(sdiam);
    setFittingColorThreshold(fcthres);
}

void RasterHandler::setDebugON()
{
    _debug = true;
    p_debug.setFileName("output.debug");
    f_debug.setFileName("found.map");
}

void RasterHandler::setOriginal(QString path)
{
    _rastered = false;
    _loaded = _original.load(path);
    if (_original.width() > MAX_PIXELS || _original.height() > MAX_PIXELS)
        _loaded = false;
    if (_loaded)
        emit statusUpdate(QString("Image loaded."));
}

void RasterHandler::setWindow(int window) {_window = window;}
void RasterHandler::setColorThreshold(double cthres) {_cthres = cthres;}
void RasterHandler::setFittingColorThreshold (double fcthres) {_fcthres = fcthres;}
void RasterHandler::setSearchDiameter(int sdiam) { _sdiam = sdiam;}

const QImage &RasterHandler::getOriginal() {return _original;}
const QImage &RasterHandler::getRastered() {return _raster;}
int RasterHandler::getWindow() {return _window;}
int RasterHandler::getSearchDiameter() {return _sdiam;}
double RasterHandler::getColorThreshold() {return _cthres;}
double RasterHandler::getFittingColorThreshold() {return _fcthres;}

bool RasterHandler::isLoaded() {return _loaded;}
bool RasterHandler::isRastered() {return _rastered;}

// Rasterization invoker

void RasterHandler::raster()
{
    if (!_loaded) return;
    _raster = _original;
    found.fill('0', _original.width() * _original.height());
    if (_debug)
    {
        if (!p_debug.open(QIODevice::WriteOnly | QIODevice::Text))
            _debug = false;
        if (!f_debug.open(QIODevice::WriteOnly | QIODevice::Text))
            _debug = false;
        debug_out.setDevice(&p_debug);
        debug_out.setRealNumberNotation(QTextStream::FixedNotation);
        debug_out.setRealNumberPrecision(2);
    }

    emit processPercentage(0);
    emit statusUpdate(QString("Start rastering..."));

    emit statusUpdate(QString("Start searching shape color..."));
    getShapeColor();
    buildANNS();

    recolorization();
    _rastered = true;
    delete kdTree;

    if (_debug)
    {
        p_debug.close();
        f_debug.close();
    }
    emit processPercentage(0);
    emit statusUpdate(QString("Done."));
    emit finished();
}

// Raster related private function

void RasterHandler::getShapeColor()
{
    int width = _original.width();
    int height = _original.height();
    int i,j,x,y;
    bool* table = new bool[width*height]();
\
    _c.clear();
    for (x = 0; x < height - _window; x++) for (y = 0; y < width - _window; y++)
    {
        emit processPercentage((int)50*(double)(x*width+y+1)/(height*width));
        if (table[x*width+y]) continue;
        QColor color(_original.pixel(y, x));
        bool flag = true;
        for (i = 0; i < _window; i++) for (j = 0; j < _window; j++)
        {
            double c[3], p[3], cp[3];
            convertColorToVector(QColor(_original.pixel(y+j, x+i)), p);
            convertColorToVector(color, c);
            vectorMinus(cp, p, c);
            if (vectorDotProduct(cp) > _cthres * _cthres && flag)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            if (!_c.contains(color)) _c.append(color);
            for (i = 0; i < _window; i++) for (j = 0; j < _window; j++)
                table[(x+i)*width+y+j] = true;
        }
    }

    delete table;
}

void RasterHandler::recolorization()
{
    int width = _raster.width();
    int height = _raster.height();
    int x,y;

    // phase 1 : find all case 1 pixels
    emit statusUpdate(QString("Recolorization phase 1..."));
    for (x = 0; x < height; x++) for (y = 0; y < width; y++)
    {
        emit processPercentage((int)50+50/3*(double)(x*width+y+1)/(height*width));
        readANNpoint(queryPt, QColor(_raster.pixel(y, x)));

        kdTree->annkSearch(queryPt, NEAREST_POINTS,
                           nnIdx, dists, ERROR_BOUNDS);

        if (dists[0] < _fcthres * _fcthres)
        {
            _raster.setPixel(y, x,
                             qRgb(dataPts[nnIdx[0]][0],
                                  dataPts[nnIdx[0]][1],
                                  dataPts[nnIdx[0]][2]));
            found[x*width+y] = '1';
        }
    }

    // phase 2: find all case 2 pixels
    emit statusUpdate(QString("Recolorization phase 2..."));
    for (x = 0; x < height; x++) for (y = 0; y < width; y++) if (found.at(x*width+y) == '0')
    {
        emit processPercentage((int)50+50/3+1+50/3*(double)(x*width+y+1)/(height*width));
        QColor target = search2(QPoint(x, y));
        if (target.isValid())
        {
            _raster.setPixel(y, x, target.rgb());
            found[x*width+y] = '2';
        }
    }

    // phase 3: find all case 3 pixels
    emit statusUpdate(QString("Recolorization phase 3..."));
    for (x = 0; x < height; x++) for (y = 0; y < width; y++) if (found.at(x*width+y) == '0')
    {
        emit processPercentage((int)50+50/3*2+2+50/3*(double)(x*width+y+1)/(height*width));
        QColor target = search3(QPoint(x, y));
        if (target.isValid())
        {
            _raster.setPixel(y, x, target.rgb());
            found[x*width+y] = '3';
        }
    }

    // debug related
    if (_debug)
    {
        debug_out.setDevice(&f_debug);
        for (x = 0; x < height; x++)
        {
            for (y = 0; y < width; y++)
                debug_out << found.at(x*width+y) << " ";
            debug_out << endl;
        }
    }
}

// Re-rasterization related private function

QColor RasterHandler::search2(QPoint p)
{
    // search, clean up
    QList<QColor>* clist = search(p, 2);
    if (clist->length() < 2) return QColor(_raster.pixel(p.y(), p.x()));
    QColor t[3] = {clist->at(0), clist->at(1), QColor(_raster.pixel(p.y(), p.x()))};
    delete clist;

    // calc
    double cp[3], ca[3], cb[3];
    convertColorToVector(t[2], cp);
    convertColorToVector(t[0], ca);
    convertColorToVector(t[1], cb);

    double ap[3], ab[3], sp[3];
    vectorMinus(ap, cp, ca);
    vectorMinus(ab, cb ,ca);
    double wa = 1 - vectorDotProduct(ap, ab) / vectorDotProduct(ab);

    // fitting error
    double cs[3]={ca[0] * wa + cb[0] *  (1-wa),
                  ca[1] * wa + cb[1] *  (1-wa),
                  ca[2] * wa + cb[2] *  (1-wa)};
    vectorMinus(sp, cp, cs);
    double error = vectorDotProduct(sp);

    if (error < _fcthres * _fcthres)
        if (wa > 1 - wa) return t[0]; else return t[1];
    else
        return QColor();
}

QColor RasterHandler::search3(QPoint p)
{
    // search, clean up
    QList<QColor>* clist = search(p, 3);
    if (clist->length() < 3) return QColor(_raster.pixel(p.y(), p.x()));
    QColor t[4] = {clist->at(0), clist->at(1), clist->at(2), QColor(_raster.pixel(p.y(), p.x()))};
    delete clist;

    // calc
    double cp[3], ca[3], cb[3], cc[3];
    convertColorToVector(t[3], cp);
    convertColorToVector(t[0], ca);
    convertColorToVector(t[1], cb);
    convertColorToVector(t[2], cc);

    double c[2][3];
    double ac[3], bc[3], pc[3];
    vectorMinus(ac, ca ,cc);
    vectorMinus(bc, cb, cc);
    vectorMinus(pc, cc, cp);
    c[0][0] = vectorDotProduct(ac);
    c[0][1] = vectorDotProduct(ac, bc);
    c[0][2] = vectorDotProduct(pc, ac);
    c[1][0] = c[0][1];
    c[1][1] = vectorDotProduct(bc);
    c[1][2] = vectorDotProduct(pc, bc);

    if (c[0][1]*c[1][0] - c[0][0]*c[1][1] == 0) return t[3];
    double w1 = (c[1][1]*c[0][2] - c[1][2]*c[0][1]) / (c[0][1]*c[1][0] - c[0][0]*c[1][1]);
    double w2 = (c[1][2]*c[0][0] - c[0][2]*c[1][0]) / (c[0][1]*c[1][0] - c[0][0]*c[1][1]);
    double w3 = 1 - w1 - w2;
    if (w1 < 0 || w2 < 0 || w3 < 0) return t[3];

    double cs[3] = {t[0].red()  *w1 +   t[1].red()      *   w2  +   t[2].red()  *   w3,
                    t[0].green()*w1 +   t[1].green()    *   w2  +   t[2].green()*   w3,
                    t[0].blue() *w1 +   t[1].blue()     *   w2  +   t[2].blue() *   w3};
    double sp[3];
    vectorMinus(sp, cp, cs);
    double error = vectorDotProduct(sp);

    if (error < _fcthres * _fcthres)
    {
        if (w1 > w2 && w1 > w3) return t[0];
        if (w2 > w1 && w2 > w3) return t[1];
        if (w3 > w1 && w3 > w1) return t[2];
        return t[0];
    }
    else
    {
        // for debugging
        if (_debug)
        {
            debug_out << "(" << t[3].red() << ' ' << t[3].green() << ' ' << t[3]. blue() <<
                         ") (" << p.y() << ", " << p.x() << ")" << endl;
            debug_out << "(" << t[0].red() << ' ' << t[0].green() << ' ' << t[0].blue() << ") " << w1 << endl;
            debug_out << "(" << t[1].red() << ' ' << t[1].green() << ' ' << t[1].blue() << ") " << w2 << endl;
            debug_out << "(" << t[2].red() << ' ' << t[2].green() << ' ' << t[2].blue() << ") " << w3 << endl;
            debug_out << "(" << cs[0] << ' ' << cs[1] << ' ' << cs[2] << ") " << error << endl;
            debug_out << endl;
        }
        return QColor();
    }
}

bool RasterHandler::posJudge(QPoint p)
{
    return (p.x() > 0 && p.x() < _raster.height() && p.y() > 0 && p.y() < _raster.width());
}

QList<QColor>* RasterHandler::search(QPoint p, int num)
{
    const int dir[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};
    int i, o, j, k = 0;
    QPoint c = p;
    QList<QColor>* clist = new QList<QColor>;

    for (i = 1; i <= _sdiam && num ; i++)
        for (o = 0; o < 2 && num; o++)
        {
            for (j = 1; j <= i && num; j++)
            {
                c.rx() += dir[k%4][0];
                c.ry() += dir[k%4][1];
                if (posJudge(c) && found.at(c.x()*_raster.width()+c.y()) == '1' &&
                        !clist->contains(_raster.pixel(c.y(), c.x())))
                {
                    clist->append(_raster.pixel(c.y(), c.x()));
                    num--;
                }
            }
            k++;
        }
    return clist;
}

void RasterHandler::convertColorToVector(QColor p, double *c)
{
    c[0] = p.red();
    c[1] = p.green();
    c[2] = p.blue();
}

// ANN related private functions

void RasterHandler::readANNpoint(ANNpoint p, QColor c)
{
    p[0] = c.red();
    p[1] = c.green();
    p[2] = c.blue();
}

void RasterHandler::buildANNS()
{
    for (int i = 0; i < _c.length(); i++) readANNpoint(dataPts[i], _c[i]);

    kdTree = new ANNkd_tree(dataPts, _c.length(), DIMENSIONS);
}

// Calculation related private function;

void RasterHandler::vectorMinus(double *_dest, double* a, double* b)
{
    _dest[0] = a[0] - b[0];
    _dest[1] = a[1] - b[1];
    _dest[2] = a[2] - b[2];
}

double RasterHandler::vectorDotProduct(double* a) {return vectorDotProduct(a, a);}
double RasterHandler::vectorDotProduct(double* a, double* b) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

// clean things up

RasterHandler::~RasterHandler()
{
    delete []nnIdx;
    delete []dists;
    annClose();
}
