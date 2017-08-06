#include <QFileDialog>
#include <QFile>
#include <QThread>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <csignal>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    uvm = new QProcess;
    ui->setupUi(this);

    h1_graphicsScene = new QGraphicsScene(this);
    ui->heap1View->setScene(h1_graphicsScene);
    ui->heap1View->setAlignment(Qt::AlignTop|Qt::AlignLeft);

    h2_graphicsScene = new QGraphicsScene(this);
    ui->heap2View->setScene(h2_graphicsScene);
    ui->heap2View->setAlignment(Qt::AlignTop|Qt::AlignLeft);

    connect(uvm, SIGNAL(readyRead()), SLOT(uvmRead()));
    connect(uvm, SIGNAL(started()), SLOT(uvmStarted()));
    connect(uvm, SIGNAL(finished(int)), SLOT(uvmFinished()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openButton_clicked()
{
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    uvm->start(tr("./uvm --no-jit --gc-debug ") + fileName);

    pid = uvm->pid();
    std::cerr << pid << "\n";
    std::cerr << "sizeof(x64) = " << sizeof(GCPoint) << "\n";
    h1_graphicsScene->clear();
    h2_graphicsScene->clear();
}

void MainWindow::on_runButton_clicked()
{
    ui->stopButton->setEnabled(true);
    if(fifo == -1)
        fifo = open(GC_PIPE_NAME, O_RDONLY);
    //if((sem = sem_open(SEM_NAME, O_RDWR)) == SEM_FAILED)
    //{
    //    perror("sem_open");
    //}
}

void MainWindow::on_nextButton_clicked()
{
    if(uvm->state() == QProcess::NotRunning)
    {
        gclog("VM finished");
        ui->statLabel->setText("");
        return;
    }
    GCPoint p;

    read(fifo, &p, sizeof(p));
    kill(pid, SIGCONT);
    //int val;
    //sem_getvalue(sem, &val);
    //std::clog << "VAL: " << val;
    //sem_post(sem);
    if(p.action == ALLOC)
    {
        auto scene = p.heap_type == H1 ? h1_graphicsScene : h2_graphicsScene;

        double x = ui->heap1View->width()/(double)heap_size * p.start;


        if(p.frame_type == USED)
        {
            gclog(tr("") + QString::number(p.size) + " bytes allocated in H"
                  + (p.heap_type == H1 ? "1" : "2") + " at " + QString::number(p.start));
            //std::clog << p.size << " bytes allocated in heap(" << (p.heap_type == H1 ? 1 : 2) << ") at " << p.start << std::endl;
            scene->addRect(x, 80, ui->heap1View->width()/(double)heap_size * p.size, 80,
                           QPen(QColor::fromRgb(119, 171, 255)),
                           QBrush(QColor::fromRgb(66, 134, 244)));
            if(p.heap_type == H1)
                gcstat.h1_used += p.size;
            else
                gcstat.h2_used += p.size;
        }
        else if(p.frame_type == FREE)
        {
            gclog(tr("") + QString::number(p.size) + " bytes freed in H"
                  + (p.heap_type == H1 ? "1" : "2") + " at " + QString::number(p.start));
            //std::clog << p.size << " bytes freed in heap(" << (p.heap_type == H1 ? 1 : 2) << ") at " << p.start << std::endl;
            scene->addRect(x, 80, ui->heap1View->width()/(double)heap_size * p.size, 80,
                           QPen(QColor::fromRgb(0xFF, 0xFF, 0xFF)),
                           QBrush(QColor::fromRgb(0xFF, 0xFF, 0xFF)));
            gcstat.h1_used -= p.size;
        }
        else
        {
            gclog(tr("") + QString::number(p.size) + " bytes moved from H"
                  + (p.heap_type == H1 ? "1" : "2") + " at " + QString::number(p.start));
            //std::clog << p.size << " bytes moved from heap(" << (p.heap_type == H1 ? 1 : 2) << ") at " << p.start << std::endl;
            scene->addRect(x, 80, ui->heap1View->width()/(double)heap_size * p.size, 80,
                           QPen(QColor::fromRgb(54, 255, 40)),
                           QBrush(QColor::fromRgb(55, 221, 55)));
            //gcstat.h2_used -= p.size;
        }
    }
    else
    {
        switch (p.action) {
            case INIT:
                gclog(tr("Inited. Heap size = ") + QString::number(p.size));
                //std::clog << "Inited. Heap size = " << p.size << std::endl;
                heap_size = p.size;
                memset(&gcstat, 0, sizeof(GCStat));
                gcstat.heap_size = p.size;
                break;
            case MINOR_CLEAN:
                gclog("Clean started");
                //std::clog << "Clean started\n";
                h2_graphicsScene->clear();
                h1_graphicsScene->addRect(0, 80, ui->heap1View->width()/(double)heap_size * gcstat.h1_used, 80,
                               QPen(QColor::fromRgb(170, 170, 170, 200)),
                               QBrush(QColor::fromRgb(170, 170, 170)));
                gcstat.clean_count++;
                break;
            case WIPE_H1:
                gclog("Wiped H1");
                //std::clog << "Wiped H1\n";
                h1_graphicsScene->clear();
                gcstat.h1_used = 0;
                break;
            case WIPE_H2:
                gclog("Wiped H2");
                //std::clog << "Wiped H2\n";
                h2_graphicsScene->clear();
                gcstat.h2_used = 0;
                break;
            case WIPE_H1 | WIPE_H2:
                gclog("Wiped H1 & H2");
                //std::clog << "Wiped H1 & H2\n";
                h1_graphicsScene->clear();
                h2_graphicsScene->clear();
                gcstat.h1_used = 0;
                gcstat.h2_used = 0;
            case XCHANGE_HEAPS:
                gclog("Exchanging heaps");
                //std::clog << "Xchange heaps\n";
                ui->heap1View->setScene(h2_graphicsScene);
                ui->heap2View->setScene(h1_graphicsScene);
                h1_graphicsScene->clear();
                std::swap(h1_graphicsScene, h2_graphicsScene);
                if(gcstat.phys_heap == H1)
                    gcstat.phys_heap = H2;
                else
                    gcstat.phys_heap = H1;
                gcstat.h1_used = gcstat.h2_used;
                gcstat.h2_used = 0;
                break;
            default:
                break;
        }
    }
    updateStat();
}


void MainWindow::updateStat()
{
    ui->statLabel->setText(
                tr("Heap size: 2*") + QString::number(gcstat.heap_size) + "b\n" +
                tr("Heap(H1) used: ") + QString::number(gcstat.h1_used) + "b\n" +
                tr("Heap(H2) used: ") + QString::number(gcstat.h2_used) + "b\n" +
                tr("Physical heap: ") + (gcstat.phys_heap == H1 ? "1" : "2") + "\n" +
                tr("GC clean invoked ") + QString::number(gcstat.clean_count) + " times"
            );
}

void MainWindow::gclog(const QString &str)
{
    if(ui->logOut->document()->isEmpty())
        ui->logOut->setText(str);
    else
        ui->logOut->setText(ui->logOut->document()->toPlainText() + "\n" + str);
    ui->logOut->moveCursor(QTextCursor::End);
}

void MainWindow::on_stopButton_clicked()
{
    uvm->kill();
    ui->stopButton->setEnabled(false);
}

void MainWindow::uvmRead()
{
    if(ui->outText->document()->isEmpty())
        ui->outText->setText(uvm->readLine());
    else
        ui->outText->setText(ui->outText->document()->toPlainText() + uvm->readLine());
    ui->outText->moveCursor(QTextCursor::End);
}

void MainWindow::uvmStarted()
{

}

void MainWindow::uvmFinished()
{
    gclog("VM finished");
    ui->statLabel->setText("");
}
