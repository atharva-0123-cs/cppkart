
#include "MapChunker.h"

void terrainMapLoader(std::vector<GLuint>& indices_vec, std::vector<GLfloat>& vertices_vec) {

     const char* filename = "../src/ressources/Map_A.png";
    int width, height, channels;

    // Load the PNG image
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!image) {
        std::cerr << "Error loading image: " << stbi_failure_reason() << std::endl;
    }

    std::cout << "Image SIZE: H- " << height << "px, W- " << width << "px " << std::endl;

    // Iterate through the pixels
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

        if (x < (width - 1) && y < (height - 1)) { //Prevent Wrapping of Indices
            //Triangle 1 / 2 (per unit)
            indices_vec.push_back(y * width + x);
            indices_vec.push_back((y + 1) * width + x);
            indices_vec.push_back(y * width + (x + 1));

            //Triangle 2 / 2 (per unit)
            indices_vec.push_back((y + 1) * width + x);
            indices_vec.push_back(y * width + (x + 1));
            indices_vec.push_back((y + 1) * width + (x + 1));
        }

            int raw_img_index = (y * width + x) * 4; // Each pixel has 4 channels (RGBA)

            // Access the RGBA components of the pixel
    unsigned char color_c = image[raw_img_index];
    float normalized_color = static_cast<float>(color_c) / ( 255.0f*0.75f);

            //X: 
            vertices_vec.push_back(static_cast<GLfloat>(x)/width);
            //Y:
            vertices_vec.push_back(static_cast<GLfloat>(normalized_color/1.5)); //height vertical
            //Z:
            vertices_vec.push_back(static_cast<GLfloat>(y)/width); //Not multiplying by height, as I dont want to stretch the proportions
            //
            for (int i = 0; i < 5; ++i) {
                vertices_vec.push_back(static_cast<GLuint>(0.0f));
            }
        }
    }
    
    std::cout << "Parsed the terrain?" << std::endl;

    stbi_image_free(image); // Free the image data when done
}

bool loadHeightfieldData(const char* filename, std::vector<unsigned short>& heightData, int& width, int& length, btScalar& minHeight, btScalar& maxHeight) {

    int channels;

    // Load the PNG image
    unsigned char* image = stbi_load(filename, &width, &length, &channels, STBI_rgb_alpha);

    if (!image) {
        std::cerr << "Error loading image: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "Image SIZE: H- " << length << "px, W- " << width << "px " << std::endl;

    //! Initially set LOW & HIGH
    unsigned short maxPixelValue = 0;
    unsigned short minPixelValue = 65535; // 16-bit max value, 2^16-1

    // Iterate through the pixels to populate heightData and find minHeight/maxHeight
    for (int y = 0; y < length; ++y) {
        for (int x = 0; x < width; ++x) {
            int raw_img_index = (y * width + x) * 4; // Each pixel has 4 channels (RGBA)
            unsigned char color_c = image[raw_img_index];  // Assuming height is represented by the red channel

            unsigned short heightValue = static_cast<unsigned short>(color_c/10); // 16-bit value from R and G channels
            heightData.push_back(heightValue);

            maxPixelValue = std::max(maxPixelValue, heightValue);
            minPixelValue = std::min(minPixelValue, heightValue);
        }
    }

    minHeight = static_cast<btScalar>(minPixelValue);
    maxHeight = static_cast<btScalar>(maxPixelValue);

    stbi_image_free(image);


//! test code for smoothing
    std::vector<unsigned short> originalHeightData = heightData;

    // Define the neighborhood size for smoothing (3x3)
    int neighborSize = 1; 

    // Apply smoothing
    for (int y = 0; y < length; ++y) {
        for (int x = 0; x < width; ++x) {
            int sum = 0;
            int count = 0;

            // Iterate over the neighborhood
            for (int ny = -neighborSize; ny <= neighborSize; ++ny) {
                for (int nx = -neighborSize; nx <= neighborSize; ++nx) {
                    int newY = y + ny;
                    int newX = x + nx;

                    // Check if the neighbor is within the image bounds
                    if (newY >= 0 && newY < length && newX >= 0 && newX < width) {
                        sum += originalHeightData[newY * width + newX];
                        ++count;
                    }
                }
            }

            // Set the smoothed height value
            heightData[y * width + x] = sum / count;
        }
    }
//! -------------------------


    return true;
}