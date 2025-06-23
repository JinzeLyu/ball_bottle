#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

using namespace cv;
using std::string;
using std::vector;

/* ---------- HSV 阈值（可通过点击实时查看） ---------- */
struct ColorRange { Scalar lower, upper; };
ColorRange RED_LO{{  0,120, 50}, { 10,255,255}};
ColorRange RED_HI{{160,120, 50}, {180,255,255}};
ColorRange BLUE_CO{{ 90, 60, 30}, {130,255,255}};

Mat g_hsv;                // 保存当前帧的 HSV，用于点击读取
Vec3b g_lastHSV = Vec3b(0, 0, 0);
/* ---------- 工具函数 ---------- */
static Point contourCenter(const vector<Point>& c)
{
    Moments m = moments(c);
    return { (int)(m.m10/(m.m00+1e-5)), (int)(m.m01/(m.m00+1e-5)) };
}
static double contourCircularity(const vector<Point>& c)
{
    double a = contourArea(c), p = arcLength(c,true);
    return (p==0)?0:4*CV_PI*a/(p*p);
}

/* ---------- 鼠标回调：左键点击取 HSV ---------- */
void mouseCallback(int event,int x,int y,int,void*)
{
    if(event==EVENT_LBUTTONDOWN && !g_hsv.empty())
    {
        Vec3b hsv = g_hsv.at<Vec3b>(y,x);
        g_lastHSV = hsv;
        std::cout << "[HSV] (" << hsv[0] << ", " << hsv[1] << ", " << hsv[2] << ")\n";
    }
}

int main(int argc,char** argv)
{
    VideoCapture cap;  (argc>1)?cap.open(argv[1]):cap.open(0);
    if(!cap.isOpened()){ std::cerr<<"Error: cannot open camera/video\n"; return -1; }

    namedWindow("Detection",WINDOW_AUTOSIZE);
    setMouseCallback("Detection", mouseCallback);

    Mat frame, hsv, mRed, mBlue, tmp;
    while(cap.read(frame))
    {
        /* 1. 颜色空间转换 & 缓存 */
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        g_hsv = hsv;                            // 供鼠标取色使用

        /* 2. 掩膜生成 */
        inRange(hsv, RED_LO.lower, RED_LO.upper, mRed);
        inRange(hsv, RED_HI.lower, RED_HI.upper, tmp);  bitwise_or(mRed,tmp,mRed);
        inRange(hsv, BLUE_CO.lower, BLUE_CO.upper, mBlue);

        morphologyEx(mRed, mRed, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE,{5,5}));
        morphologyEx(mBlue,mBlue,MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE,{5,5}));

        /* 3. 轮廓检测 + 可视化 */
        auto process=[&](Mat&m,const string&lab,Scalar col,bool isBall){
            vector<vector<Point>> cs; findContours(m,cs,RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);
            for(auto& c:cs){
                if(contourArea(c)<800) continue;
                Rect box = boundingRect(c);
                if(isBall && contourCircularity(c)<0.7) continue;
                if(!isBall && (double)box.height/box.width<1.3) continue;
                Point ctr = contourCenter(c);
                rectangle(frame,box,col,2);
                drawMarker(frame,ctr,col,MARKER_CROSS,15,2);
                putText(frame,lab+"("+std::to_string(ctr.x)+","+std::to_string(ctr.y)+")",
                        {box.x,box.y-8},FONT_HERSHEY_SIMPLEX,0.55,col,2);
            }
        };
        process(mRed,"ball",  Scalar(0,0,255), true);
        process(mBlue,"bottle",Scalar(255,0,0),false);

        /* 4. 叠加最近一次点击 HSV 值 */
        Vec3b hsvV = g_lastHSV;
        string hsvText = "HSV: ("+std::to_string(hsvV[0])+","+std::to_string(hsvV[1])+","+std::to_string(hsvV[2])+")";
        putText(frame,hsvText,{10,25},FONT_HERSHEY_SIMPLEX,0.7,Scalar(0,255,0),2);

        imshow("Detection",frame);
        if(waitKey(1)=='q') break;
    }
    return 0;
}
