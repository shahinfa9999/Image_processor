
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <filesystem>
using namespace std;

//***************************************************************************************************//
//                                DO NOT MODIFY THE SECTION BELOW                                    //
//***************************************************************************************************//

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>>& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

//***************************************************************************************************//
//                                DO NOT MODIFY THE SECTION ABOVE                                    //
//***************************************************************************************************//


//
// YOUR FUNCTION DEFINITIONS HERE
/*
    Function that darkens the edges of an image.
    * @param filename is the location where the file is stored
    @return a new image with darker edges.
*/
vector<vector<Pixel>> proc1(string filename)
{

    // Call the read_image function and return its result
    vector<vector<Pixel>> image = read_image(filename);
    // Getting the size of the width and heigh pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    // defines a new image withe strucutr pixels and the same size as the orginal image 
    vector<vector<Pixel>> newimg(height_pixels, vector<Pixel>(width_pixels));
    /*
    The nested for loop below 
    Adds vignette effect to image (dark corners)
    and scales the colors based on the distance of the pixels from the 
    center.
    */

    for (int row = 0; row < height_pixels; row++)
    {
        for (int col = 0; col < width_pixels; col ++)
        {
            Pixel p = image[row][col];
            double distance = sqrt(pow(col - width_pixels / 2, 2) + pow(row - height_pixels / 2, 2));
            double scaling_factor = (height_pixels - distance) / height_pixels;
            int newred =  p.red * scaling_factor;
            int newgreen = ( p.green * scaling_factor);
            int newblue =  (p.blue * scaling_factor);
            // set new pixel to new color values.
            Pixel newpixel = {newred, newgreen, newblue};
            newimg[row][col] = newpixel;

        }
    }

    return newimg;
}
/*
    Function that scales colors of pixels based on existing colors.
    * @param filename is the location where the file is stored
    @param scaling_factor is the scale at which the pixel colors are changed
    @return a new image with a clarendon affect.
*/
vector<vector<Pixel>> proc2(string filename, double scaling_factor)
{
    // Call the read_image function and return its result
    vector<vector<Pixel>> image = read_image(filename);
    // Getting the size of the width and heigh pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    // defines a new image withe strucutr pixels and the same size as the orginal image 
    vector<vector<Pixel>> newimg(height_pixels, vector<Pixel>(width_pixels));
    Pixel newpixel;
    /*
    Loop below adds a vintage effect to an image 
    by scaling based on the average color of each pixel
    */
    for (int row = 0; row < height_pixels; row++)
    {
        for (int col = 0; col < width_pixels; col ++)
        {
            Pixel p = image[row][col];
            int red = p.red;
            int blue =p.blue;
            int green = p.green;
            int average = (red+green+blue)/3;
            int newred;
            int newblue;
            int newgreen;
            // if the cell is light, make it lighter.
            if (average >= 170)
            {
                newred = (255 - (255-red)*scaling_factor);
                newblue = (255 - (255-blue)*scaling_factor);
                newgreen = (255 - (255-green)*scaling_factor);
                newpixel = {newred,newgreen,newblue};
                newimg[row][col] = newpixel;

            }
            // if pixel is dark, scale to make darker
            else if (average < 90)
            {
                newred= red*scaling_factor;
                newblue= blue*scaling_factor;
                newgreen= green*scaling_factor;
                newpixel = {newred,newgreen,newblue};
                newimg[row][col] = newpixel;
            }
            // if cell isnt light or dark, keep.
            else 
            {
                int newred= red;
                int newblue= blue;
                int newgreen= green;
                newpixel = {newred,newgreen,newblue};
                newimg[row][col] = newpixel;

            }
            


        }
    }
    return newimg;

}
/*
    Function that changes the image to a grayscaled image
    * @param filename is the location where the file is stored
    @return a new grayscalled image.
*/
vector<vector<Pixel>> proc3(string filename)
{
    // Call the read_image function and return its result
    vector<vector<Pixel>> image = read_image(filename);
    // Getting the size of the width and heigh pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    // defines a new image withe strucutre pixels and the same size as the orginal image 
    vector<vector<Pixel>> newimg(height_pixels, vector<Pixel>(width_pixels));

    Pixel newpixel;
    for (int row = 0; row < height_pixels; row++)
    {
        for (int col = 0; col < width_pixels; col ++)
        {
            //getting a pixel at row and col
            Pixel p =image[row][col];
            // getting red blue and green
            int red = p.red;
            int blue = p.blue;
            int green =  p.green;
            // getting the gray shade for the pixel
            int gray_pixel = (red+blue+green)/3;
            // setting each color for the gray value
            int newred = gray_pixel;
            int newblue =gray_pixel;
            int newgreen = gray_pixel;
            //setting color for new pixel
            newpixel = {newred,newblue,newgreen};
            //setting new pixel into the new image
            newimg[row][col] = newpixel;

        }
    }
    return newimg;


}
/*
    Function that rotates an image 90 degrees.
    * @param filename is the location where the file is stored
    @return a new rotated image.
*/
vector<vector<Pixel>> proc4(string filename)
{
    // Call the read_image function and return its result
    vector<vector<Pixel>> image = read_image(filename);
    // Getting the size of the width and heigh pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    // defines a new image withe strucutre pixels and rotated.
    vector<vector<Pixel>> newimg(width_pixels, vector<Pixel>(height_pixels));
    // loop thqt switches the row and col pixels of the new image so that it rotates.
    for (int row = 0; row < height_pixels; row++)
    {
        for (int col = 0; col < width_pixels; col ++)
        {
            Pixel p = image.at(row).at(col);
            newimg.at(col).at((height_pixels-1)-row) = p;

        }
    }

    return newimg;
}
/*
    Function that rotates an image 90 degrees.
    * @param filename is the location where the file is stored
    @return a new rotated image.
*/
vector<vector<Pixel>> rotate_90(vector<vector<Pixel>> image)
{

    int width_pixels = image[0].size();
    int height_pixels = image.size();
    // defines a new image withe strucutre pixels and rotated.
    vector<vector<Pixel>> newimg(width_pixels, vector<Pixel>(height_pixels));
    //Pixel newpixel;
    // same as proc 4 above.
    for (int row = 0; row < height_pixels; row++)
    {
        for (int col = 0; col < width_pixels; col ++)
        {
            Pixel p = image.at(row).at(col);
            newimg.at(col).at((height_pixels-1)-row) = p;

        }
    }
    return newimg;
}
/*
    Function that rotates an image in 90 degree incremenets.
    * @param filename is the location where the file is stored
    * @param number is the amount of times the image will be rotated 90 degrees.
    @return a new rotated image.
*/
vector<vector<Pixel>> proc5(string filename, int number)
{

    // reads image
    vector<vector<Pixel>> image = read_image(filename);
    // number of rotations multiplied by 90.
    // conditionals to rotate 90 degrees based on number of rotations. 
    int angle = (number*90);
    if (angle%90 !=0)
    {
        cout << " Error: number must be a multiple of 90 degrees!";
        return image;
    }
    else if (angle%360 == 0)
    {
        return image;
    }
    else if (angle%360 == 90)
    {
        return rotate_90(image);
    }
    else if (angle%360 ==180)
    {
        return rotate_90(rotate_90(image));
    }
    else
    {
      return rotate_90(rotate_90(rotate_90(image)));  
    }
    return image;
}
/*
    Function that enlarges an image.
    * @param filename is the location where the file is stored
    @param yscale specifies how much the height needs to change
    @param xscale specifies how much the width needs to change
    @return a new enlarged image.
*/
vector<vector<Pixel>> proc6(string filename, int xscale, int yscale)
{

    vector<vector<Pixel>> image= read_image(filename);
    // Check if the image is valid (not empty)
    if (image.empty()) {
        cout << "Error: Image could not be read or is empty!" << endl;
        return image; // Returning the empty image
    }

    // Check for valid scaling factors
    if (xscale <= 0 || yscale <= 0) {
        cout << "Error: Scaling factors must be positive non-zero integers!" << endl;
        return image; // Returning the original image
    }// Check if the image is valid (not empty)
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    //set up a new image space scaled 
    vector<vector<Pixel>> newimg(height_pixels*yscale, vector<Pixel>(width_pixels*xscale));
    // sets each pixel of the new image to the scalled image.
    for (int row =0; row < height_pixels*yscale;row++)
    {
        for(int col =0; col < width_pixels*xscale; col++ )
        {
            Pixel p = image.at(row/yscale).at(col/xscale);
            newimg[row][col] =p;
        }
    }

    return newimg;
}
/*
    Function that changes an image to a black and white image. 
    * @param filename is the location where the file is stored
    @return a new high contrast B/W image.
*/
vector<vector<Pixel>> proc7(string filename)
{
    vector<vector<Pixel>> img = read_image(filename);
    int width_pixels=img[0].size();
    int height_pixels=img.size();
    Pixel newpixel;
    vector<vector<Pixel>> newimg(height_pixels, vector<Pixel>(width_pixels));
    for (int row=0; row<height_pixels;row++)
    {
        for (int col = 0; col<width_pixels;col++)
        {
            Pixel p = img.at(row).at(col);
            int red = p.red;
            int blue =p.blue;
            int green =p.green;
            // get grey value
            int gray = (red+green+blue)/3;
            // if light, make white. 
            if (gray>= 255/2)
            {
                int newred =255;
                int newgreen =255;
                int newblue =255;
                newpixel= {newred,newgreen,newblue};
                newimg[row][col]=newpixel;
            }
            // if not light, make black.
            else
            {
                newpixel={0,0,0};
                newimg[row][col]=newpixel;
            }
        }
    }

    return newimg;
}
/*
    Function that lightens an image.
    * @param filename is the location where the file is stored
    @param scaling_facotr is how much lighter an image should be.
    @return a new lighter image.
*/
vector<vector<Pixel>> proc8(string filename, double scaling_factor)
{
    // new image based on the size of the input image.
    vector<vector<Pixel>> img=read_image(filename);
    int width_pixels = img[0].size();
    int height_pixels = img.size();
    vector<vector<Pixel>> newimg(height_pixels, vector<Pixel>(width_pixels));
    Pixel newpixel;
    for (int row=0; row<height_pixels; row++)
    {
        for (int col=0; col < width_pixels; col++)
        {
            Pixel p = img[row][col];
            int red = p.red;
            int green = p.green;
            int blue = p.blue;
            // applying scaling factor to lighten
            int newred = (255-(255-red)*scaling_factor);
            int newgreen =(255-(255-green)*scaling_factor);
            int newblue=(255-(255-blue)*scaling_factor);
            newpixel = {newred,newgreen,newblue} ;
            newimg[row][col]=newpixel;
        }
    }

    return newimg;
}
/*
    Function that darkens an image.
    * @param filename is the location where the file is stored
    @param scaling_facotr is how much darker an image should be.
    @return a new darker image.
*/
vector<vector<Pixel>> proc9(string filename,double scaling_factor)
{
 // new image based on size of existing image.
 vector<vector<Pixel>> image= read_image(filename);
 int width_pixels = image[0].size();
 int height_pixels = image.size();
 vector<vector<Pixel>> newimg(height_pixels,vector<Pixel>(width_pixels));
 Pixel newpixel;
 for (int row =0; row <height_pixels;row++)
 {
    for (int col  = 0; col<width_pixels;col++)
    {
        Pixel P = image[row][col];
        int red = P.red;
        int green = P.green;
        int blue = P.blue;
        // darkens pixels based on scaling factor.
        int newred = red*scaling_factor;
        int newgreen = green*scaling_factor;
        int newblue = blue*scaling_factor;
        newpixel= {newred,newgreen,newblue};
        newimg[row][col]= newpixel;

    }
 }
    return newimg;
}
/*
    Function that changes an image only using black, white, red, green, and blue colors.
    * @param filename is the location where the file is stored
    @return a new colored image.
*/
vector<vector<Pixel>> proc10 (string filename)
{
    vector<vector<Pixel>> image= read_image(filename);
    int width_pixels = image[0].size();
    int height_pixels = image.size();
    vector<vector<Pixel>> newimg(height_pixels,vector<Pixel> (width_pixels));
    Pixel newpixel;
    for (int row =0; row<height_pixels; row++)
    {
        for (int col =0; col < width_pixels; col++)
        {
            Pixel p= image[row][col];
            //getting color of each pixel
            int red =p.red;
            int green=p.green;
            int blue = p.blue;
            // new paramaters
            int newred;
            int newblue;
            int newgreen;
            // conditions for loops
            int max_color= max((red),max((green),(blue)));
            int sum_color = red+green+blue;
            // if light, make white. 
            if (sum_color >= 550)
            {
                newred =255;
                newblue = 255;
                newgreen = 255;
                newpixel={newred,newgreen,newblue};
                newimg[row][col]= newpixel;
            }
            // if dark, make black. 
            else if (sum_color <=150)
            {
                newred =0;
                newblue=0;
                newgreen=0;
                newpixel={newred,newgreen,newblue};
                newimg[row][col]= newpixel;
            }
            // if red, keep as red.
            else if (max_color==red)
            {
                newred=255;
                newblue=0;
                newgreen=0;
                newpixel={newred,newgreen,newblue};
                newimg[row][col]= newpixel;

            }
            // of green, keep as green. 
            else if (max_color==green)
            {
                newred = 0;
                newgreen =255;
                newblue = 0;
                newpixel={newred,newgreen,newblue};
                newimg[row][col]= newpixel;
            }
            // if blue, keep blue.
            else 
            {
                newred=0;
                newblue=255;
                newgreen=0;
                newpixel={newred,newgreen,newblue};
                newimg[row][col]= newpixel;
            }


        }
    }

    return newimg;
}


/*
    Function that applies an image filter based on user choice
    @param choice is the user input on the image filter selection. 
    * @param filename is the location where the file is stored
*/

void applyFilter(int choice, const string& filename) {
    // forward decliration of varibales in the switch cases
    vector<vector<Pixel>> newimage;
    string output_filename;
    double scaling_factor;
    int rotation_number;
    int x_scale;
    int y_scale;
    // switch to assign a function based on user input
    switch (choice) {
        case 1:
            newimage= proc1(filename);
            break;
        case 2:
            cout << "Please select a scaling factor (between 0 and 1)";
            // check to verify valid user input
            while (!(cin >> scaling_factor) || scaling_factor>1.0 || scaling_factor<0.0 )
            {   
                cin.clear();
                cin.ignore();
                cout << "Error: please select a scaling factor between 0 and 1: ";
            }
            newimage= proc2(filename,scaling_factor);
            break;
        case 3:
            newimage = proc3(filename);
            break;
        case 4:
            newimage= proc4(filename);
            break;
        case 5:
            cout << "Please insert amount of times you would like to rotate the image by 90 degrees: ";
            // check to verify valid user input
            while(!(cin >> rotation_number))
            {
                cin.clear();
                cin.ignore();
               
                cout << "Error: please select a whole number for rotation: ";
            }
            newimage = proc5(filename,rotation_number);
            break;
        case 6:
            cout <<"Please enter a scale you would like to enalrge the height by (must be a whole nuber greater than 0): ";
            // check to verify valid user input
            while(!(cin >> y_scale) || y_scale<=0)
            {
                cin.clear();
                cin.ignore();
               
                cout << "Error: please select a whole number for scalling: ";
            }
            cout <<"Please enter a scale you would like to enalrge the width by (must be a whole nuber greater than 0): ";
            // check to verify valid user input
            while(!(cin >> x_scale) || x_scale<=0)
            {
                cin.clear();
                cin.ignore();
                cout << "Error: please select a whole number for scalling: ";
            }
            newimage = proc6(filename,x_scale,y_scale);
        break;
        case 7:
            newimage = proc7(filename);            
            break;
        case 8:

            cout << "Please select a scaling factor (between 0 and 1): ";
            // check to verify valid user input
            while (!(cin >> scaling_factor) || scaling_factor>1.0 || scaling_factor<0.0 )
            {   
                cin.clear();
                cin.ignore();
                cout << "Error: please select a scaling factor between 0 and 1: ";
            }
            newimage = proc8(filename,scaling_factor);

            break;
        case 9:
            cout << "Please select a scaling factor (between 0 and 1): ";
            // check to verify valid user input
            while (!(cin >> scaling_factor) || scaling_factor>1.0 || scaling_factor<0.0 )
            {   
                cin.clear();
                cin.ignore();
                cout << "Error: please select a scaling factor between 0 and 1: ";
            }
            newimage = proc9(filename,scaling_factor);
            
            break;
        case 10:
            newimage=proc10(filename);
            break;
        default:
        // check to verify valid user input
            cout << "Invalid choice. This should never happen." << endl;
            break;
    }
    // loop that assigns a new filename for the new image if the input is valid.
    // and saves the new image. 
    for (int i =1; i<11;i++)
    {
        if (choice == i)
        {
            cout << "Enter a image name for the new image (dont provide the extension or path): ";
            cin >>output_filename;
            const char* file = filename.c_str();
            int len = strlen(file);
            // loop that inserts the new file name before the .bmp extension 
            for (int i = len -1; i>=0; i--)
            {
                if (file[i] == '.')
                {
                    string str_num = to_string(i);
                    output_filename = filename.substr(0,i)+ +"_"+output_filename+ ".bmp";
                    break;
                }
            }
            // writes a new image with the userinput. 
            write_image(output_filename, newimage);
            cout << "Successfully saved " + output_filename<<endl;
        }
    }
}
/*
    Function that asks for user input
    and checks user input. If the user input
    is called then apply filter is run with the 
*/
void User_interface() {
    // string choice to handle quit.
    string choice;
    string filename;
    cout << "Welcome to Faisal's image editing application" << endl;

    // loop to ask the user what to do and breaks for invalid argument or quit.
    while (true) {
        cout << "Please select the filter you would like to apply to your image (must be a bmp file) or Q to quit" << endl;
        cout << "Image Processing Menu" << endl;
        cout << "1. Vignette\n2. Clarendon\n3. Grayscale\n4. Rotate 90 degrees\n5. Rotate multiple 90 degrees\n6. Enlarge\n7. High Contrast\n8. Lighten\n9. Darken\n10. Black, white, red, green, blue\n";
        cout << "Q. Quit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        // user input to quit.
        if (choice == "Q" || choice == "q") {
            cout << "Exiting. Hope you had a good experience!" << endl;
            break;
        }

        int choiceInt;
        // trys to convert a string to an integer, if it passes that represents the user input
        try {
            choiceInt = stoi(choice);
        } catch (invalid_argument&) {
            cout << "Invalid choice. Please enter a number or 'Q' to quit." << endl;
            continue;
        }

        // if user puts and invalid integer, it reprompts for input
        if (choiceInt < 1 || choiceInt > 10) {
            cout << "Choice out of range. Please enter a valid option from the menu." << endl;
            continue;
        }

        // user input for filename-- FIX FIX FIX -- INCLUDE FILE SYSTEMS. 
        string out_put_name;
        cout << "Enter the filepath of the BMP image: "<< endl;
        // check to see if a file name is right
        while (cin>>filename)
        {
            const char* file = filename.c_str();
            int len = strlen(file);
            if (file[len-1]== 'p'&& file[len-2]== 'm' && file[len-3] == 'b' )
            {
                try {
                    vector<vector<Pixel>> orginal_image_check = read_image(filename);
                    break;
                } catch (invalid_argument&) {
                    cout << "Invalid choice. Please enter a valid file path: " << endl;
                    continue;
                }
                break;
            }
            else
            {
                cin.clear();
                cin.ignore();
                cout << "Error file is not a bmp file, try again: ";
            }
        }
        
        /*
        I wanted to use this code below for a file check but this is std C++17.
        while (cin >> filename)
        {
            filesystem::path pathToCheck(filename);
            if (filesystem::exists(pathToCheck)) {
                break;
            } else {
                cin.clear();
                cin.ignore();
                cout << "Path does not exist. Please enter a valid filepath: " << endl;
            }
        } */


        

        // Call the function to apply the selected filter
        applyFilter(choiceInt, filename);
    }
}


int main()
{
    //string file_test="/Users/faisalshahin/Downloads/final/sample_images/sample.bmp";
    User_interface();
    return 0;
}