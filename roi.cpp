#include"roi.h"
//判断两条线段相交
bool intersection(const vector<Point> & line1, const vector<Point> &line2)//vector<Point> line(2),代表一线段
{
	CV_Assert(line1.size() == line2.size());
	CV_Assert(line1.size()==2);
	Point point1_11,point1_12,point1_21,point1_22;
	//首先判断line1的两个端点,在line2的两侧
	point1_11 = line2[0] - line1[0];
	point1_12 = line2[1]-line1[0];

	point1_21 = line2[0]-line1[1];
	point1_22 = line2[1]-line1[1];

	//point1_11.cross(point1_12)*point1_21.cross(point1_22)<0;//----------表明在两侧
	//再次判断line2的两个端点，在line1的两侧
	Point point2_11,point2_12,point2_21,point2_22;

	point2_11 = line1[0]-line2[0];
	point2_12 = line1[1] - line2[0];
	
	point2_21 = line1[0] -line2[1];
	point2_22 = line1[1]-line2[1];
	
	//point2_11.cross(point2_12)*point2_21.cross(point2_22)<0;
	return ( point1_11.cross(point1_12)*point1_21.cross(point1_22)<0 && point2_11.cross(point2_12)*point2_21.cross(point2_22)<0 );
}

//判断矩形和一条线段相交(线段只要与矩形的一条边相交，就可以判定相交 或者 线段在矩形内部)
bool rect_line_intersection(const vector<Point> & line,const Rect & targetRect)//rect的四个顶点(roi.x,roi.y),(roi.x,roi.y+roi.height-1),(roi.x+roi.width-1,roi.y),(roi.x+roi.width-1,roi.y+roi.height-1)
{
	CV_Assert(line.size()==2);
	//先判断第一种情况：线段在矩形内部
	if(line[0].inside(targetRect) && line[1].inside(targetRect))
		return true;
	//再判断第二种情况,线段和矩形的至少一条边相交
	//---第一步：提取矩形的四条边
	vector<Point> line1;
	line1.clear();
	line1.push_back(Point(targetRect.x,targetRect.y));
	line1.push_back(Point(targetRect.x,targetRect.y+targetRect.height-1));
	if(intersection(line1,line))
		return true;
	vector<Point> line2;
	line2.clear();
	line2.push_back(Point(targetRect.x+targetRect.width-1,targetRect.y));
	line2.push_back(Point(targetRect.x+targetRect.width-1,targetRect.y+targetRect.height -1));
	if( intersection(line2,line))
		return true;
	vector<Point> line3;
	line3.clear();
	line3.push_back(Point(targetRect.x,targetRect.y));
	line3.push_back(Point(targetRect.x+targetRect.width-1,targetRect.y));
	if(  intersection(line3,line))
		return true;
	vector<Point> line4;
	line4.clear();
	line4.push_back(Point(targetRect.x,targetRect.y+targetRect.height-1));
	line4.push_back(Point(targetRect.x+targetRect.width-1,targetRect.y+targetRect.height-1));
	if(  intersection(line2,line) )
		return true;
	//return ( intersection(line1,line) || intersection(line2,line)||intersection(line3,line)||intersection(line4,line) );
	return false;
}

//以Rect的中心点为基础,同比例缩放Rect
