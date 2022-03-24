#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <complex>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
//Library to read images
#include <dirent.h>
using namespace cv;
using namespace std;

const double PI = 3.14159265359;
int rows, cols, channels;
int colorCast=1;

Mat input_img;
vector<vector<int>> red;
vector<vector<int>> blue;
vector<vector<int>> green;

double meanRed, meanBlue, meanGreen;

vector<vector<double>> hue;
vector<vector<vector<double>>> colorBalanced;

//Function to get hue value at (i, j)
double hueValue(int i, int j)
{
	double R = red[i][j]; double G = green[i][j]; double B = blue[i][j];
	double V = max(R, B);
	V = max(V, G);
	
	double minm = min(R, B);
	minm = min(minm, G);

	if (R==G && G==B)
		return 0;

	double h;
	if (V == R)
	{
		h = (60 * (G - B));
		h /= (V - minm);
	}
	else if (V == B)
		h = 240 + (60 * (R - G))/ (V - minm);
	
	else
		h = 120 + (60 * (B - R)) / (V - minm);

	if (h < 0)
		h = h + 360;

	return h/360;
}

// MOS Calculation
double MOS()
{
	hue.resize(rows);
	
	double hueSum = 0;
	for (int i = 0; i < rows; i++)
	{
		hue[i].resize(cols);
		for (int j = 0; j <cols; j++)
		{
			hue[i][j] = hueValue(i, j);
			hueSum += hue[i][j];
		}
	}

	double meanHue = hueSum / (rows * cols);

	double delSum = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			double t1 = (meanHue - hue[i][j]) * (meanHue - hue[i][j]);
			double t2 = (1 - abs(meanHue - hue[i][j])) * (1 - abs(meanHue - hue[i][j]));
			delSum += min(t1, t2);
		}
	}



	return delSum/(rows*cols);
}


// ColorCast Balancing

void colorCastBalancing()
{
	colorBalanced.resize(rows);
	if (colorCast)
	{
		double maxMean = max(meanRed, meanGreen);
		maxMean = max(maxMean, meanBlue);

		double nr, nb, ng;
		nr = meanRed / maxMean;
		nb = meanBlue / maxMean;
		ng = meanGreen / maxMean;

		cout << nb << " " << nr << "  " << ng << endl;

		

		for (int i = 0; i < rows; i++)
		{
			colorBalanced[i].resize(cols);
			for (int j = 0; j < cols; j++)
			{
				colorBalanced[i][j].resize(3);
				double maxmi = max(blue[i][j], green[i][j]);
				maxmi = max(maxmi, red[i][j]);
				colorBalanced[i][j][0] = min(pow(blue[i][j], nb), maxmi);
				colorBalanced[i][j][1] = min(pow(green[i][j], ng), maxmi);
				colorBalanced[i][j][2] = min(pow(red[i][j], nr), maxmi);
			}
		}
	}
	else
	{
		for (int i = 0; i < rows; i++)
		{
			colorBalanced[i].resize(cols);
			for (int j = 0; j < cols; j++)
			{
				colorBalanced[i][j].resize(channels);
				colorBalanced[i][j][0] = blue[i][j];
				colorBalanced[i][j][1] = green[i][j];
				colorBalanced[i][j][2] = red[i][j];
			}
		}
	}
}


//Output Image
Mat form_output_img()
{
	Mat out_img = input_img.clone();
	for (int i = 0; i < input_img.rows; i++)
		for (int j = 0; j < input_img.cols; j++)
		{
			out_img.at<Vec3b>(i, j)[0] = colorBalanced[i][j][0];
			out_img.at<Vec3b>(i, j)[1] = colorBalanced[i][j][1];
			out_img.at<Vec3b>(i, j)[2] = colorBalanced[i][j][2];
				//out_img.at<uchar>(Point(i, j)) = (red[i][j]);
		}
	return out_img;
}



int main() {
	string img_path = "cast1.jpg";

	//creating windows
	namedWindow("Input Image", WINDOW_AUTOSIZE);
	namedWindow("Output Image", WINDOW_AUTOSIZE);
	input_img = imread(img_path, IMREAD_UNCHANGED);

	cout << " Rows: " << input_img.rows << endl;
	cout << " Columns: " << input_img.cols << endl;
	cout << " Channels: " << input_img.channels() << endl;

	rows = input_img.rows;
	cols = input_img.cols;
	channels = input_img.channels();
	
	red.resize(rows);
	green.resize(rows);
	blue.resize(rows);

	double sumBlue=0, sumRed = 0, sumGreen = 0;
	for (int i = 0; i < rows; i++)
	{
		red[i].resize(cols);
		green[i].resize(cols);
		blue[i].resize(cols);
		for (int j = 0; j < cols; j++)
		{
			//BGR -> Order of channels
			blue[i][j] = (input_img.at<Vec3b>(i, j)[0]);
			green[i][j] = (input_img.at<Vec3b>(i, j)[1]);
			red[i][j] = (input_img.at<Vec3b>(i, j)[2]);
			sumBlue += blue[i][j];
			sumRed += red[i][j];
			sumGreen += green[i][j];
		}
	}

	meanBlue = sumBlue / (rows * cols);
	meanRed = sumRed / (rows * cols);
	meanGreen = sumGreen / (rows * cols);

	
	double mosValue = MOS();

	cout << "MOS: " << mosValue << endl;

	if (mosValue > 0.006)
		colorCast = 0;
	
	colorCastBalancing();


	//Converting to image
	Mat out_img;
	out_img = form_output_img();

	//Showing the images
	imshow("Input Image", input_img);
	imshow("Output Image", out_img);
	waitKey();
	return 0;
}