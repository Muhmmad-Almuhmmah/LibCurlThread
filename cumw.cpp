#include "cumw.h"
#include "ui_cumw.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    cumw w;
    w.show();
    return a.exec();
}

cumw::cumw(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::cumw)
{
    ui->setupUi(this);
}

cumw::~cumw()
{
    delete ui;
}

void cumw::on_pushButton_clicked()
{
    QString url=ui->lineEdit->text(),file;
    if(url.isEmpty() or !url.startsWith("http"))
    {
        QMessageBox::warning(this,"Error","please Input valid url first!");
        return;
    }
    file=QFileDialog::getSaveFileName(this,"select Save File");
    if(file.isEmpty()){
        QMessageBox::warning(this,"Error","please select save file!");
        return;
    }
    QElapsedTimer timer;
    timer.start();
    curlThread clt;
    // init header options
    clt.setHeaders(ListKeys()
                   <<JsKeys("Type","Test")
                   <<JsKeys("Access","Simple"));

    // init body options
    clt.setOptions(ListKeys()
                   <<JsKeys("Name","Test Opt"));

    connect(&clt,SIGNAL(Update(int)),ui->progressBar,SLOT(setValue(int)));
    connect(ui->pushButton_2,SIGNAL(pressed()),&clt,SLOT(emitCancel()));
    bool state=clt.DownloadFile(url,file);
    qDebug() <<"state"<<state<<clt.timeConversion(timer.elapsed());
}
