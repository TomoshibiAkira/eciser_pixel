#include "about.h"
#include "ui_about.h"


about::about(QWidget *parent) :
    QDialog(parent, Qt::WindowTitleHint),
    ui(new Ui::about)
{
    ui->setupUi(this);

    connect (ui->url, SIGNAL(clicked()), this, SLOT(url()));
    connect (ui->close, SIGNAL(clicked()), this, SLOT(close()));
}

void about::url()
{
    QDesktopServices::openUrl(QUrl("http://plainstar.tk"));
}

about::~about()
{
    delete ui;
}
