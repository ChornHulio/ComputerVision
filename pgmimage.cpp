#include "pgmimage.h"

PgmImage::PgmImage() {
    tmpFile = new QTemporaryFile();
    imageHeight = 0;
    imageWidth = 0;
}

PgmImage::~PgmImage() {
    // free imageData memory
    if(imageHeight > 0) {
	for(int i = 0; i < imageHeight; i++){
	    free(imageData[i]);
	}
	free(imageData);
    }

    // close and delete tmpFile
    tmpFile->close();
    delete tmpFile;
}

int PgmImage::loadPgm(QString path) {
    QString str;
    QStringList strList;
    char headerLine[200];

    // open original image
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
	return -1;
    }

    // read header
    // magic number
    file.readLine(headerLine, 5);
    if (QString::compare("P5\n",headerLine,Qt::CaseSensitive) != 0) {
	return -2;
    }
    // comments
    while(true) {
	file.readLine(headerLine, 200);
	if (QString::compare("#",QString(headerLine[0]),Qt::CaseSensitive) != 0) {

	    // width and height
	    str = headerLine;
	    strList = str.split(" ");
	    if(strList.size() != 2) {
		return -2;
	    }
	    this->imageWidth = strList.at(0).toInt();
	    this->imageHeight = strList.at(1).toInt();
	    break;
	}
    }
    // max value of a pixel
    file.readLine(headerLine, 5);
    if (QString::compare("255\n",headerLine,Qt::CaseSensitive) != 0) {
	return -3;
    }

    // read data
    imageData = (char**) malloc(sizeof(char*) * imageHeight);
    for(int i = 0; i < imageHeight; i++){
	imageData[i] = (char*) malloc(sizeof(char) * imageWidth);
	file.read(imageData[i], imageWidth);
    }

    // close file
    file.close();

    // save it in temporary file
    if(saveInTmpPgm() != 0) {
	return -4;
    }
    return 0;
}

int PgmImage::histogram() {

    // create and clean a array for the histogram data
    int histogramData[256];
    for(int i = 0; i < 256; i++) {
	histogramData[i] = 0;
    }

    // analyse data and count this pixels in array
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    histogramData[(unsigned char) imageData[i][j]]++;
	}
    }

    // search for maximum pixel count
    int max = 0;
    for(int i = 0; i < 256; i++) {
	if(histogramData[i] > max) {
	    max = histogramData[i];
	}
    }

    // create chart with width 256 and height 500
    // width is the gray-value
    // height is the count of pixel with this value
    char **histogramChart;
    histogramChart = (char**) malloc(sizeof(char*) * 500);
    for(int i = 0; i < 500; i++){
	histogramChart[i] = (char*) malloc(sizeof(char) * 256);
    }

    // go column by column throught the chart
    for(int i = 0; i < 256; i++) {
	// fill the upper part with white bytes
	for(int j = 0; j < 500 - histogramData[i]*500/max; j++) {
	    histogramChart[j][i] = (unsigned char) 255;
	}
	// fill the lower part with black bytes
	for(int j = 500 - histogramData[i]*500/max +1; j < 500; j++) {
	    histogramChart[j][i] = (unsigned char) 0;
	}
    }

    // save the chart in the tmpFile
    int ret = saveInTmpPgm(histogramChart, 256, 500);

    // free memory
    for(int i = 0; i < 500; i++){
	free(histogramChart[i]);
    }
    free(histogramChart);

    return ret;
}

int PgmImage::invert() {
    // invert data
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    imageData[i][j] = 255 - (unsigned char) imageData[i][j];
	}
    }

    // save it in temporary file
    return saveInTmpPgm();
}

int PgmImage::convolution(int** kernel, int size, bool rotate) {
    int lOfC = (size-1)/2; // one pixel left of center

    // create a new image with the size of the old
    int cImage[imageHeight][imageWidth];

    // calculate sum of the kernel
    int kernelSum = 0;
    for(int i = 0; i < size; i++) {
	for(int j = 0; j < size; j++) {
	    kernelSum += kernel[i][j];
	}
    }

    // convolute image with the given kernel
    // pixel by pixel in the image
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {

	    long valueSum = 0;
	    // array by array in the kernel
	    for(int k = 0; k < size; k++) {
		for(int l = 0; l < size; l++) {

		    // attention: borders
		    if((i-lOfC + k) > 0 && (i-lOfC + k) < imageHeight
		       && (j-lOfC + l) > 0 && (j-lOfC + l) < imageWidth) {
			// multiplize kernel with a pixel of the image
			if(rotate) {
			    valueSum += kernel[k][l] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			    valueSum += kernel[l][size-1-k] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			} else {
			    valueSum += kernel[k][l] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			}
		    } else {
			// multiplize kernel with the color white (for borders)
			valueSum += kernel[k][l] * 255; // white
		    }
		}
	    }

	    // scale and save it in the new image
	    if(kernelSum == 0) {
		cImage[i][j] = valueSum;
	    } else if (!rotate){
		cImage[i][j] = valueSum / kernelSum;
	    } else {
		cImage[i][j] = valueSum / (kernelSum*2);
	    }
	}
    }

    // scale cImage
    int max = 0;
    int min = 0;
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    if(cImage[i][j] > max) {
		max = cImage[i][j];
	    } else if(cImage[i][j] < min) {
		min = cImage[i][j];
	    }
	}
    }
    if(min < 0 || max > 255) {
	for(int i = 0; i < imageHeight; i++) {
	    for(int j = 0; j < imageWidth; j++) {
		cImage[i][j] = (cImage[i][j] - min) * 255 / (max - min);
	    }
	}
    }

    // copy the new image to the original
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    imageData[i][j] = (unsigned char) cImage[i][j];
	}
    }

    // save the chart in the tmpFile
    return saveInTmpPgm();
}

int PgmImage::hough() {
    // threshold of gray value
    int threshold = 20;
    // intervall for local maxima - must be odd
    int intervall = 15;

    // create and init akku
    int akkuHeight = sqrt(imageHeight*imageHeight + imageWidth*imageWidth) + 1;
    int akkuWidth = 360;
    int **akku;
    akku = (int**) malloc(sizeof(int*) * akkuHeight);
    for(int i = 0; i < akkuHeight; i++){
	akku[i] = (int*) malloc(sizeof(int) * akkuWidth);
    }
    for(int row = 0; row < akkuHeight; row++) {
	for(int col = 0; col < akkuWidth; col++) {
	    akku[row][col] = 0;
	}
    }

    // write akku
    for(int x = 1; x < imageWidth; x++) {
	for(int y = 1; y < imageHeight; y++) {
	    if(threshold > (unsigned char) imageData[y][x]) {
		for(int t = 0; t < akkuWidth; t++) {
		    double radian = t * M_PI / 180;
		    int r = round(x*cos(radian) + y*sin(radian));
		    if(r >= 0 && r < akkuHeight) {
			akku[r][t]++;
		    }
		}
	    }
	}
    }

    // find local maximas
    int maxR, maxT;
    QList<QPoint> list;
    for(int r = (intervall-1)/2; r < akkuHeight; r += intervall) {
	for(int t = (intervall-1)/2; t < akkuWidth; t += intervall) {
	    int ret = localMaxima(akku, akkuHeight, akkuWidth, t, r, &maxT, &maxR, intervall);
	    if(ret == 0) {
		// maxima found - save it, when it isn't in the list
		if(!list.contains(QPoint(maxT, maxR))) {
		    list.append(QPoint(maxT, maxR));
		}
	    } else if(ret == 1) {
		// no maxima found
		continue;
	    } else {
		// error while calculation
		return -3;
	    }
	}
    }

    qDebug() << list;

    // draw lines in orginial image
    foreach(QPoint point, list) {
	double sample = 1000;
	double radian = point.x()*M_PI/180;
	if(radian != 0.0) {
	    double m = (-1) * (double) (cos(radian) / sin(radian));
	    double b = (double) (point.y() / (sin(radian)));
	    for(int x = 0; x < imageWidth*sample; x++) {
		int y = round(m * (double) (x/sample) + b);
		if(y >= 0 && y < imageHeight) {
		    imageData[y][(int) round(x/sample)] = 0;
		}
	    }
	}
    }

    // free akku
    for(int i = 0; i < akkuHeight; i++){
	free(akku[i]);
    }
    free(akku);

    // save it in temporary file
    return saveInTmpPgm();
}

int PgmImage::savePgm(QString path) {
    // workaround for Windows
    delete tmpFile;
    tmpFile = new QTemporaryFile();

    // open file
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
	return -1;
    }

    // save it with standard data
    return savePgm(&file, imageData, imageWidth, imageHeight);
}

QString PgmImage::getTmpFilePath() {
    return tmpFile->fileName();
}

int PgmImage::saveInTmpPgm() {
    // save it with standard data
    return saveInTmpPgm(imageData, imageWidth, imageHeight);
}

int PgmImage::saveInTmpPgm(char **data, int width, int height) {
    // workaround for Windows
    delete tmpFile;
    tmpFile = new QTemporaryFile();

    // open temporary file
    if (!tmpFile->open()) {
	return -1;
    }

    // save it
    return savePgm(tmpFile, data, width, height);
}

int PgmImage::savePgm(QFile *file, char **data, int width, int height) {
    // write header
    if(file->write(QByteArray("P5\n")) != 3) {
	return -2;
    } else if (file->write(QByteArray("# Created by Tobias Dreher\n")) != 27) {
	return -2;
    }
    QByteArray tmpArray;
    tmpArray.append(QString::number(width,10));
    tmpArray.append(" ");
    tmpArray.append(QString::number(height,10));
    tmpArray.append("\n255\n");
    if (file->write(tmpArray) <= 0) {
	return -2;
    }

    // write data
    for(int i = 0; i < height; i++) {
	file->write(data[i],width);
    }

    // close file
    file->close();
    return 0;
}

int PgmImage::localMaxima(int** akku, int height, int width, int oldX, int oldY, int *newX, int *newY, int intervall) {
    int threshold = 33; // minimal value of an maxima

    // intervall must be odd
    if(intervall % 2 == 0) {
	return -1;
    }
    int hOfI = (intervall-1)/2;

    // space between upper left corner and start point
    if(oldX < hOfI|| oldY < hOfI) {
	return -1;
    }

    // current values
    int max = akku[oldY][oldX];
    *newX = oldX;
    *newY = oldY;

    // search if any point is greater than the current value
    for(int x = oldX-hOfI; x < width && x < (oldX+hOfI); x++) {
	for(int y = oldY-hOfI; y < height && y < (oldY+hOfI); y++) {
	    if(max <= akku[y][x]) {
		max = akku[y][x];
		*newX = x;
		*newY = y;
	    }
	}
    }

    // if a greater point is found, repeat this procedure
    if(oldX != *newX || oldY != *newY) {
	if(*newX >= hOfI && *newY >= hOfI) {
	    return localMaxima(akku, height, width, *newX, *newY, newX, newY, intervall);
	}
    }

    // if local maxima, which is found, is to small, ignore it
    if(akku[oldY][oldX] < threshold) {
	return 1;
    }
    return 0;
}

int PgmImage::convolutionLD(int** kernel, int size, bool rotate) {
    int lOfC = (size-1)/2; // one pixel left of center

    // create a new image with the size of the old
    int cImage[imageHeight][imageWidth];

    // calculate sum of the kernel
    int kernelSum = 0;
    for(int i = 0; i < size; i++) {
	for(int j = 0; j < size; j++) {
	    kernelSum += kernel[i][j];
	}
    }

    // convolute image with the given kernel
    // pixel by pixel in the image
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {

	    long valueSum = 0;
	    // array by array in the kernel
	    for(int k = 0; k < size; k++) {
		for(int l = 0; l < size; l++) {

		    // attention: borders
		    if((i-lOfC + k) > 0 && (i-lOfC + k) < imageHeight
		       && (j-lOfC + l) > 0 && (j-lOfC + l) < imageWidth) {
			// multiplize kernel with a pixel of the image
			if(rotate) {
			    valueSum += kernel[k][l] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			    valueSum += kernel[l][size-1-k] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			} else {
			    valueSum += kernel[k][l] * (unsigned char) imageData[i-lOfC + k][j-lOfC + l];
			}
		    } else {
			// multiplize kernel with the color white (for borders)
			valueSum += kernel[k][l] * 255; // white
		    }
		}
	    }

	    // scale and save it in the new image
	    if(kernelSum == 0) {
		cImage[i][j] = valueSum;
	    } else if (!rotate){
		cImage[i][j] = valueSum / kernelSum;
	    } else {
		cImage[i][j] = valueSum / (kernelSum*2);
	    }
	}
    }

    // scale cImage
    int max = 0;
    int min = 0;
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    if(cImage[i][j] > max) {
		max = cImage[i][j];
	    } else if(cImage[i][j] < min) {
		min = cImage[i][j];
	    }
	}
    }
    if(min < 0 || max > 255) {
	for(int i = 0; i < imageHeight; i++) {
	    for(int j = 0; j < imageWidth; j++) {
		cImage[i][j] = (cImage[i][j] - min) * 255 / (max - min);
	    }
	}
    }

    // copy the new image to the original (filter gray values)
    for(int i = 0; i < imageHeight; i++) {
	for(int j = 0; j < imageWidth; j++) {
	    //imageData[i][j] = (unsigned char) cImage[i][j];
	    if( (unsigned char) cImage[i][j] < 120 ||  (unsigned char) cImage[i][j] > 135) {
		imageData[i][j] = (unsigned char) 0;
	    } else {
		imageData[i][j] = (unsigned char) 255;
	    }
	}
    }

    // save the chart in the tmpFile
    return saveInTmpPgm();
}

int PgmImage::houghLD() {
    // threshold of gray value
    int threshold = 20;
    // intervall for local maxima - must be odd
    int intervall = 21;

    // create and init akku
    int akkuHeight = sqrt(imageHeight*imageHeight + imageWidth*imageWidth) + 1;
    int akkuWidth = 360;
    int **akku;
    akku = (int**) malloc(sizeof(int*) * akkuHeight);
    for(int i = 0; i < akkuHeight; i++){
	akku[i] = (int*) malloc(sizeof(int) * akkuWidth);
    }
    for(int row = 0; row < akkuHeight; row++) {
	for(int col = 0; col < akkuWidth; col++) {
	    akku[row][col] = 0;
	}
    }

    // write akku
    for(int x = 1; x < imageWidth; x++) {
	for(int y = 1; y < imageHeight; y++) {
	    if(threshold > (unsigned char) imageData[y][x]) {
		for(int t = 0; t < akkuWidth; t++) {
		    double radian = t * M_PI / 180;
		    int r = round(x*cos(radian) + y*sin(radian));
		    if(r >= 0 && r < akkuHeight) {
			akku[r][t]++;
		    }
		}
	    }
	}
    }

    // find local maximas
    int maxR, maxT;
    QList<QPoint> list;
    for(int r = (intervall-1)/2; r < akkuHeight; r += intervall) {
	for(int t = (intervall-1)/2; t < akkuWidth; t += intervall) {
	    int ret = localMaximaLD(akku, akkuHeight, akkuWidth, t, r, &maxT, &maxR, intervall);
	    if(ret == 0) {
		// maxima found - save it, when it isn't in the list
		if(!list.contains(QPoint(maxT, maxR))) {
		    list.append(QPoint(maxT, maxR));
		}
	    } else if(ret == 1) {
		// no maxima found
		continue;
	    } else {
		// error while calculation
		return -3;
	    }
	}
    }

    qDebug() << list;

    // draw lines in orginial image
    foreach(QPoint point, list) {
	double sample = 1000;
	double radian = point.x()*M_PI/180;
	if(radian != 0.0) {
	    double m = (-1) * (double) (cos(radian) / sin(radian));
	    double b = (double) (point.y() / (sin(radian)));
	    //if((b > -260 && b < -160) || (b > 600 && b < 700)) {
		if((m > -0.85 && m < -0.55) || (m > 0.55 && m < 1.05)) {
		    for(int x = 0; x < imageWidth*sample; x++) {
			int y = round(m * (double) (x/sample) + b);
			if(y >= 0 && y < imageHeight) {
			    imageData[y][(int) round(x/sample)] = 0;
			}
		    }
		}
	    //}
	}

	// left:  m = -0.7 && b =  650
	// right: m =  0.8 && b = -210
    }

    // free akku
    for(int i = 0; i < akkuHeight; i++){
	free(akku[i]);
    }
    free(akku);

    // save it in temporary file
    return saveInTmpPgm();
}

int PgmImage::localMaximaLD(int** akku, int height, int width, int oldX, int oldY, int *newX, int *newY, int intervall) {
    int threshold = 51; // minimal value of an maxima

    // intervall must be odd
    if(intervall % 2 == 0) {
	return -1;
    }
    int hOfI = (intervall-1)/2;

    // space between upper left corner and start point
    if(oldX < hOfI|| oldY < hOfI) {
	return -1;
    }

    // current values
    int max = akku[oldY][oldX];
    *newX = oldX;
    *newY = oldY;

    // search if any point is greater than the current value
    for(int x = oldX-hOfI; x < width && x < (oldX+hOfI); x++) {
	for(int y = oldY-hOfI; y < height && y < (oldY+hOfI); y++) {
	    if(max <= akku[y][x]) {
		max = akku[y][x];
		*newX = x;
		*newY = y;
	    }
	}
    }

    // if a greater point is found, repeat this procedure
    if(oldX != *newX || oldY != *newY) {
	if(*newX >= hOfI && *newY >= hOfI) {
	    return localMaxima(akku, height, width, *newX, *newY, newX, newY, intervall);
	}
    }

    // if local maxima, which is found, is to small, ignore it
    if(akku[oldY][oldX] < threshold) {
	return 1;
    }
    return 0;
}

int PgmImage::dyeLD() {
    // dye(imageWidth/2, imageHeight/2, 255, 128);
    dye(imageWidth/2, 53, 255, 128);

    // calculate lane width
    int laneWidth[imageHeight];
    for(int y = 0; y < imageHeight; y++) {
	laneWidth[y] = 0;
	for(int x = 0; x < imageWidth; x++) {
	    if((unsigned char) imageData[y][x] == 128) {
		laneWidth[y]++;
	    }
	}
    }

    // calculate lane middle
    for(int y = 0; y < imageHeight; y++) {
	for(int x = 0; x < imageWidth-10; x++) {
	    if((unsigned char) imageData[y][x] == 128 && (unsigned char) imageData[y][x+10] == 128) {
		laneWidth[y] = x + laneWidth[y]/2;
		break;
	    }
	}
    }

    // paint lane middle
    for(int y = 5; y < imageHeight-10; y+=2) {
	long lanePos = 0;
	for(int i = 0; i < 21; i++) {
	    lanePos += laneWidth[y+i-10];
	}
	lanePos /= 21;
	if(lanePos > 1 && lanePos < imageWidth-1) {
	    imageData[y][lanePos-1] = (unsigned char) 0;
	    imageData[y][lanePos]   = (unsigned char) 0;
	    imageData[y][lanePos+1] = (unsigned char) 0;
	}
    }


    // save it in temporary file
    return saveInTmpPgm();
}

void PgmImage::dye(int curX, int curY, int oldValue, int newValue) {
    imageData[curY][curX] = newValue;
    if(curX < (imageWidth-1) && (unsigned char) imageData[curY][curX+1] == oldValue) {
	dye(curX+1, curY, oldValue, newValue);
    }
    if(curY < (imageHeight-1) && (unsigned char) imageData[curY+1][curX] == oldValue) {
	dye(curX, curY+1, oldValue, newValue);
    }
    if(curX > 0 && (unsigned char) imageData[curY][curX-1] == oldValue) {
	dye(curX-1, curY, oldValue, newValue);
    }
}

int PgmImage::cutRD() {
    // cut borders
    // bottom
    for(int y = 0; y < 15; y++) {
	for(int x = 0; x < imageWidth; x++) {
	   imageData[y][x] = (unsigned char) 0;
	}
    }
    // top
    for(int y = imageHeight-15; y < imageHeight; y++) {
	for(int x = 0; x < imageWidth; x++) {
	   imageData[y][x] = (unsigned char) 0;
	}
    }
    // left
    for(int y = 0; y < imageHeight; y++) {
	for(int x = 0; x < 15; x++) {
	   imageData[y][x] = (unsigned char) 0;
	}
    }
    // right
    for(int y = 0; y < imageHeight; y++) {
	for(int x = imageWidth-15; x < imageWidth; x++) {
	   imageData[y][x] = (unsigned char) 0;
	}
    }


    // cut all lower values und invert it
    for(int y = 0; y < imageHeight; y++) {
	for(int x = 0; x < imageWidth; x++) {
	    if((unsigned char) imageData[y][x] < 140) {
		imageData[y][x] = (unsigned char) 255;
	    } else {
		imageData[y][x] = (unsigned char) 0;
	    }
	}
    }

    //hough
    if(houghRD() != 0) {
	return 3;
    }

    // save it in temporary file
    return saveInTmpPgm();
}

int PgmImage::houghRD() {
    // threshold of gray value
    int threshold = 40;
    // intervall for local maxima - must be odd
    int intervall = 15;

    // create and init akku
    int akkuHeight = sqrt(imageHeight*imageHeight + imageWidth*imageWidth) + 1;
    int akkuWidth = 360;
    int **akku;
    akku = (int**) malloc(sizeof(int*) * akkuHeight);
    for(int i = 0; i < akkuHeight; i++){
	akku[i] = (int*) malloc(sizeof(int) * akkuWidth);
    }
    for(int row = 0; row < akkuHeight; row++) {
	for(int col = 0; col < akkuWidth; col++) {
	    akku[row][col] = 0;
	}
    }

    // write akku
    for(int x = 1; x < imageWidth; x++) {
	for(int y = 1; y < imageHeight; y++) {
	    if(threshold > (unsigned char) imageData[y][x]) {
		for(int t = 0; t < akkuWidth; t++) {
		    double radian = t * M_PI / 180;
		    int r = round(x*cos(radian) + y*sin(radian));
		    if(r >= 0 && r < akkuHeight) {
			akku[r][t]++;
		    }
		}
	    }
	}
    }

    // find two maximas
    int maxLowR = 0;
    int maxLowT = 0;
    int maxHighR = 0;
    int maxHighT = 0;
    int maxHigh = 0;
    for(int r = 0; r < akkuHeight; r++) {
	for(int t = 0; t < akkuWidth; t++) {
	    if(akku[r][t] > maxHigh) {
		maxHigh = akku[r][t];
		maxHighR = r;
		maxHighT = t;
	    }
	}
    }
    maxHigh = 0;
    for(int r = 0; r < akkuHeight; r++) {
	for(int t = 0; t < akkuWidth; t++) {
	    if(akku[r][t] > maxHigh) {
		if(   (r > (maxHighR + 0.15*maxHighR)
		    || r < (maxHighR - 0.15*maxHighR))
		    &&(t < (maxHighT + 0.10*maxHighT)
		    && t > (maxHighT - 0.10*maxHighT))) {
		    maxLowR = r;
		    maxLowT = t;
		    maxHigh = akku[r][t];
		}
	    }
	}
    }
    QList<QPoint> list;
    list.append(QPoint(maxLowT, maxLowR));
    list.append(QPoint(maxHighT, maxHighR));
    qDebug() << list;

    // draw lines in orginial image
    foreach(QPoint point, list) {
	double sample = 1000;
	double radian = point.x()*M_PI/180;
	if(radian != 0.0) {
	    double m = (-1) * (double) (cos(radian) / sin(radian));
	    double b = (double) (point.y() / (sin(radian)));
	    for(int x = 0; x < imageWidth*sample; x++) {
		int y = round(m * (double) (x/sample) + b);
		if(y >= 0 && y < imageHeight) {
		    imageData[y][(int) round(x/sample)] = 0;
		}
	    }
	}
    }

    // free akku
    for(int i = 0; i < akkuHeight; i++){
	free(akku[i]);
    }
    free(akku);
}

