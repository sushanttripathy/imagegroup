#include <cctype>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdio.h>
#include <string>
#include <map>
#include <time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/objdetect/objdetect.hpp>
//#include <opencv2/ml/ml.hpp>
//#include <opencv2/nonfree/features2d.hpp>

#include <boost/filesystem.hpp>
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/serialization/map.hpp>
//#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace cv;
using namespace boost;


typedef std::map <std::string, std::string> FILE_MAP, *ptrFILE_MAP;
typedef FILE_MAP::iterator itFILE_MAP;

typedef std::vector<std::string> PATH_LIST, *ptrPATH_LIST;
typedef PATH_LIST::iterator itPATH_LIST;

void draw_box(Mat &img, Rect r);
void my_mouse_callback( int event, int x, int y, int flags, void* param );

void rotate(cv::Mat& src, double angle, cv::Mat& dst);

bool destroy=false;
Rect box;
bool drawing_box = false;


void options();
void help();


int ProcessImageGroups(string input_path, string output_path, int recurse_dirs, PATH_LIST &images, FILE_MAP &file_map);
int GroupImage(string image_path, string output_path, PATH_LIST &images, FILE_MAP &file_map);

int main (int argc, const char *argv[])
{
    if(argc < 3)
    {
        options();
    }
    else
    {
        std::cout << "Proceeding to verifying and reading directories." << std::endl;
        std::cout << " Press [s] key for help." << std::endl;
        std::cout << std::endl;

        FILE_MAP fmap;
        PATH_LIST images;
        namedWindow( "Image", CV_WINDOW_AUTOSIZE );

        ProcessImageGroups(argv[1], argv[2], 1, images, fmap);
    }

    return 0;
}

void options()
{
    std::cout << " Usage: imggrp <input path> <output path>\n" << std::endl;
    std::cout << " This program will go through all images it finds in the <input path>" << std::endl
              << " and will allow the user to rotate (90 degree increments) and flip" << std::endl
              << " (horizontal and vertical) the images, before saving them to a" << std::endl
              << " particular sub-directory under <output path>." << std::endl
              << " Output sub-directory can be chosen by pressing numbers 0-9." << std::endl
              << " Original files in the <input path> are not modified." << std::endl << std::endl
              << " Note: Currently gif file format is not supported."
              << std::endl;
}

void help()
{
    std::cout << std::endl
              << " Help key pressed." << std::endl << std::endl;
    std::cout << " Press [a] key to move back to the previous image." << std::endl
              << " Press [d] key to move forward to the next image." << std::endl
              << " Press [r] key to rotate the image clockwise in 90 degree increments." << std::endl
              << " Press [h] key to flip the image horizontally." << std::endl
              << " Press [v] key to flip the image vertically." << std::endl
              << " Press keys from [0-9] to save into the output sub-directory [0-9]." << std::endl
              << " Press [Esc] to quit at any give time." << std::endl
              << std::endl
              << " Note: If you move ([a]/[d]) without saving, modifications will be lost" << std::endl << std::endl;
}

string MakePath(string dir, string filename)
{
    string slash_ = "/";
    boost::filesystem::path slash(slash_);
    string preferredSlash = slash.make_preferred().string();

    char path[1024];
    sprintf(path, "%s%s%s", dir.c_str(), preferredSlash.c_str(), filename.c_str());
    string ret = path;
    return ret;
}

int ProcessImageGroups(string input_path, string output_path, int recurse_dirs, PATH_LIST &images, FILE_MAP &file_map)
{
    boost::filesystem::path inpath(input_path.c_str());
    boost::filesystem::path outpath(output_path.c_str());

    boost::filesystem::directory_iterator end_iter;

    PATH_LIST dirs;

    if(boost::filesystem::exists(inpath) && boost::filesystem::is_directory(inpath))
    {
        if(boost::filesystem::exists(outpath) && boost::filesystem::is_directory(outpath))
        {
            for( boost::filesystem::directory_iterator dir_iter(inpath) ; dir_iter != end_iter ; ++dir_iter)
            {
                if (boost::filesystem::is_regular_file(dir_iter->status()) )
                {
                    Mat img_raw = imread(dir_iter->path().string(), 1);

                    if(!img_raw.empty())
                    {
                        std::cout << "Successfully read " << (*dir_iter) << "...\n" << std::endl;
                        //add image path to path list
                        images.push_back(dir_iter->path().string());

                        GroupImage(dir_iter->path().string(), output_path, images, file_map);
                    }
                }
                else if(boost::filesystem::is_directory(dir_iter->status()))
                {

                    if(recurse_dirs)
                    {
                        dirs.push_back(dir_iter->path().string());
                    }
                }
            }
        }
        else
        {
            std::cout << " Output path specified doesn't exist or is not a directory.\n" << std::endl;
        }
    }
    else
    {
        std::cout << " Input path specified doesn't exist or is not a directory.\n" << std::endl;
    }

    if(dirs.size())
    {
        for(itPATH_LIST dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it)
        {
            ProcessImageGroups(*(dir_it), output_path, recurse_dirs, images, file_map);
        }
    }

    return 0;
}

int GroupImage(string image_path, string output_path, PATH_LIST &images, FILE_MAP &file_map)
{
    int done = false;
    int cur_pos = 0;
    int destroy_cropped = false;

    unsigned int modification = 0;

    Mat modified;
    Mat input;

    double angle = 0.0;
    int flip_x = 0;
    int flip_y = 0;

    static string last_image_path  = "";
    static char last_key = 0;

    do
    {
        Mat res_input;
        int r = 0, c = 0;
        box = Rect(0,0,0,0);
        Mat roi;

        if(!modification)
        {
            if(last_image_path != image_path)
                std:cout << "Processing : " << image_path << std::endl;



            input = imread(image_path, 1);

            if(last_key == 's' || last_key == 'S')
                std::cout << " Current image is : " << image_path << std::endl;

            std::cout << " Image size: " << input.size() << std::endl;

            last_image_path = image_path;

            angle = 0.0;
            flip_x = 0;
            flip_y = 0;

            if(input.rows > 640 || input.cols > 640)
            {
                if(input.rows > input.cols)
                {
                    r = 640;
                    c = (int)cvRound((float)input.cols*((float)640/(float)input.rows));
                }
                else if(input.cols >= input.rows)
                {
                    r = (int)cvRound((float)input.rows*((float)640/(float)input.cols));
                    c = 640;
                }
            }

            if(r && c)
            {
                resize(input, res_input, Size(c,r));
                cv::imshow( "Image", res_input );
            }
            else
            {
                cv::imshow( "Image", input );
            }
        }
        else
        {
            std::cout << " Modified image size : " << modified.size() << std::endl;
            if(modified.rows > 640 || modified.cols > 640)
            {
                if(modified.rows > modified.cols)
                {
                    r = 640;
                    c = (int)cvRound((float)modified.cols*((float)640/(float)modified.rows));
                }
                else if(modified.cols >= modified.rows)
                {
                    r = (int)cvRound((float)modified.rows*((float)640/(float)modified.cols));
                    c = 640;
                }
            }

            if(r && c)
            {
                resize(modified, res_input, Size(c,r));
                cv::imshow( "Image", res_input );
            }
            else
            {
                cv::imshow( "Image", modified );
            }
        }


        char k = cv::waitKey(0);

        last_key = k;


        if(destroy_cropped)
        {
            cv::destroyWindow("Cropped");
            destroy_cropped = false;
        }



        switch(k)
        {
        move_forward:
        case 'd':
        case 'D':
            //Move forward
            if(modification)
                std::cout << "Discarding modifications!" << std::endl;
            std::cout << std::endl;
            modification = 0;
            if(cur_pos < 0)
            {
                cur_pos++;
                if(images.size())
                {
                    int i = images.size() + cur_pos - 1;

                    if(i < images.size())
                    {
                        image_path = images[i];
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }

            break;

        move_backward:
        case 'a':
        case 'A':
            //move backward
            if(modification)
                std::cout << "Discarding modifications!" << std::endl;
            std::cout << std::endl;
            modification = 0;
            cur_pos--;
            if(images.size())
            {
                int i = images.size() + cur_pos - 1;

                if(i < images.size())
                {
                    image_path = images[i];
                }
                else
                {
                    cur_pos = 0;
                }
            }
            else
            {
                cur_pos = 0;
            }
            break;

        case 'r':
        case 'R':
            //rotate 90 degrees
            {
                std::cout << "Rotation 90 degrees :";// << std::endl;
                Mat source;
                if(modification)
                {
                    //use modified Mat instead of input
                    source = modified;
                }
                else
                {
                    source = input;
                }
                modification |= 0x01;
                angle += 90.0;
                angle = angle - ((double)((long)(angle/360.0)))*360;

                double cur_angle = 90.0;
                rotate(source, cur_angle, modified);
                //cv::imshow("Image", modified);

                if(angle == 0.0)
                    modification &= ~(0x01);
            }
            break;

        case 'h':
        case 'H':
            //flip about x
            {
                std::cout << "Horizontal flip :";// << std::endl;
                Mat source;
                if(modification)
                {
                    //use modified Mat instead of input
                    source = modified;
                }
                else
                {
                    source = input;
                }
                modification |= 0x02;
                flip_x++;
                cv::flip(source, modified, 0);

                //cv::imshow("Image", modified);

                if(flip_x % 2 == 0)
                    modification &= ~(0x02);
            }
            break;

        case 'v':
        case 'V':
            //flip about y
            {
                std::cout << "Vertical flip :";// << std::endl;
                Mat source;
                if(modification)
                {
                    //use modified Mat instead of input
                    source = modified;
                }
                else
                {
                    source = input;
                }
                modification |= 0x04;
                flip_y++;
                cv::flip(source, modified, 1);

                //cv::imshow("Image", modified);

                if(flip_y % 2 == 0)
                    modification &= ~(0x04);
            }
            break;



        case 'c':
        case 'C':
            {
                box = Rect (0,0,0,0);
                Rect prevBox = box;
                Mat show;
                if(r && c)
                {
                    show = res_input.clone();
                }
                else
                {
                    show = input.clone();
                }
                cv::destroyWindow("Image");
                cv::namedWindow("Image");
                cv::imshow("Image", show);
                cvSetMouseCallback("Image", my_mouse_callback, &show );
                destroy = false;
                do
                {
                    if(box.width >= 0 && box.height >= 0 && box.x >= 0 && box.y >= 0)
                    {

                        if(r && c)
                        {
                            roi = res_input(box);
                        }
                        else
                        {
                            roi = input(box);
                        }
                    }
                    else
                    {
                        roi.create(0, 0, CV_32FC1);
                    }

                    if(!roi.empty())
                    {
                        if(box != prevBox)
                        {
                            prevBox = box;
                            if(destroy_cropped)
                            {
                                cv::destroyWindow("Cropped");
                                destroy_cropped = false;
                            }
                            cv::namedWindow("Cropped");
                            cv::imshow("Cropped", roi);
                            destroy_cropped = true;

                            if(r && c)
                            {
                                show = res_input.clone();
                            }
                            else
                            {
                                show = input.clone();
                            }


                            std::cout << "Drawing box : (" << box.x << ", " << box.y << ", "
                            << box.width << ", " << box.height << ")\n" << std::endl;
                            draw_box(show, box);
                            cv::imshow("Image",show);
                        }
                    }

                    if(waitKey(30) == 27)
                        exit(0);
                }while(!destroy);
            }
            break;

        case 's':
        case 'S':
            help();
            break;

        case 'j':
        case 'J':
            {
                int num_steps = 0;
                std::cout << " Please enter the file number in sequence : ";
                std::cin >> num_steps;
            }
            break;

        case 27:
            std::cout << "Escape key pressed, terminating program." << std::endl;
            exit(0);
            break;

        default:
            {
                if(k >= '0' && k <= '9')
                {
                    char fold_name[12];
                    sprintf(fold_name, "%c", k);
                    string cur_output_path = MakePath(output_path, fold_name);

                    if(!boost::filesystem::exists(cur_output_path.c_str()))
                    {
                        std::cout << "Making destination directory : " << cur_output_path << "\n" << std::endl;
                        boost::filesystem::create_directory(cur_output_path.c_str());
                    }

                    itFILE_MAP imprev_path = file_map.find(image_path);

                    if(imprev_path != file_map.end())
                    {
                        string dest_to_del = imprev_path->second;

                        std::cout << "Deleting image from " << dest_to_del << "\n" << std::endl;

                        boost::filesystem::remove(dest_to_del.c_str());

                        file_map.erase(imprev_path);
                    }

                    std::cout << "Adding " << image_path << " to " << cur_output_path << "\n" << std::endl;


                    boost::filesystem::path impath(image_path);

                    string imfilename = impath.filename().string();

                    string cur_dest_path  = MakePath(cur_output_path, imfilename);

                    file_map.insert(std::pair<std::string, std::string>(image_path, cur_dest_path));

                    std::string temp_str = "";
                    temp_str += k;

                    namedWindow(temp_str, 0);
                    resizeWindow(temp_str,80,80);

                    if(!modification)
                    {
                        boost::filesystem::copy_file(image_path.c_str(), cur_dest_path.c_str(),  boost::filesystem::copy_option::overwrite_if_exists);
                        imshow(temp_str, input);
                    }
                    else
                    {
                        imwrite(cur_dest_path, modified);
                        modification = 0;
                        imshow(temp_str, modified);
                    }


                    if(cur_pos == 0)
                    {
                        return 0;
                    }
                    else
                    {
                        //cur_pos++;
                        goto move_forward;
                    }
                }
            }
            break;
        }

    }while(!done);
    return 0;
}

void draw_box(Mat &img, Rect r)
{
    const static Scalar colors[] =
             {  CV_RGB(0,0,255),
                CV_RGB(0,128,255),
                CV_RGB(0,255,255),
                CV_RGB(0,255,0),
                CV_RGB(255,128,0),
                CV_RGB(255,255,0),
                CV_RGB(255,0,0),
                CV_RGB(255,0,255)} ;

    int i = 0;//rand();
    Scalar color = colors[i%8];

    rectangle( img, cvPoint(cvRound(r.x), cvRound(r.y)),
                               cvPoint(cvRound((r.x + r.width-1)), cvRound((r.y + r.height-1))),
                               color, 3, 8, 0);

  //std::cout << "Drawing rectangle\n" << std::endl;
  //cvSetImageROI(image, rect2);   //here I wanted to set the drawn rect as ROI
}

// Implement mouse callback
void my_mouse_callback( int event, int x, int y, int flags, void* param )
{

  //`Mat *img = (Mat *)param;
  switch( event )
  {
      case CV_EVENT_MOUSEMOVE:
      {
          if( drawing_box )
          {
              box.width = x-box.x;
              box.height = y-box.y;
          }

          //draw_box((*img), box);
      }
      break;

      case CV_EVENT_LBUTTONDOWN:
      {
          drawing_box = true;
          box = Rect( x, y, 0, 0 );

          //draw_box((*img), box);
      }
      break;

      case CV_EVENT_LBUTTONUP:
      {
          drawing_box = false;
          if( box.width < 0 )
          {
              box.x += box.width;
              box.width *= -1;
          }

          if( box.height < 0 )
          {
              box.y += box.height;
              box.height *= -1;
          }

          //draw_box((*img), box);

      }
      break;

      case CV_EVENT_RBUTTONUP:
      {
          destroy=true;
      }
      break;

      default:
      break;
   }
}


void rotate(cv::Mat& src, double angle, cv::Mat& dst)
{
    int len = std::max(src.cols, src.rows);
    int diff = len - std::min(src.cols, src.rows);
    //Mat temp = SquareResizeImageWithPadding(src, len);

    double diff_x = 0;
    double diff_y = 0;


    Mat rotated;
    int x = src.cols;
    int y = src.rows;
    cv::Point2f pt(x/2., y/2.);//
    cv::Point2f pt2((len-1)/2., (len-1)/2.);
    cv::Mat r = cv::getRotationMatrix2D(pt2, angle, 1.0);

    int h = y;
    int w = x;

    if(((long)angle) % 180 == 90)
    {
        h = x;
        w = y;
    }

    double ci_x = 0;
    double ci_y = 0;

    if(angle == 90.)
    {
        if(src.cols > src.rows)
        {
            ci_x = (y - 1)/2.;
            ci_y = (x - 1)/2.;
        }
        else
        {
            ci_x = (y - 1)/2.;
            ci_y = (double)diff + (x - 1)/2.;
        }

    }
    else if (angle == 180.)
    {
        if(src.cols > src.rows)
        {
            ci_x = (x - 1)/2.;
            ci_y = (double)diff + (y - 1)/2.;
        }
        else
        {
            ci_x = (double)diff + (x - 1)/2.;
            ci_y = (y - 1)/2.;
        }

    }
    else if (angle == 270.)
    {
        if(src.cols > src.rows)
        {
            ci_x = (double)diff + (y - 1)/2.;
            ci_y = (x - 1)/2.;
        }
        else
        {
            ci_x = (y - 1)/2.;
            ci_y = (x - 1)/2.;
        }
    }

    cv::Point2f pt3(ci_x, ci_y);
    cv::warpAffine(src, rotated, r, cv::Size(len, len));
    cv::getRectSubPix(rotated, cv::Size(w,h), pt3, dst);
    //dst = rotated;
}


