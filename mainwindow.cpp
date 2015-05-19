#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "about.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    r = new RasterHandler();
    rasterThread = new QThread(this);
    connect (rasterThread, SIGNAL(started()), r, SLOT(raster()));
    connect (r, SIGNAL(finished()), this, SLOT(rasterFinished()));
    r->moveToThread(rasterThread);

    ui->action_Save->setDisabled(true);
    ui->windowSize->setValue(r->getWindow());
    ui->searchDiameter->setValue(r->getSearchDiameter());
    ui->fittingColorThreshold->setValue(r->getFittingColorThreshold());
    ui->colorThreshold->setValue(r->getColorThreshold());
    ui->autoRefresh->setChecked(false);
    ui->original->setDisabled(true);
    ui->apply->setDisabled(true);
    ui->progressBar->setValue(0);
    ui->graphicsView->setScene(&original);
    ui->graphicsView->show();

    connect (ui->actionAbout_This, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect (ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect (ui->action_Open, SIGNAL(triggered()), this, SLOT(openFile()));
    connect (ui->action_Save, SIGNAL(triggered()), this, SLOT(saveFile()));
    connect (ui->action_Exit, SIGNAL(triggered()), this, SLOT(close()));
    connect (ui->apply, SIGNAL(clicked()), this, SLOT(raster()));
    connect (ui->zoomOut, SIGNAL(clicked()), this, SLOT(zoomOut()));
    connect (ui->zoomIn, SIGNAL(clicked()), this, SLOT(zoomIn()));

    connect (ui->autoRefresh, SIGNAL(clicked(bool)), this, SLOT(setAutoRefresh(bool)));
    connect (ui->original, SIGNAL(pressed()), this, SLOT(showOriginal()));
    connect (ui->original, SIGNAL(released()), this, SLOT(showRastered()));

    connect (r, SIGNAL(processPercentage(int)), ui->progressBar, SLOT(setValue(int)));
    connect (r, SIGNAL(statusUpdate(QString)), ui->statusBar, SLOT(showMessage(QString)));
    connect (this, SIGNAL(statusUpdate(QString)), ui->statusBar, SLOT(showMessage(QString)));

}

void MainWindow::showAbout()
{
    about* a = new about(this);
    a->show();
}


void MainWindow::openFile()
{
    workingFile = QFileDialog::getOpenFileName(this, QString("Open"), workingFile,
                                                 QString("Images (*.png *.bmp *.jpg)"));
    if (workingFile.isEmpty()) return;

    QFile file(workingFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, QString("Error"), QString("Can't read this file!"));
        return;
    }
    file.close();

    r->setOriginal(workingFile);
    if (!r->isLoaded())
    {
        QMessageBox::critical(this, QString("Error"), QString("Can't process this image!"));
        return;
    }

    original.clear();
    original.setSceneRect(0, 0, r->getOriginal().width(), r->getOriginal().height());
    original.addPixmap(QPixmap::fromImage(r->getOriginal()));
    rastered.clear();

    if (ui->autoRefresh->isChecked())
        raster();
    else showOriginal();

    qreal _scale = ui->graphicsView->transform().m11();
    ui->graphicsView->scale(1 / _scale, 1 / _scale);
    ui->action_Save->setEnabled(true);
    ui->apply->setEnabled(true);
    ui->original->setDisabled(true);
}

void MainWindow::saveFile()
{
    if (!r->isRastered())
    {
        emit statusUpdate(QString("Please process this image first."));
        return;
    }

    QString savepath = QFileDialog::getSaveFileName(this, QString("Save"), workingFile,
                                                    QString("Images (*.png *.bmp *.jpg)"));
    if (savepath.isEmpty()) return;

    if (!r->getRastered().save(savepath))
    {
        QMessageBox::critical(this, QString("Error"), QString("Can't save to this file!"));
        return;
    }
    else
        emit statusUpdate(QString("Saved."));
}

void MainWindow::raster()
{
    ui->action_Open->setDisabled(true);
    ui->action_Save->setDisabled(true);
    ui->colorThreshold->setDisabled(true);
    ui->fittingColorThreshold->setDisabled(true);
    ui->windowSize->setDisabled(true);
    ui->searchDiameter->setDisabled(true);
    ui->apply->setDisabled(true);

    r->setWindow(ui->windowSize->value());
    r->setSearchDiameter(ui->searchDiameter->value());
    r->setColorThreshold(ui->colorThreshold->value());
    r->setFittingColorThreshold(ui->fittingColorThreshold->value());
    rasterThread->start();
}

void MainWindow::rasterFinished()
{
    rasterThread->quit();

    ui->action_Open->setEnabled(true);
    ui->action_Save->setEnabled(true);
    ui->colorThreshold->setEnabled(true);
    ui->fittingColorThreshold->setEnabled(true);
    ui->windowSize->setEnabled(true);
    ui->searchDiameter->setEnabled(true);
    ui->apply->setEnabled(true);
    ui->original->setEnabled(true);

    if (!r->isRastered()) return;
    rastered.clear();
    rastered.setSceneRect(0, 0, r->getRastered().width(), r->getRastered().height());
    rastered.addPixmap(QPixmap::fromImage(r->getRastered()));

    showRastered();
}

void MainWindow::showOriginal()
{
    ui->graphicsView->setScene(&original);
    ui->graphicsView->show();
}

void MainWindow::showRastered()
{
    ui->graphicsView->setScene(&rastered);
    ui->graphicsView->show();
}

void MainWindow::setAutoRefresh(bool on)
{
    if (on) {
        connect (ui->windowSize, SIGNAL(valueChanged(int)), this, SLOT(raster()));
        connect (ui->searchDiameter, SIGNAL(valueChanged(int)), this, SLOT(raster()));
        connect (ui->colorThreshold, SIGNAL(valueChanged(double)), this, SLOT(raster()));
        connect (ui->fittingColorThreshold, SIGNAL(valueChanged(double)), this, SLOT(raster()));
    }
    else {
        disconnect (ui->windowSize, SIGNAL(valueChanged(int)), this, SLOT(raster()));
        disconnect (ui->searchDiameter, SIGNAL(valueChanged(int)), this, SLOT(raster()));
        disconnect (ui->colorThreshold, SIGNAL(valueChanged(double)), this, SLOT(raster()));
        disconnect (ui->fittingColorThreshold, SIGNAL(valueChanged(double)), this, SLOT(raster()));
    }
}

void MainWindow::zoomIn()
{
    ui->graphicsView->scale(ZOOM_SCALE, ZOOM_SCALE);
}

void MainWindow::zoomOut()
{
    ui->graphicsView->scale(1 / ZOOM_SCALE, 1 / ZOOM_SCALE);
}

MainWindow::~MainWindow()
{
    delete ui;
}
