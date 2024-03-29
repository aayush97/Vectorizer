// Vectorizer.cpp : Defines the entry point for the console application.
// uses openCV library
// the library's main purpose in this program for loading different raster image formats: jpg, png etc
// in a 2D matrix form.
#include "stdafx.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

using namespace cv;
using namespace std;

const int MAX_HEIGHT = 5005;
const int MAX_WIDTH = 5005;

// structure to store the structure of a svg rectangle
struct svgRect {
	int x, y, width, height, intensity;
	svgRect(int x, int y, int width, int height, unsigned intensity) :
		x(x), y(y), width(width), height(height), intensity(intensity){}
};

Mat img;	// global variable: in a better design all these could be the members of the same class
const double TOLERANCE = 100.0;


vector<svgRect> rectangles;


// memoization tables
long long sumSquare[MAX_HEIGHT][MAX_WIDTH];
long long sumNorm[MAX_HEIGHT][MAX_WIDTH];
long long centreX[MAX_HEIGHT][MAX_WIDTH];
long long centreY[MAX_HEIGHT][MAX_WIDTH];


/*
*	initializes the tables defined above
*	the tables are memoization/helper tables for fast calculation
*	of sum of intensities, sum of square of intensities and center of gravity
*	in a particular rectangle within the image
*/
void init(int r, int c) {

	for (int i = 0; i < r; i++) {
		for (int j = 0; j < c; j++) {
			Scalar intensity = img.at<uchar>(i, j);
			int w = intensity.val[0];
			w++;
			if (i == 0) {
				sumNorm[i][j] = w;
				sumSquare[i][j] = w*w;
				centreY[i][j] = i * w;
				centreX[i][j] = j * w;
			}else {
				sumNorm[i][j] = sumNorm[i-1][j] +  w;
				sumSquare[i][j] = sumSquare[i-1][j] + w*w;
				centreY[i][j] = centreY[i-1][j] + i * w;
				centreX[i][j] = centreX[i-1][j] + j * w;
			}
		}
	}

	for (int i = 0; i < r; i++) {
		for (int j = 1; j < c; j++) {
			sumNorm[i][j] += sumNorm[i][j - 1];
			sumSquare[i][j] += sumSquare[i][j - 1];
			centreY[i][j] += centreY[i][j - 1];
			centreX[i][j] += centreX[i][j - 1];
		}
	}




}

/*
*	vectorizes the given image recursively by taking a rectangular image
*	calculating the variance of intensities of of the pixels in the image and making decision as follows:
*	i. if variance is less than TOLERANCE the rectangle is plotted as the average intensity of all the pixels in the image
*	ii. else divide the rectangle into four rectangles from the centre of gravity the the pixels
*/
void vectorize(int sr,int sc, int nr, int nc) {

	if (nr <= 0 or nc <= 0)
		return;
	long long avg = 1;
	long long cgx = 0, cgy = 0;
	long long variance = 0;
	if (sr == 0 and sc == 0) {
		avg = sumNorm[sr + nr - 1][sc + nc - 1];
		variance = sumSquare[sr + nr - 1][sc + nc - 1];
		cgx = centreX[sr + nr - 1][sc + nc - 1];
		cgy = centreY[sr + nr - 1][sc + nc - 1];
	}else if (sr == 0) {
		avg = sumNorm[sr + nr - 1][sc + nc - 1] - sumNorm[sr + nr - 1][sc - 1];
		variance = sumSquare[sr + nr - 1][sc + nc - 1] - sumSquare[sr + nr - 1][sc - 1];
		cgx = centreX[sr + nr - 1][sc + nc - 1] - centreX[sr + nr - 1][sc - 1];
		cgy = centreY[sr + nr - 1][sc + nc - 1] - centreY[sr + nr - 1][sc - 1];
	}else if (sc == 0) {
		avg = sumNorm[sr + nr - 1][sc + nc - 1] - sumNorm[sr - 1][sc + nc - 1];
		variance = sumSquare[sr + nr - 1][sc + nc - 1] - sumSquare[sr - 1][sc + nc - 1];
		cgx = centreX[sr + nr - 1][sc + nc - 1] - centreX[sr - 1][sc + nc - 1];
		cgy = centreY[sr + nr - 1][sc + nc - 1] - centreY[sr - 1][sc + nc - 1];
	}else {
		avg = sumNorm[sr + nr - 1][sc + nc - 1] - sumNorm[sr - 1][sc + nc - 1] - sumNorm[sr + nr - 1][sc - 1] + sumNorm[sr - 1][sc - 1];
		variance = sumSquare[sr + nr - 1][sc + nc - 1] - sumSquare[sr - 1][sc + nc - 1] - sumSquare[sr + nr - 1][sc - 1] + sumSquare[sr - 1][sc - 1];
		cgx = centreX[sr + nr - 1][sc + nc - 1] - centreX[sr - 1][sc + nc - 1] - centreX[sr + nr - 1][sc - 1] + centreX[sr - 1][sc - 1];
		cgy = centreY[sr + nr - 1][sc + nc - 1] - centreY[sr - 1][sc + nc - 1] - centreY[sr + nr - 1][sc - 1] + centreY[sr - 1][sc - 1];
	}
	
	cgx /= avg;
	cgy /= avg;
	assert(variance*nr*nc - avg * avg >= 0);
	avg /= (nc*nr);
	assert(cgx >= sc and cgy >= sr);
	double var = variance / double(nr*nc);
	var -= (long long)avg * avg;
	if (var <= TOLERANCE or (nr==cgy-sr+1 and nc==cgx-sc+1)) {
		//push the rectangle for plotting
		rectangles.push_back(svgRect(sc,sr,nc,nr,avg-1));
		return; 
	}
	vectorize(sr, sc, cgy-sr+1, cgx-sc+1);
	vectorize(sr, cgx+1, cgy+1-sr, nc - (cgx+1 - sc));
	vectorize(cgy+1, sc, nr - (cgy+1 - sr), cgx+1-sc);
	vectorize(cgy + 1, cgx + 1, nr - (cgy + 1 - sr), nc - (cgx + 1 - sc));
	
}

int main() {
	string filename = "image5.png";
	img = imread(filename);
	cvtColor(img, img, CV_BGR2GRAY);
	int numRows = img.rows, numCols = img.cols;
	init(numRows, numCols);
	vectorize(0, 0, numRows, numCols);
	string outputFileName = "outputImage.svg";
	ofstream outFile(outputFileName);

	// write to svg file
	outFile << "<svg width=\"" << numCols <<"\"" << " height=\"" << numRows << "\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink= \"http://www.w3.org/1999/xlink\" >" << endl;
	for (int i = 0; i < rectangles.size(); i++) {
		outFile << "<rect x = \"" << rectangles[i].x << "\" "
			<< "y = \"" << rectangles[i].y << "\" "
			<< "width = \"" << rectangles[i].width << "\" "
			<< "height = \"" << rectangles[i].height << "\" "
			<< "style = \"fill:rgb(" << rectangles[i].intensity << "," << rectangles[i].intensity << "," << rectangles[i].intensity << ")\" />" << endl;
	}
	outFile << "</svg>";
	outFile.close();

	// show the actual raster image.
	imshow("main", img);
	waitKey(0);

	return 0;
	
}