#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

int main () {
    glm::mat4 projection = glm::ortho(0.0f, 64.0f, 32.0f, 0.0f, -1.0f, 1.0f);

    std::cout <<  "| " << projection[0][0] << " | " << projection[1][0] << " | " << projection[2][0] << " | " << projection[3][0] << "|" << std::endl;
    std::cout <<  "| " << projection[0][1] << " | " << projection[1][1] << " | " << projection[2][1] << " | " << projection[3][1] << "|" << std::endl;
    std::cout <<  "| " << projection[0][2] << " | " << projection[1][2] << " | " << projection[2][2] << " | " << projection[3][2] << "|" << std::endl;
    std::cout <<  "| " << projection[0][3] << " | " << projection[1][3] << " | " << projection[2][3] << " | " << projection[3][3] << "|" << std::endl;

    unsigned char a = 0b11011101;
    unsigned char b = a << 2;
    std::cout << ((a << 2) >> 4) << "\n" << (int)b << "\n" << (b >> 4) << std::endl;

    std::cout << __cplusplus << std::endl;
}
