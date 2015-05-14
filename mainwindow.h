#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QMessageBox>
#include "rasterhandler.h"

#define ZOOM_SCALE 1.1

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void setRasterHandlerDebugOn() {r->setDebugON();}
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    RasterHandler* r;
    QGraphicsScene original, rastered;
    QString workingFile;
    QThread* rasterThread;

private slots:
    void showAbout();
    void showOriginal();
    void showRastered();
    void setAutoRefresh(bool);

    void openFile();
    void saveFile();

    void raster();
    void rasterFinished();

    void zoomIn();
    void zoomOut();

signals:
    void statusUpdate(QString);
};

#endif // MAINWINDOW_H
