#include"roi.h"
#include"originalVibe.h"
#include<iostream>
using namespace std;
#include<opencv2\contrib\contrib.hpp>
#include<opencv2\objdetect\objdetect.hpp>
using namespace cv;
/*--------------------------------定义鼠标事件--画直线--------------------------*/
bool got_line = false;
//全局变量
Point beginPoint=Point(0,0);//--注意这个有一个初始化的(0,0)
bool got_beigin_point = false;
Point endPoint=Point(0,0);//--注意这个有一个自己默认的初始化(0,0)
void mouseLineHandler(int event, int x, int y, int flags, void *param)
{
	switch(event)
	{
	case CV_EVENT_LBUTTONDOWN:
		beginPoint = Point(x,y);
		endPoint = beginPoint;
		got_beigin_point = true;
		break;
	case   CV_EVENT_MOUSEMOVE:
		if(got_beigin_point)
		{
			endPoint = Point(x,y);
		}
		break;
	case CV_EVENT_LBUTTONUP:
		got_line = true;
		endPoint = Point(x,y);
		break;
	default:
		break;
	}
}
/*---------------------------------------------------------------------------------------------*/
//分析找到的框的

/*------筛选条件的参数-------*/
int areaThresh = 120*120;
const int MAX_AREATHRESH = 150*150;
int ratioThresh = 2;
const int MAX_RATIOTHRESH = 10;


//保存上一帧和当前帧的vehicleRect
Rect previousVehicleRect;
Rect currentVehicleRect;

//用于相同的VehicleRect计数
int numberFrame_EqualVehicleRect = 0;
int thresh_numberFrame_EqualVehicleRect = 5;
int MAX_NF_EV = 10;

bool currentVehicleRectBool = false;
bool previousVehicleRectBool = false;
int numberVehicleRect = 0;

int numberVehicleRectFrame =1;
int vehicleFrequency = 3;
int currentVehicleNumber;
int previousVehicleNumber;

//比例
int areaScale = 8;
const int MAX_AREASCALE = 10;

int main(int, char*argv[])
{
	/*视频流的输入*/
	VideoCapture cap(argv[1]);
	if(!cap.isOpened()) 
		return -1;
	/*视频帧图像*/
	Mat frame;
	/*创建vibe背景建模的对象*/
	OriginalVibe vibe(20,2,20,16,3,3);
	int number =0;
	/*背景建模*/
	Mat seg;
	/*拌线*/
	vector<Point> picketLine;
	picketLine.clear();

	 //register 
	 namedWindow("video",1);
	 setMouseCallback("video",mouseLineHandler,NULL);
	/*-------------------------------20160605--------------------------------------*/
	createTrackbar("面积:","video",&areaThresh,MAX_AREATHRESH,NULL);
	createTrackbar("长\\宽比:","video",&ratioThresh,MAX_RATIOTHRESH,NULL);

	//判断运动物体是否为车的方法
	vector<string> models_filenames;
	models_filenames.clear();
	models_filenames.push_back("car.xml");
	LatentSvmDetector detector(models_filenames);
	vector<LatentSvmDetector::ObjectDetection>  objects;
	objects.clear();
	
	for(;;)
	{
		cap >> frame;
		if(! frame.data)
			break;

		//一些筛选条件的选择	
		number++;
		if(number == 1)
		{
			/*--------------画出地感线:得到起始点------------------*/
			Mat first;
			frame.copyTo(first);
			while(!got_line)
			{
				first.copyTo(frame);
				line(frame,beginPoint,endPoint,Scalar(0,255,255),2);
				imshow("video",frame);
				if(waitKey(50) == 'q')
					break;
			}
			//remove callback
			setMouseCallback("video",NULL,NULL);
			//初始化vide算法
			vibe.originalVibe_Init_BGR( frame);
			line(frame,beginPoint,endPoint,Scalar(0,255,0),2);
			//得出警戒线
			picketLine.push_back(beginPoint);
			picketLine.push_back(endPoint);
			continue;
		}
		else
		{
			//更新vibe算法
			vibe.originalVibe_ClassifyAndUpdate_BGR(frame,seg);
	
			line(frame,beginPoint,endPoint,Scalar(0,255,0),2);
			
			//均值滤波
			medianBlur(seg,seg,5);
			//开闭运算
			
			//找区域
			vector< vector<Point> > contours;
			vector<Vec4i> hierarchy;
			findContours( seg, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );//CV_RETR_TREE
			vector< vector<Point> > contours_poly( contours.size() );
		
			/*存储运动物体*/
			vector<Rect> boundRect;
			boundRect.clear();

			//对视频中出现的运动物体，进行初次的筛选
			for(int index = 0;index < contours.size() ;index++)
			{
				approxPolyDP( Mat(contours[index]), contours_poly[index], 3, true );
				Rect rect =  boundingRect( Mat(contours_poly[index]) );
				//rectangle(frame,rect,Scalar(255,0,0),2);
				if( rect_line_intersection(picketLine,rect) )
				{
					if( rect.area() > areaThresh && (float(rect.height) /float(rect.width) <= 2 && float(rect.width) /float(rect.height) <= 2))
					boundRect.push_back(rect);
				}
				
			}


			//对筛选过后的车辆，进行合并：得到的currentVehicleRect,一定是和拌线相交的
			for(unsigned int index = 0;index < boundRect.size() ;index++)
			{
				if(index == 0)
					currentVehicleRect = boundRect[index];
				else
					currentVehicleRect = currentVehicleRect | boundRect[index];
			}

			//如果boundRect.size() != 0 ，代表这一帧有车辆经过
			if( boundRect.size() != 0)
			{
				currentVehicleRectBool = true;

				if( previousVehicleRectBool == false)
				{
					//代表这是第几辆车
					numberVehicleRect++;

					//记录当前帧的车辆的标号
					currentVehicleNumber = numberVehicleRect;

					//归零
					numberFrame_EqualVehicleRect = 0;
				}

				//为了防止遗留下来的固定的Rect
				if( currentVehicleRect == previousVehicleRect)
				{
					numberFrame_EqualVehicleRect ++;
					
					if( numberFrame_EqualVehicleRect >= thresh_numberFrame_EqualVehicleRect)
					{
						currentVehicleRectBool = false;

						//用来记录当前车的标号(第几辆车)的帧数
						numberVehicleRectFrame = 1;
					}
				}
				else
				{
					//归零
					numberFrame_EqualVehicleRect = 0;
				}

				//用于计数，该辆车出现的连续帧数
				if( currentVehicleNumber == previousVehicleNumber )
				{
					numberVehicleRectFrame++;

					if( numberVehicleRectFrame == vehicleFrequency)
					{
						//为了防止车辆挡住摄像头(第一：用面积，第二：用汽车的宽度和frame的宽度)
						if( currentVehicleRect.area() <= float(areaScale)/float(MAX_AREASCALE)*frame.rows * frame.cols || currentVehicleRect.width < 0.9*frame.cols)
						{
							//cout << "抓拍"<<endl;
							imshow("抓拍",frame);
						}
					}
					/*------------------------------------------------------------------------------*/
				}

			}
			else
			{
				currentVehicleRectBool = false;

				//用来记录当前车的标号(第几辆车)的帧数
				numberVehicleRectFrame = 1;
			}


			//在frame中画出汽车
			rectangle(frame,currentVehicleRect,Scalar(255,255,0),2);

			//把当前帧的汽车保存到上一帧中
			previousVehicleRect = currentVehicleRect;
			previousVehicleNumber = currentVehicleNumber;
			previousVehicleRectBool = currentVehicleRectBool;

			imshow("video",frame);
			if(waitKey(100) >= 0) 
				break;
		}
	}
return 0;
}
