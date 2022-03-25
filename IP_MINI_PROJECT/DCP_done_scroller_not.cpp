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
int colorCast = 1;
vector<string> imgs_name;
int img_number = 0;
Mat input_img;
Mat out_img;
vector<vector<int>> red;
vector<vector<int>> blue;
vector<vector<int>> green;

double meanRed, meanBlue, meanGreen;

vector<vector<double>> hue;
vector<vector<vector<double>>> colorBalanced;
vector < vector<double>> dcp;



vector<string> read_images(string path)
{
	vector<string> imgs;
	string img_name;
	string temp;
	string img_path = path;
	const char* path_img = &img_path[0];
	DIR* dir = opendir(path_img);
	struct dirent* dp;
	//Make vector of images name placed in path
	if (dir != NULL)
	{
		while ((dp = readdir(dir)) != NULL)
		{
			img_name = dp->d_name;
			//check for garbage value
			if (img_name.length() >= 4)
			{
				temp = img_name.substr(img_name.length() - 3, 3);
				if (temp == "jpg")
					imgs.push_back(img_path + "/" + img_name);
			}
		}
	}
	(void)closedir(dir);

	return imgs;
}

//Function to get hue value at (i, j)
double hueValue(int i, int j)
{
	double R = red[i][j]; double G = green[i][j]; double B = blue[i][j];
	double V = max(R, B);
	V = max(V, G);

	double minm = min(R, B);
	minm = min(minm, G);

	if (R == G && G == B)
		return 0;

	double h;
	if (V == R)
	{
		h = (60 * (G - B));
		h /= (V - minm);
	}
	else if (V == B)
		h = 240 + (60 * (R - G)) / (V - minm);

	else
		h = 120 + (60 * (B - R)) / (V - minm);

	if (h < 0)
		h = h + 360;

	return h / 360;
}

// MOS Calculation
double MOS()
{
	hue.clear();
	hue.resize(rows);

	double hueSum = 0;
	for (int i = 0; i < rows; i++)
	{
		hue[i].resize(cols);
		for (int j = 0; j < cols; j++)
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



	return delSum / (rows * cols);
}


// ColorCast Balancing

void colorCastBalancing()
{
	colorBalanced.clear();
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

		//nr = 0.38;
		//ng = 0.5;
		//nb = 0.42;


		for (int i = 0; i < rows; i++)
		{
			colorBalanced[i].resize(cols);
			for (int j = 0; j < cols; j++)
			{
				colorBalanced[i][j].resize(3);
				double maxmi = max(blue[i][j], green[i][j]);
				maxmi = max(maxmi, red[i][j]);

				//colorBalanced[i][j][0] = min(pow(blue[i][j], nb), maxmi);
				//colorBalanced[i][j][1] = min(pow(green[i][j], ng), maxmi);
				////colorBalanced[i][j][1] = green[i][j];
				//colorBalanced[i][j][2] = min(pow(red[i][j], nr), maxmi);

				double mean = (meanBlue + meanRed + meanGreen) / 3;
				
				colorBalanced[i][j][0] = blue[i][j]*(mean/meanBlue);
				colorBalanced[i][j][1] = green[i][j]* (mean / meanGreen);
				colorBalanced[i][j][2] = red[i][j]* (mean / meanRed);
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

void DCPCal()
{
	dcp.clear();
	dcp.resize(rows);
	for (int i = 0; i < rows; i++)
	{
		dcp[i].resize(cols);
		for (int j = 0; j < cols; j++) {
			dcp[i][j] = 1e9;
		}
	}
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++) {
			int minipix = min(colorBalanced[i][j][0], min(colorBalanced[i][j][1], colorBalanced[i][j][2]));
			int dx[8] = { 1,-1,0,0,-1,-1,1,1 };
			int dy[8] = { 0,0,1,-1,1,-1,1,-1 };
			for (int i2 = 0; i2 < 8; i2++) {
				for (int j2 = 0; j2 < 8; j2++) {
					if (i + dx[i2] >= 0 && i + dx[i2] < rows && j + dy[j2] >= 0 && j + dy[j2] < cols)
					{
						int tmpmin = min(colorBalanced[i + dx[i2]][j + dy[j2]][0], min(colorBalanced[i + dx[i2]][j + dy[j2]][1], colorBalanced[i + dx[i2]][j + dy[j2]][2]));
						minipix = min(minipix, tmpmin);
					}
				}
			}
			dcp[i][j] = minipix;
		}
	}
}

//Output Image
void form_output_img()
{
	colorCastBalancing();
	DCPCal();
	out_img = input_img.clone();
	for (int i = 0; i < input_img.rows; i++)
		for (int j = 0; j < input_img.cols; j++)
		{
			//for colorcast removed image
			out_img.at<Vec3b>(i, j)[0] = colorBalanced[i][j][0];
			out_img.at<Vec3b>(i, j)[1] = colorBalanced[i][j][1];
			out_img.at<Vec3b>(i, j)[2] = colorBalanced[i][j][2];

			//for dcp image
			/*out_img.at<Vec3b>(i, j)[0] = dcp[i][j];
			out_img.at<Vec3b>(i, j)[1] = dcp[i][j];
			out_img.at<Vec3b>(i, j)[2] = dcp[i][j];*/

			//out_img.at<uchar>(Point(i, j)) = (red[i][j]);
		}
	/*for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			cout << dcp[i][j] << " ";
		}
		cout << endl;
	}*/
	cout << rows << " " << cols << " in output function "<< endl;
	//imshow("Output Image", out_img);
}

//Callback function to select image, it simply read the image and calls operation
static void Select_img(int, void*)
{
	
	input_img = imread(imgs_name[img_number], IMREAD_UNCHANGED);

	/*cout << " Rows: " << input_img.rows << endl;
	cout << " Columns: " << input_img.cols << endl;
	cout << " Channels: " << input_img.channels() << endl;*/
	cout << imgs_name[img_number]<<" in select_img"<<endl;
	rows = input_img.rows;
	cols = input_img.cols;
	channels = input_img.channels();
	red.clear();
	green.clear();
	blue.clear();

	red.resize(rows);
	green.resize(rows);
	blue.resize(rows);

	double sumBlue = 0, sumRed = 0, sumGreen = 0;
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

	//cout << "MOS: " << mosValue << endl;

	if (mosValue > 0.006)
		colorCast = 0;

	

	//Converting to image
	//Mat out_img;
	imshow("Input Image", input_img);
	form_output_img();
	imshow("Output Image", out_img);
}

int main() {


	//store the path to image folders
	string img_path = "images";

	//Read images
	imgs_name = read_images(img_path);
	int max_images = imgs_name.size();



	//creating windows
	namedWindow("Input Image", WINDOW_AUTOSIZE);
	namedWindow("Output Image", WINDOW_AUTOSIZE);

	//Create the required trackbars		
	createTrackbar("Select images", "Input Image", &img_number, max_images - 1, Select_img);
	
	Select_img(0, 0);
	//Showing the images
	
	waitKey();
	return 0;
}
