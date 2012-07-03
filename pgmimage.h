#ifndef IMAGE_H
#define IMAGE_H

#include <QString>
#include <QTemporaryFile>
#include <QStringList>
#include <QList>
#include <QPoint>
#include <QDebug>
#include <math.h>

/**
  * PGM Image with functions to invert, save and create a histogram
  */
class PgmImage
{
private:
    QTemporaryFile *tmpFile; ///< temporary file for the image
    int imageHeight; ///< height of the image
    int imageWidth; ///< width of the image
    char **imageData; ///< image (size: [imageHeight][imageWidth])

public:
    PgmImage();
    ~PgmImage();

    /**
      * load an pgm image and save it in a temporary file
      *
      * @param path path of the original image
      * @return  0 -> image loaded successfully
      *         -1 -> no such file
      *         -2 -> no pgm file-format
      *         -3 -> cannot handle this pgm file
      *         -4 -> error while writing temporary file
      */
    int loadPgm(QString path);

    /**
      * create a histogram and save it in a temporary file
      *
      * @return  0 -> histogram created successfully
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      */
    int histogram();

    /**
      * invert the image and save it in a temporary file
      *
      * @return  0 -> image inverted successfully
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      */
    int invert();

    /**
      * convolute the image with a given kernel and save it in a temporary file
      *
      * @param kernel colvolute image with this kernel
      * @param size x and y size of the kernel
      * @param rotate convolute with the kernel and with the rotated kernel
      * @return  0 -> image convolute successfully
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      */
    int convolution(int** kernel, int size, bool rotate);

    /**
      * convolute the image with a given kernel and save it in a temporary file
      * other scale algo than upper method
      *
      * @param kernel colvolute image with this kernel
      * @param size x and y size of the kernel
      * @param rotate convolute with the kernel and with the rotated kernel
      * @return  0 -> image convolute successfully
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      */
    int convolutionLD(int** kernel, int size, bool rotate);

    /**
      * calculate the Hough transformation and save it in a temporary file
      *
      * @return  0 -> calculation of Hough transformation complete
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      *         -3 -> error while calculation
      */
    int hough();

    /**
      * calculate the Hough transformation (for lane detection) and save it in
      * a temporary file
      *
      * @return  0 -> calculation of Hough transformation complete
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      *         -3 -> error while calculation
      */
    int houghLD();

    /**
      * dye image with gray and save it in a temporary file
      *
      * @return  0 -> calculation of Hough transformation complete
      *         -1 -> error while opening temporary file
      *         -2 -> error while writing temporary file
      *         -3 -> error while calculation
      */
    int dyeLD();

    /**
      * save the image (pgm) to the given path
      *
      * @param path path to save the image
      * @return  0 -> saved successfully
      *         -1 -> error while opening path
      *         -2 -> error while writing the file
      */
    int savePgm(QString path);

    /**
      * get the path to the temporary file
      *
      * @return  path to temporary file
      */
    QString getTmpFilePath();

    /**
      * cut lower values (0 - 139), invert and hough
      *
      * @return  0 -> successfully
      *         -1 -> error while opening path
      *         -2 -> error while writing the file
      *         -3 -> error while calculating
      */
    int cutRD();

private:
    /**
      * save the temporary pgm file with standard data
      *
      * @return  0 -> saved successfully
      *         -1 -> error while opening path
      *         -2 -> error while writing the file
      */
    int saveInTmpPgm();

    /**
      * save the temporary pgm file with the given data
      *
      * @param data two dimension array which represents the pgm image
      * @param width width of the pgm image
      * @param height height of the pgm image
      * @return  0 -> saved successfully
      *         -1 -> error while opening path
      *         -2 -> error while writing the file
      */
    int saveInTmpPgm(char **data, int width, int height);

    /**
      * save the pgm file with the given data
      *
      * @param file instance of QFile of the file to save
      * @param data two dimension array which represents the pgm image
      * @param width width of the pgm image
      * @param height height of the pgm image
      * @return  0 -> saved successfully
      *         -1 -> error while opening path
      *         -2 -> error while writing the file
      */
    int savePgm(QFile *file, char **data, int width, int height);

    /**
      * recursive function to find a local maxima (threshold = 30)
      *
      * @param akku pointer to matrix
      * @param height height of the matrix
      * @param width width of the matrix
      * @param oldX startpoint (X) to search
      * @param oldY startpoint (Y) to search
      * @param newX pointer to found X-point of local maxima
      * @param newY pointer to found Y-point of local maxima
      * @param intervall array(intervall x intervall) to search
      * @return  1 -> no maxima found
      *          0 -> maxima found
      *         -1 -> error while calculation
      */
    int localMaxima(int** akku, int height, int width, int oldX, int oldY, int *newX, int *newY, int intervall);

    /**
      * recursive function to find a local maxima (threshold = 30)
      * optimized for lane detection
      *
      * @param akku pointer to matrix
      * @param height height of the matrix
      * @param width width of the matrix
      * @param oldX startpoint (X) to search
      * @param oldY startpoint (Y) to search
      * @param newX pointer to found X-point of local maxima
      * @param newY pointer to found Y-point of local maxima
      * @param intervall array(intervall x intervall) to search
      * @return  1 -> no maxima found
      *          0 -> maxima found
      *         -1 -> error while calculation
      */
    int localMaximaLD(int** akku, int height, int width, int oldX, int oldY, int *newX, int *newY, int intervall);

    /**
      * dye image
      *
      * @param curX startpoint (X) to search and replace
      * @param curY startpoint (Y) to search and replace
      * @param oldValue old color value
      * @param newValue new color value
      */
    void dye(int curX, int curY, int oldValue, int newValue);

    /**
      * calculate the Hough transformation for rail detection
      *
      * @return  0 -> calculation of Hough transformation complete
      *         -3 -> error while calculation
      */
    int houghRD();
};

#endif // IMAGE_H
