#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(PgmImage *pgmImage, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {

    // init gui
    ui->setupUi(this);
    setWindowTitle("Digitale Bildverarbeitung - Tobias Dreher");

    // pointer to image class
    this->pgmImage = pgmImage;

    // connect select-buttons with methods
    connect(ui->btnLoad,SIGNAL(clicked()),this,SLOT(load()));
    connect(ui->btnHistogram,SIGNAL(clicked()),this,SLOT(histogram()));
    connect(ui->btnInvert,SIGNAL(clicked()),this,SLOT(invert()));
    connect(ui->btnConvolution,SIGNAL(clicked()),this,SLOT(convolution()));
    connect(ui->btnHough,SIGNAL(clicked()),this,SLOT(hough()));
    connect(ui->btnSave,SIGNAL(clicked()),this,SLOT(save()));
    connect(ui->btnLaneDec,SIGNAL(clicked()),this,SLOT(laneDetection()));
    connect(ui->btnLaneDec2,SIGNAL(clicked()),this,SLOT(laneDetection2()));
    connect(ui->btnLaneDec3,SIGNAL(clicked()),this,SLOT(laneDetection3()));
    connect(ui->btnRailDec,SIGNAL(clicked()),this,SLOT(railDetection()));

    // init other things
    rotateKernel = false;
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::load() {
    statusBar()->showMessage("load image");

    // get path of image
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"),
						    QDir::homePath(),
						    tr("Portable Graymap (*.pgm)"));

    // load image with standard path
    int ret = pgmImage->loadPgm(fileName);
    if(ret == 0) {
	statusBar()->showMessage("image loaded successfully",3000);
    } else {
	switch(ret) {
	case -1:
	    statusBar()->showMessage("no such file");
	    break;
	case -2:
	    statusBar()->showMessage("no pgm file-format");
	    break;
	case -3:
	    statusBar()->showMessage("cannot handle this pgm file");
	    break;
	case -4:
	    statusBar()->showMessage("error while writing temporary file");
	    break;
	default:
	    statusBar()->showMessage("unkown error while loading image");
	}
	return;
    }

    // show image
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);

    // enable the other buttons
    ui->btnHistogram->setEnabled(true);
    ui->btnInvert->setEnabled(true);
    ui->btnSave->setEnabled(true);
    ui->btnConvolution->setEnabled(true);
    ui->btnHough->setEnabled(true);
    ui->btnLaneDec->setEnabled(true);
    ui->btnLaneDec2->setEnabled(true);
    ui->btnLaneDec3->setEnabled(true);
    ui->btnRailDec->setEnabled(true);
}

void MainWindow::histogram() {
    statusBar()->showMessage("create histogram");

    // create histogram
    if(pgmImage->histogram() != 0) {
	statusBar()->showMessage("error while creating histogram");
	return;
    }

    // show chart
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);

    statusBar()->showMessage("histogram created successfully",3000);
}

void MainWindow::invert() {
    statusBar()->showMessage("invert image");

    // invert image
    if(pgmImage->invert() != 0) {
	statusBar()->showMessage("error while writing temporary file");
    } else {
	statusBar()->showMessage("image inverted successfully",3000);
	// show image
	QPixmap pixmap(pgmImage->getTmpFilePath());
	ui->imageLabel->setPixmap(pixmap);
    }
}

void MainWindow::convolution() {
    statusBar()->showMessage("calculate convolution");

    // ask user which kernel should be used
    if(generateKernel() != 0) {
	statusBar()->showMessage("error while convoluting");
	return;
    }

    // covolution between the image and the given matrix
    if(pgmImage->convolution(kernel, kSize, rotateKernel) != 0) {
	statusBar()->showMessage("error while calculating convolution");
	return;
    }

    // free memory
    for(int i = 0; i < kSize; i++){
	free(kernel[i]);
    }
    free(kernel);

    // show chart
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);

    statusBar()->showMessage("convolution calculated successfully",3000);
}

void MainWindow::hough() {
    statusBar()->showMessage("calculate Hough transformation");

    // caculate Hough transformation
    if(pgmImage->hough() != 0) {
	statusBar()->showMessage("error while calculating Hough transformation");
    } else {
	statusBar()->showMessage("Hough transformation complete",3000);
	// show image
	QPixmap pixmap(pgmImage->getTmpFilePath());
	ui->imageLabel->setPixmap(pixmap);
    }
}

void MainWindow::save() {
    statusBar()->showMessage("save");

    // get path of image
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
						    QDir::homePath(),
						    tr("Portable Graymap (*.pgm)"));

    // load image with standard path
    if(pgmImage->savePgm(fileName) != 0) {
	statusBar()->showMessage("error while saving image");
    } else {
	statusBar()->showMessage("saved successfully",3000);
    }
}

int MainWindow::generateKernel() {
    // Fill List with options
    QStringList items;
    items << tr("Gauss") << tr("Kirsch (rotating)");
    items << tr("Laplacian of the Gaussian (5x5)");
    items << tr("Prewitt 1 (rotating)") << tr("Prewitt 2 (rotating)");
    items << tr("Sobel (rotating)") << tr("other");

    // ask user
    bool ok;
    QString item = QInputDialog::getItem(this, tr("QInputDialog::getItem()"),
					 tr("Season:"), items, 0, false, &ok);
    if (!ok || item.isEmpty()){
	return -1;
    }
    statusBar()->showMessage("choosen " + item);

    // interpret data
    switch(items.indexOf(item)) {
    case 0: //Gauss
	return kernelGauss();
	break;
    case 1: //Kirsch
	return kernelKirsch();
	break;
    case 2: //Laplace
	return kernelLaplace();
	break;
    case 3: //Prewitt 1
	return kernelPrewitt1();
	break;
    case 4: //Prewitt 2
	return kernelPrewitt2();
	break;
    case 5: //Sobel
	return kernelSobel();
	break;
    case 6: //other
	return kernelOther();
	break;
    default:
	return -1;
    }
    return 0;
}

int MainWindow::kernelGauss() {
    // ask user for the size of the kernel
    if(sizeOfKernel() != 0) {
	return -1; //convolution canceled
    }

    // allocate memory for kernel
    mallocKernel();

    // init table with Gauss
    // init borders (top left quarter)
    kernel[0][0] = 1;
    for(int i = 1; i < (kSize+1)/2; i++) {
	kernel[0][i] = kernel[0][i-1] * 2;
	kernel[i][0] = kernel[0][i-1] * 2;
    }
    // init the rest on top left quarter
    for(int row = 1; row < (kSize+1)/2; row++) {
	for(int col = 1; col < (kSize+1)/2; col++) {
	    kernel[col][row] = kernel[row-1][col] * 2;
	    kernel[row][col] = kernel[row-1][col] * 2;
	}
    }
    // mirror this quarter to the others
    for(int row = 0; row < (kSize-1)/2; row++) {
	for(int col = 0; col < (kSize-1)/2; col++) {
	    kernel[kSize-1-col][row] = kernel[row][col];
	    kernel[col][kSize-1-row] = kernel[row][col];
	    kernel[kSize-1-col][kSize-1-row] = kernel[row][col];
	}
    }
    // fill the rest (borders of the bottom right quarter)
    for(int i = 0; i < (kSize-1)/2; i++) {
	kernel[kSize-1-i][(kSize-1)/2] = kernel[i][(kSize-1)/2];
	kernel[(kSize-1)/2][kSize-1-i] = kernel[i][(kSize-1)/2];
    }

    // this kernel is a non-rotating kernel
    rotateKernel = false;
    return 0;
}

int MainWindow::kernelKirsch() {
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  5; kernel[0][1] =  5; kernel[0][2] =  5;
    kernel[1][0] = -3; kernel[1][1] =  0; kernel[1][2] = -3;
    kernel[2][0] = -3; kernel[2][1] = -3; kernel[2][2] = -3;

    // this kernel is a rotating kernel
    rotateKernel = true;
    return 0;
}

int MainWindow::kernelLaplace() {
    // set size of kernel
    kSize = 5;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  0; kernel[0][1] =  0; kernel[0][2] = -1; kernel[0][3] =  0; kernel[0][4] =  0;
    kernel[1][0] =  0; kernel[1][1] = -1; kernel[1][2] = -2; kernel[1][3] = -1; kernel[1][4] =  0;
    kernel[2][0] = -1; kernel[2][1] = -2; kernel[2][2] = 16; kernel[2][3] = -2; kernel[2][4] = -1;
    kernel[3][0] =  0; kernel[3][1] = -1; kernel[3][2] = -2; kernel[3][3] = -1; kernel[3][4] =  0;
    kernel[4][0] =  0; kernel[4][1] =  0; kernel[4][2] = -1; kernel[4][3] =  0; kernel[4][4] =  0;

    // this kernel is a non-rotating kernel
    rotateKernel = false;
    return 0;
}

int MainWindow::kernelPrewitt1() {
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  1; kernel[0][1] =  1; kernel[0][2] =  1;
    kernel[1][0] =  1; kernel[1][1] = -2; kernel[1][2] =  1;
    kernel[2][0] = -1; kernel[2][1] = -1; kernel[2][2] = -1;

    // this kernel is a rotating kernel
    rotateKernel = true;
    return 0;
}

int MainWindow::kernelPrewitt2() {
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  1; kernel[0][1] =  1; kernel[0][2] =  1;
    kernel[1][0] =  0; kernel[1][1] =  0; kernel[1][2] =  0;
    kernel[2][0] = -1; kernel[2][1] = -1; kernel[2][2] = -1;

    // this kernel is a rotating kernel
    rotateKernel = true;
    return 0;
}

int MainWindow::kernelSobel() {
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  1; kernel[0][1] =  2; kernel[0][2] =  1;
    kernel[1][0] =  0; kernel[1][1] =  0; kernel[1][2] =  0;
    kernel[2][0] = -1; kernel[2][1] = -2; kernel[2][2] = -1;

    // this kernel is a rotating kernel
    rotateKernel = true;
    return 0;
}

int MainWindow::kernelOther() {
    // ask user for the size of the kernel
    if(sizeOfKernel() != 0) {
	return -1; //convolution canceled
    }

    // allocate memory for kernel
    mallocKernel();

    // ask user for the contents of the kernel
    int err;
    if((err = contentOfKernel()) != 0) {
	if(err == -1) {
	    return -1; //convolution canceled
	} else {
	    return -2; //wrong value in matrix
	}
    }

    // this kernel is a non-rotating kernel
    rotateKernel = false;
    return 0; //no error
}

int MainWindow::sizeOfKernel() {
    bool ok;
    kSize = QInputDialog::getInt(this,	tr("Size of the kernel"),
					tr("Size of the kernel:"),
					3, 3, 23, 2, &ok);
    if (!ok) {
	return -1;
    }

    // only odd values
    if((kSize % 2) != 1) {
	kSize -= 1;
    }
    return 0;
}

int MainWindow::contentOfKernel() {
    // create dialog with layout
    QDialog dialog(this);
    dialog.setWindowTitle("Kernel");
    QVBoxLayout layout;
    dialog.setLayout(&layout);

    // add label for information
    QLabel label("Configure your kernel", this);
    label.setAlignment(Qt::AlignCenter);
    layout.addWidget(&label);

    // init table with ones
    QTableView tableView(this);
    tableView.horizontalHeader()->hide();
    tableView.verticalHeader()->hide();
    QStandardItemModel model(kSize, kSize);
    for(int row = 0; row < kSize; row++) {
	for(int col = 0; col < kSize; col++) {
	    QStandardItem *item0 = new QStandardItem(QString::number(1));
	    model.setItem(row, col, item0);
	}
    }
    tableView.setModel(&model);
    tableView.resizeColumnsToContents();
    tableView.resizeRowsToContents();

    // create accept button with surroundings
    QHBoxLayout hLayout;
    hLayout.addStretch(1);
    QPushButton acceptBtn(tr("Accept"),this);
    connect(&acceptBtn,SIGNAL(clicked()), &dialog, SLOT(accept()));
    hLayout.addWidget(&acceptBtn);
    hLayout.addStretch(1);

    // fill layout with matrix
    layout.addWidget(&tableView);
    layout.addLayout(&hLayout);

    // exec modal dialog
    if(dialog.exec() == 0) {
	return -1; // user canceled the dialog
    }

    // interpret values of the table
    bool ok;
    for (int row = 0; row < kSize; row++) {
	for (int col = 0; col < kSize; col++) {
	    QModelIndex index = model.index(row, col);
	    kernel[row][col] = model.data(index).toInt(&ok);
	    if(!ok) {
		return -2; // wrong value
	    }
	}
    }
    return 0;
}

void MainWindow::mallocKernel() {
    kernel = (int**) malloc(sizeof(int*) * kSize);
    if(kernel == NULL) {
	qDebug() << "out of memory";
	QMessageBox::warning(this, tr("Error"),	tr("System out of memory"), QMessageBox::Close);
	qApp->exit(-1);
    }
    for(int i = 0; i < kSize; i++){
	kernel[i] = (int*) malloc(sizeof(int) * kSize);
	if(kernel[i] == NULL) {
	    qDebug() << "out of memory";
	    QMessageBox::warning(this, tr("Error"),	tr("System out of memory"), QMessageBox::Close);
	    qApp->exit(-1);
	}
    }
}

void MainWindow::laneDetection() {
    statusBar()->showMessage("start lane detection");

    // GAUSS
    // set size of kernel
    kSize = 7;

    // allocate memory for kernel
    mallocKernel();

    // init table with Gauss
    // init borders (top left quarter)
    kernel[0][0] = 1;
    for(int i = 1; i < (kSize+1)/2; i++) {
	kernel[0][i] = kernel[0][i-1] * 2;
	kernel[i][0] = kernel[0][i-1] * 2;
    }
    // init the rest on top left quarter
    for(int row = 1; row < (kSize+1)/2; row++) {
	for(int col = 1; col < (kSize+1)/2; col++) {
	    kernel[col][row] = kernel[row-1][col] * 2;
	    kernel[row][col] = kernel[row-1][col] * 2;
	}
    }
    // mirror this quarter to the others
    for(int row = 0; row < (kSize-1)/2; row++) {
	for(int col = 0; col < (kSize-1)/2; col++) {
	    kernel[kSize-1-col][row] = kernel[row][col];
	    kernel[col][kSize-1-row] = kernel[row][col];
	    kernel[kSize-1-col][kSize-1-row] = kernel[row][col];
	}
    }
    // fill the rest (borders of the bottom right quarter)
    for(int i = 0; i < (kSize-1)/2; i++) {
	kernel[kSize-1-i][(kSize-1)/2] = kernel[i][(kSize-1)/2];
	kernel[(kSize-1)/2][kSize-1-i] = kernel[i][(kSize-1)/2];
    }

    // this kernel is a non-rotating kernel
    rotateKernel = false;

    // covolution between the image and the given matrix
    if(pgmImage->convolution(kernel, kSize, rotateKernel) != 0) {
	statusBar()->showMessage("error while calculating convolution");
	return;
    }

    // free memory
    for(int i = 0; i < kSize; i++){
	free(kernel[i]);
    }
    free(kernel);

    // SOBEL (vertical)
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  1; kernel[0][1] =  0; kernel[0][2] = -1;
    kernel[1][0] =  2; kernel[1][1] =  0; kernel[1][2] = -2;
    kernel[2][0] =  1; kernel[2][1] =  0; kernel[2][2] = -1;

    // this kernel is a non-rotating kernel
    rotateKernel = false;

    // covolution between the image and the given matrix
    if(pgmImage->convolutionLD(kernel, kSize, rotateKernel) != 0) {
	statusBar()->showMessage("error while calculating convolution");
	return;
    }

    // free memory
    for(int i = 0; i < kSize; i++){
	free(kernel[i]);
    }
    free(kernel);

    // show image
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);

    statusBar()->showMessage("lane detection complete",3000);
}

void MainWindow::laneDetection2() {
    // HOUGH
    // caculate Hough transformation
    if(pgmImage->houghLD() != 0) {
	statusBar()->showMessage("error while calculating Hough transformation");
    } else {
	statusBar()->showMessage("Hough transformation complete",3000);
    }

    // show image
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);
}

void MainWindow::laneDetection3() {
    // HOUGH
    // caculate Hough transformation
    if(pgmImage->dyeLD() != 0) {
	statusBar()->showMessage("error while calculating Hough transformation");
    } else {
	statusBar()->showMessage("Hough transformation complete",3000);
    }

    // show image
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);

    statusBar()->showMessage("lane detection complete",3000);
}

void MainWindow::railDetection() {
    statusBar()->showMessage("start rail detection");

    /*// SOBEL
    // set size of kernel
    kSize = 3;

    // allocate memory for kernel
    mallocKernel();

    // init kernel
    kernel[0][0] =  2; kernel[0][1] =  1; kernel[0][2] =  0;
    kernel[1][0] =  1; kernel[1][1] =  0; kernel[1][2] = -1;
    kernel[2][0] =  0; kernel[2][1] = -1; kernel[2][2] = -2;

    // this kernel is a rotating kernel
    rotateKernel = true;

    // covolution between the image and the given matrix
    if(pgmImage->convolution(kernel, kSize, rotateKernel) != 0) {
	statusBar()->showMessage("error while calculating convolution");
	return;
    }

    // free memory
    for(int i = 0; i < kSize; i++){
	free(kernel[i]);
    }
    free(kernel);*/

    // cut low values
    if(pgmImage->cutRD() != 0) {
	statusBar()->showMessage("error while cutting low values");
    } else {
	statusBar()->showMessage("cut low values",3000);
    }

    // show image
    QPixmap pixmap(pgmImage->getTmpFilePath());
    ui->imageLabel->setPixmap(pixmap);
}
