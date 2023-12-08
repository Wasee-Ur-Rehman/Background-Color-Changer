#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include "tinyfiledialogs.h" // tiny file for file dialogs
#define TINYFD_IMPLEMENTATION
#define TINYFD_ALLOWMULTISELECT
#include <string>
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include<thread>
#include <fstream>

#define DELETE_KEY 8
#define ENTER_KEY 13
#define ESCAPE_KEY 27
#define SHIFT_KEY 16
#define SPACE_KEY 32
#define Z_KEY 90
#define X_KEY 88
#define C_KEY 67
#define W_KEY 87 // }-> All of these may not be used, but roughly they will be needed
#define A_KEY 65
#define S_KEY 83
#define D_KEY 68
#define UP_KEY 72
#define DOWN_KEY 80
#define LEFT_KEY 75
#define RIGHT_KEY 77
#define CTRL_KEY 17
#define TAB_KEY 9
#define Backspace_KEY 8


using namespace std;// iostream
using namespace sf;// sfml
using namespace cv;// opencv

Vec3b targetColor;
bool isProcessingDone = false;
RenderWindow process;
int* InputColor();

void processing()
{
    // Create a separate thread for the window display
    std::thread windowThread([&]()
    {
         //just load the vidoe and run it untill the video ends
        VideoCapture cap("pro.mp4");
        Mat frame;
        if (!cap.isOpened())
        {
			cout << "Error opening video stream or file" << endl;
			return;
		}
        while (1)
        {
			cap >> frame;
			if (frame.empty())
				break;
			imshow("Processing", frame);
			waitKey(30);
            if (isProcessingDone)
            {
                break;
            }
		}
		destroyWindow("Processing");
    });
    // Detach the thread so it runs independently 
    windowThread.detach();
}

void colorPicker(int event, int x, int y, int flags, void* imgptr) // for color picker
{
    // Cast the param to Mat pointer
    Mat* image = reinterpret_cast<Mat*>(imgptr);

    if (event == EVENT_LBUTTONDOWN)
    {
        targetColor = (*image).at<Vec3b>(Point(x, y));  
    }
}

void processImage(const string& imagePath, const Scalar& backgroundColor)
{
    // Load the image
    Mat image = imread(imagePath);

    // Check if the image is loaded successfully
    if (image.empty())
    {
        cerr << "Error: Could not load the image: " << imagePath << endl;
        return;
    }

     //Convert the image to grayscale for contour detection
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);

    // Apply thresholding to create a binary image
    Mat thresholded;
    threshold(gray, thresholded, 127, 255, THRESH_BINARY_INV);

    // Find contours in the binary image
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(thresholded, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

   //  Create a mask for the background
    Mat backgroundMask = Mat::zeros(image.size(), CV_8UC1);
    drawContours(backgroundMask, contours, -1, Scalar(255), FILLED);

    // Replace the target color with the background color only outside the detected object
    for (int i = 0; i < image.rows; ++i)
    {
        for (int j = 0; j < image.cols; ++j)
        {
            if (backgroundMask.at<uchar>(i, j) <= 0)
            {
                Vec3b& pixel = image.at<Vec3b>(i, j);
                //allow for range
                if (pixel[0] >= targetColor[0] - 3 && pixel[0] <= targetColor[0] + 3 &&
                    pixel[1] >= targetColor[1] - 3 && pixel[1] <= targetColor[1] + 3 &&
                    pixel[2] >= targetColor[2] - 3 && pixel[2] <= targetColor[2] + 3)
                {
                    pixel[0] = backgroundColor[0];
                    pixel[1] = backgroundColor[1];
                    pixel[2] = backgroundColor[2];
                }
            }
        }
    }

    isProcessingDone = true;
   //  Display the image after color replacement
    namedWindow("Image with Color Replacement", WINDOW_NORMAL);
    imshow("Image with Color Replacement", image);
    waitKey(0);

    const char* lFilterPatterns[3] = { "*.jpg", "*.png", " *.jpeg" };
    
    char const* savePath = tinyfd_saveFileDialog(
        "Choose a directory to save the image",
        "result",  // Default file path
        1,  // Number of filter patterns
        lFilterPatterns,
        NULL);

    if (!savePath) {
        cerr << "No directory was selected." << endl;
        isProcessingDone = false;
        return;
    }
   //  Save the image to the selected directory
    string outputImagePath = string(savePath);  // Add '_result' to the file name
    imwrite(outputImagePath, image);
    cout << "Saved the image: " << outputImagePath << endl;
    isProcessingDone = false;
}

class TextBox // Selecting textbox using Enter key
{
private:
    Text textbox;
    ostringstream text;
    bool isSelected = false;
    bool haslimit = false;
    int limit;

    void inputLogic(int charTyped)
    {
        if (charTyped != DELETE_KEY && charTyped != ENTER_KEY && charTyped != ESCAPE_KEY && charTyped != TAB_KEY)
        {
            text << static_cast<char>(charTyped);
        }

        else if (charTyped == DELETE_KEY)
        {
            if (text.str().length() > 0)
            {
                deleteLastChar();
            }
        }
        textbox.setString(text.str() + "_");
    }

    void deleteLastChar()
    {
        string t = text.str();
        string newT = ""; // NewT means New Text
        for (int i = 0; i < t.length() - 1; i++)
        {
            newT += t[i];
        }
        text.str("");
        text << newT;
        textbox.setString(text.str());
    }

public:
    TextBox(int size, Color color, bool sel)
    {
        textbox.setCharacterSize(size);
        textbox.setFillColor(color);
        isSelected = sel;

        if (sel)
        {
            textbox.setString("_");
        }
        else
        {
            textbox.setString("");
        }
    }
    void setFont(Font& font)
    {
        textbox.setFont(font);
    }

    void setPosition(Vector2f pos) // pos means position
    {
        textbox.setPosition(pos);
    }

    void setLimit(bool ToF) // Tof means True or False
    {
        haslimit = ToF;
    }

    void setLimit(bool ToF, int lim)
    {
        haslimit = ToF;
        limit = lim - 1;
    }

    void setSelected(bool sel) // sel means selected
    {
        isSelected = sel;
        if (!sel)
        {
            string t = text.str();
            string newT = "";
            for (int i = 0; i < t.length(); i++)
            {
                newT += t[i];
            }
            textbox.setString(newT);
        }
    }

    string getText()
    {
        return text.str();
    }

    void drawTo(RenderWindow& window)
    {
        window.draw(textbox);
    }

    void typedOn(Event input)
    {
        // if it is not a number, dont type it
        if ((input.text.unicode > 57 || input.text.unicode < 48) && (input.text.unicode != 8 && input.text.unicode != 13))
        {
            return;
        }
        if (isSelected)
        {
            int charTyped = input.text.unicode; // get value of char being typed
            if (charTyped < 128)
            {
                if (haslimit)
                {
                    if (text.str().length() <= limit)
                    {
                        inputLogic(charTyped);
                    }
                    else if (text.str().length() > limit && charTyped == DELETE_KEY)
                    {
                        deleteLastChar();
                    }
                }
                else
                {
                    inputLogic(charTyped);
                }
            }
        }
    }

    string return_text() // to verify login and signup
    {
        string t = text.str();
        // if there is an _ at the end of the string, remove it
        if (t[t.length() - 1] == '_')
        {
            t.erase(t.length() - 1);
        }
        //  if there is a space at end of string remove it
        if (t[t.length() - 1] == ' ')
        {
            t.erase(t.length() - 1);
        }
        return t;
    }
    //  destructors
    ~TextBox()
    {
        //  Object Destroyed
        cout << "textbox object destroyed" << endl;
    }
};
// take image path 
int colorChanger(vector<string> imagePaths)
{
    int* backgroundColor;
    //Process each image in the vector
    for (const string& imagePath : imagePaths)
    {
        backgroundColor = InputColor();
        Scalar background(backgroundColor[2], backgroundColor[1], backgroundColor[0]);
        //   Load the image
        Mat image = imread(imagePath);

        // Check if the image is loaded successfully
        if (image.empty())
        {
            cerr << "Error: Could not load the image: " << imagePath << endl;
            return -1;
        }

        //Create a window for color picking
        namedWindow("Color Picker", WINDOW_NORMAL);
        imshow("Color Picker", image);

        //Set the mouse callback function for color picking
        setMouseCallback("Color Picker", colorPicker, &image);

        //Show a message box to inform the user about the next steps
        tinyfd_messageBox(
            "Instructions",
            "The original image is opened.Click on the pixel to select target color and press ESC button for processing",
            "ok",
            "info",
            1);

        // Wait for the user to pick a color
        while (waitKey(1) != 27);  // Press ESC to exit

        destroyWindow("Color Picker");

        processing();

        processImage(imagePath, background);
    }
}

int colorInpputChange(vector<string> imagePaths, RenderWindow &window)
{
    int* target{}, *backgroundColor{};
    for (const string& imagePath : imagePaths)
    {
        // extract BGR Value and store it in a scalr
        target = InputColor();
        window.close();
        backgroundColor = InputColor();
        window.close();
        Scalar background(backgroundColor[2], backgroundColor[1], backgroundColor[0]);
       
        targetColor[0] = target[2];
        targetColor[1] = target[1];
        targetColor[2] = target[0];
        //  Load the image
        Mat image = imread(imagePath);

        // Check if the image is loaded successfully
        if (image.empty())
        {
            cerr << "Error: Could not load the image: " << imagePath << endl;
            return -1;
        }
        processing();

        // Process the image with the picked color
        processImage(imagePath, background);
    }
}

int* InputColor()
{
    // new window
    RenderWindow window(VideoMode(550, 250), "Color Input Window");

    Font font;
    font.loadFromFile("Resources/Bukhari Script.ttf");

    // create 3 textboxes
    TextBox textbox1(30, Color::Black, true);
    textbox1.setPosition({ 170, 50 });
    textbox1.setLimit(true, 3);
    textbox1.setFont(font);
    //  selected in false
    textbox1.setSelected(false);

    TextBox textbox2(30, Color::Black, true);
    textbox2.setPosition({ 170, 100 });
    textbox2.setLimit(true, 3);
    textbox2.setFont(font);
    textbox2.setSelected(false);

    TextBox textbox3(30, Color::Black, true);
    textbox3.setPosition({ 170, 150 });
    textbox3.setLimit(true, 3);
    textbox3.setFont(font);
    textbox3.setSelected(false);

    Text text("R = ", font, 30);
    text.setFillColor(Color::Black);
    text.setPosition(70, 50);
    //  crete text G

    Text text2("G = ", font, 30);
    text2.setFillColor(Color::Black);
    text2.setPosition(70, 100);

    Text text3("B = ", font, 30);
    text3.setFillColor(Color::Black);
    text3.setPosition(70, 150);

    //  grey rectangle 128 128 128, width 110, height 45
    RectangleShape rectangle1(Vector2f(200, 45));
    rectangle1.setFillColor(Color(255, 192, 203));
    rectangle1.setPosition(150, 50);

    //  grey rectangle 128 128 128, width 110, height 45
    RectangleShape rectangle2(Vector2f(200, 45));
    rectangle2.setFillColor(Color(255, 192, 203));
    rectangle2.setPosition(150, 100);

    // grey rectangle 128 128 128, width 110, height 45
    RectangleShape rectangle3(Vector2f(200, 45));
    rectangle3.setFillColor(Color(255, 192, 203));
    rectangle3.setPosition(150, 150);

    //text confirm
    Text text4("Confirm", font, 30);
    text4.setFillColor(Color::Black);
    text4.setPosition(400, 200);

    //text Select color to change
    Text text5("Enter RGB Values", font, 30);
    text5.setFillColor(Color::Black);
    text5.setPosition(150, 10);
    int* arr = new int[4];
    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::TextEntered)
            {
                textbox1.typedOn(event);
                textbox2.typedOn(event);
                textbox3.typedOn(event);
            }

            // if textbox area is clicked select it, else deselect it
            if (event.type == Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == Mouse::Left)
                {
                    //  if mouse from position 170 to 200 and 50 to 80
                    if ((event.mouseButton.x > 170 && event.mouseButton.x < 250) && (event.mouseButton.y > 50 && event.mouseButton.y < 95))
                    {
                        cout << "textbox1 selected" << endl;
                        textbox1.setSelected(true);
                        textbox2.setSelected(false);
                        textbox3.setSelected(false);
                    }
                    //  if mouse from position 170 to 200 and 100 to 130
                    else if ((event.mouseButton.x > 170 && event.mouseButton.x < 250) && (event.mouseButton.y > 100 && event.mouseButton.y < 145))
                    {
                        cout << "textbox2 selected" << endl;
                        textbox1.setSelected(false);
                        textbox2.setSelected(true);
                        textbox3.setSelected(false);
                    }
                    // if mouse from position 170 to 200 and 150 to 180
                    else if ((event.mouseButton.x > 170 && event.mouseButton.x < 250) && (event.mouseButton.y > 150 && event.mouseButton.y < 195))
                    {
                        cout << "textbox3 selected" << endl;
                        textbox1.setSelected(false);
                        textbox2.setSelected(false);
                        textbox3.setSelected(true);
                    }
                    else
                    {
                        textbox1.setSelected(false);
                        textbox2.setSelected(false);
                        textbox3.setSelected(false);
                    }
                }
            }

            //if hovering over confirm turn white
            if (event.type == Event::MouseMoved)
            {
                if (event.mouseMove.x > 400 && event.mouseMove.x < 500 && event.mouseMove.y > 200 && event.mouseMove.y < 230)
                {
                    text4.setFillColor(Color::Blue);
                }
                else
                {
                    text4.setFillColor(Color::Black);
                }
            }

            // if confirm is pressed take 3 strings from textboxes and convert them to int
            if (event.type == Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == Mouse::Left)
                {
                    if (event.mouseButton.x > 400 && event.mouseButton.x < 500 && event.mouseButton.y > 200 && event.mouseButton.y < 230)
                    {
                        arr[0] = stoi(textbox3.return_text());
                        arr[1] = stoi(textbox2.return_text());
                        arr[2] = stoi(textbox1.return_text());
                        return arr;
                    }
                }
            }
        }

        //   background color 39, 216, 90
        window.clear(Color(192, 255, 244));
        window.draw(rectangle1);
        window.draw(rectangle2);
        window.draw(rectangle3);
        window.draw(text);
        window.draw(text2);
        window.draw(text3);
        textbox1.drawTo(window);
        textbox2.drawTo(window);
        textbox3.drawTo(window);
        window.draw(text4);
        window.draw(text5);
        window.display();
    }
}


void colorWindow(int * targetColor, int* replacementColor , vector<string> imagePaths)
{
    RenderWindow window(VideoMode(800, 800), "Input Options");
    CircleShape shape(100.f);
    shape.setFillColor(Color::Green);
    
    Texture texture;
    texture.loadFromFile("Resources/Color_Changer.png");
    Sprite background(texture);
    background.setPosition(0, 0);

    Font BK;
    BK.loadFromFile("Resources/Bukhari Script.ttf");
    Text text("Use Color Picker", BK, 30);
    text.setFillColor(Color::Black);
    text.setPosition(292, 360);

    Text text2("Input Color", BK, 30);
    text2.setFillColor(Color::Black);
    text2.setPosition(333, 539);
    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }
            if (event.type == Event::MouseMoved)
            {
                if (event.mouseMove.x > 292 && event.mouseMove.x < 492 && event.mouseMove.y > 360 && event.mouseMove.y < 390)
                {
                    text.setFillColor(Color::Blue);
                }
                else
                {
                    text.setFillColor(Color::Black);
                }
                if (event.mouseMove.x > 333 && event.mouseMove.x < 467 && event.mouseMove.y > 539 && event.mouseMove.y < 569)
                {
                    text2.setFillColor(Color::Blue);
                }
                else
                {
                    text2.setFillColor(Color::Black);
                }
            }
            // if button clicked
            if (event.type == Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == Mouse::Left)
                {
                    if (event.mouseButton.x > 292 && event.mouseButton.x < 492 && event.mouseButton.y > 360 && event.mouseButton.y < 390)
                    {
                        window.close();
                        colorChanger(imagePaths);
                    }
                    else if (event.mouseButton.x > 333 && event.mouseButton.x < 467 && event.mouseButton.y > 539 && event.mouseButton.y < 569)
                    {
                        window.close(); 
                        colorInpputChange(imagePaths,window);
                    }
                }
            }
        }
        window.clear();
        window.draw(background);
        window.draw(text);
        window.draw(text2);
        window.display();
    }
}

int main()
{
    int* targetColor{}, * replacementColor{};
    char const* lFilterPatterns[3] = { "*.jpg", "*.png", "*.jpeg" };
    char const* lTheSaveFileName;
    char const* lMultiSelect;

    sf::RenderWindow window(sf::VideoMode(1520, 855), "Background Changer App");
  
    sf::Texture texture;
    if (!texture.loadFromFile("frontpage2.png"))
        return -1;

    Sprite sprite(texture);
    Text text1,text2;
    Font font1;
    font1.loadFromFile("Resources/Bukhari Script.ttf");
    text1.setFont(font1);
    text1.setString("Select Images");
    text1.setCharacterSize(50);
    text1.setFillColor(Color::Black);
    text1.setPosition(610,400);
    text2.setFont(font1);
    text2.setString("Exit");
    text2.setCharacterSize(50);
    text2.setFillColor(Color::Black);
    text2.setPosition(705, 600);

    vector<string> imagePaths;
    string filePath;
  
    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == Mouse::Left)
                {
                    if (event.mouseButton.x >= 610 && event.mouseButton.x <= 920 && event.mouseButton.y >= 400 && event.mouseButton.y <= 550)
                    {
                        window.clear();
                        window.close();
                        
                        // Open the file dialog for selecting multiple images
                        lTheSaveFileName = tinyfd_openFileDialog(
                            "Choose images",
                            "",
                            3,  // Number of filter patterns
                            lFilterPatterns,
                            NULL,
                            1);  

                        if (lTheSaveFileName)
                        {
                            //Split the selected file paths
                            stringstream ss(lTheSaveFileName);
                            while (getline(ss, filePath, '|'))
                            {
                                imagePaths.push_back(filePath);
                            }
                        }
                        else
                        {
                            printf("File selection canceled.\n");
                        }
                        colorWindow(targetColor, replacementColor, imagePaths);
					}
                    if (event.mouseButton.x >= 608 && event.mouseButton.x <= 900 && event.mouseButton.y >= 600 && event.mouseButton.y <= 750)
                    {
                        window.close();
                    }
                }
            }
            if (event.type == Event::MouseMoved)
            {
                if (event.mouseMove.x >= 610 && event.mouseMove.x <= 920 && event.mouseMove.y >= 400 && event.mouseMove.y <= 500)
                {
					text1.setFillColor(Color::Magenta);
				}
                else
                {
					text1.setFillColor(Color::Black);
				}
                
                if (event.mouseMove.x >= 608 && event.mouseMove.x <= 900 && event.mouseMove.y >= 600 && event.mouseMove.y <= 750)
                {
                    text2.setFillColor(Color::Magenta);
                }
                else
                {
					text2.setFillColor(Color::Black);
				}
            }
        }
      
        window.clear();
        window.draw(sprite);
        window.draw(text1);
        window.draw(text2);
        window.display();
    }
    return 0;
}
