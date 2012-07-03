#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QTableWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardItemModel>
#include <QMessageBox>
#include "pgmimage.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(PgmImage *pgmImage, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    PgmImage *pgmImage;

    // kernel things
    int** kernel;
    int kSize; ///< size of kernel
    bool rotateKernel;
    int generateKernel(); ///< ask user for type of kernel
    int kernelFreiChen(); ///< user chose "Frei & Chen" kernel
    int kernelGauss(); ///< user chose "Gauss" kernel
    int kernelKirsch(); ///< user chose "Kirsch" kernel
    int kernelLaplace(); ///< user chose "Laplace" kernel
    int kernelPrewitt1(); ///< user chose "Prewitt 1" kernel
    int kernelPrewitt2(); ///< user chose "Prewitt 2" kernel
    int kernelSobel(); ///< user chose "Sobel" kernel
    int kernelOther(); ///< user chose "other" kernel
    int sizeOfKernel(); ///< ask user for size of kernel
    int contentOfKernel(); ///< ask user for content of kernel
    void mallocKernel(); ///< allocate memory for kernel

private slots:
    void load(); ///< load a pgm image and show it
    void histogram(); ///< create a histogram of the pgm image and show it
    void invert(); ///< invert the pgm image and show it
    void convolution(); ///< convolution between the image and a matrix
    void hough(); ///< Hough transformation
    void laneDetection(); ///< Lane detection part 1
    void laneDetection2(); ///< Lane detection part 2
    void laneDetection3(); ///< Lane detection part 3
    void railDetection(); ///< rail detection
    void save(); ///< save the pgm image
};

#endif // MAINWINDOW_H
