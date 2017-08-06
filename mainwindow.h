#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QProcess>
//#include <semaphore.h>

#define GC_PIPE_NAME "/tmp/uvm_gc_pipe"
#define SEM_NAME "/uvm_gc_semaphore"

namespace Ui {
class MainWindow;
}

typedef unsigned char byte;

enum GCActionType
{
    ALLOC =           0b0,
    XCHANGE_HEAPS =   0b1,
    MINOR_CLEAN =    0b10,
    WIPE_H1 =       0b100,
    WIPE_H2 =      0b1000,
    INIT =        0b10000
};

enum GCHeapType
{
    H1, H2
};

enum GCFrameType
{
    USED,
    FREE,
    MOVED,
};

struct GCPoint
{
    GCActionType action = ALLOC;
    uint start;
    int size;
    uint heapsize;
    GCFrameType frame_type = USED;
    GCHeapType heap_type = H1;

    GCPoint()
    {

    }

    GCPoint(GCActionType act, uint start, int size, GCFrameType ftype)
    {
        this->action = act;
        this->start = start;
        this->size = size;
        this->frame_type = ftype;
    }

    GCPoint(GCActionType act, uint start, int size, GCFrameType ftype, GCHeapType htype)
    {
        this->action = act;
        this->size = start;
        this->size = size;
        this->frame_type = ftype;
        this->heap_type = htype;
    }
};

struct GCStat
{
    uint heap_size;
    uint h1_used;
    uint h2_used;
    uint clean_count;
    GCHeapType phys_heap;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

    void updateStat();
    void gclog(const QString &str);
public:
    explicit MainWindow(QWidget *parent = 0);
    int getPid() { return pid; }
    ~MainWindow();

private slots:
    void on_openButton_clicked();

    void on_runButton_clicked();

    void on_nextButton_clicked();


    void on_stopButton_clicked();

    void uvmRead();
    void uvmStarted();
    void uvmFinished();

private:
    Ui::MainWindow *ui;
    QString fileName;
    QGraphicsScene *h1_graphicsScene;
    QGraphicsScene *h2_graphicsScene;
    QProcess *uvm;
    int fifo = -1;
    int pid = -1;
    int heap_size;
    GCStat gcstat;
    //sem_t* sem;
};

#endif // MAINWINDOW_H
