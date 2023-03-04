#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{ 0x4D42 }; //тип файла (аски код двух символов BM)
    uint32_t file_size{ 0 };// размер файла в байтах
    uint16_t reserved1{ 0 };//зарезервировано (должно быть 0)
    uint16_t reserved2{ 0 };
    uint32_t offset_data{ 0 };//смещение в байтах от начала структуры до BMPFileHeader до массива пкселей
  
};

struct BMPInfoHeader {
    uint32_t size{ 0 }; //размер структуры в байтах
    int32_t width{ 0 }; //ширина в пикселях
    int32_t height{ 0 };//ширина в пикселях. Если эта величина + то строки изображения хранятся снизу вверх. Если - то наоборот


    uint16_t planes{ 1 }; // кол-во плоскостей (должно быть 1)
    uint16_t bit_count{ 0 }; // количество бит на пиксель (24 для BMP24)
    uint32_t compression{ 0 };// тип сжатия
    uint32_t size_image{ 0 }; //размер изображения в байтах
    int32_t x_pixels_per_meter{ 0 };//пиксели на метр (горизонтальное решение)
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 }; //размер палитры (0 для 24)
    uint32_t colors_important{ 0 }; //колво значимых элтов палитры (0 для 24)
};

//struct BMPColorHeader {
//    uint32_t red_mask{ 0x00ff0000 };
//    uint32_t green_mask{ 0x0000ff00 };
//    uint32_t blue_mask{ 0x000000ff };
//    uint32_t alpha_mask{ 0xff000000 };
//    uint32_t color_space_type{ 0x73524742 };
//    uint32_t unused[16]{ 0 };
//};
#pragma pack(pop)

struct BMP {
public:
    const char* file_name;
    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
  //  BMPColorHeader bmp_color_header;
    std::vector<uint8_t> data;
    BMP() { }
    BMP(const char* fname) {
        file_name = fname;
        read(fname);
    }


    void read(const char* fname) {
        std::ifstream inp{ fname, std::ios_base::binary };
        if (inp) {
            inp.read((char*)&file_header, sizeof(file_header));
            if (file_header.file_type != 0x4D42) {
                throw std::runtime_error("Error! Unrecognized file format.");
            }
            inp.read((char*)&bmp_info_header, sizeof(bmp_info_header));
          //  printf("%d", bmp_info_header.bit_count);
            if (bmp_info_header.bit_count == 0)    throw std::runtime_error("Error! Unrecognized file format, its not 24.");
            /*if (bmp_info_header.bit_count == 32) {
                if (bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) {
                    inp.read((char*)&bmp_color_header, sizeof(bmp_color_header));
                }
                else {
                    std::cerr << "Error! The file \"" << fname << "\" does not seem to contain bit mask information\n";
                    throw std::runtime_error("Error! Unrecognized file format.");
                }
            }*/

            inp.seekg(file_header.offset_data, inp.beg); // Устанавливает позицию следующего символа, который будет извлечен из входного потока.

            /*if (bmp_info_header.bit_count == 32) {
                bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            }*/
            if (bmp_info_header.bit_count != 32) {
                bmp_info_header.size = sizeof(BMPInfoHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            file_header.file_size = file_header.offset_data;

            if (bmp_info_header.height < 0) {
                throw std::runtime_error("The program can treat only BMP images with the origin in the bottom left corner!");
            }

            data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

            if (bmp_info_header.width % 4 == 0) {
                inp.read((char*)data.data(), data.size());
                file_header.file_size += static_cast<uint32_t>(data.size());
            }
            else {
                row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
                uint32_t new_stride = make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - row_stride);

                for (int y = 0; y < bmp_info_header.height; ++y) {
                    inp.read((char*)(data.data() + row_stride * y), row_stride);
                    inp.read((char*)padding_row.data(), padding_row.size());
                }
                file_header.file_size += static_cast<uint32_t>(data.size()) + bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
            }
        }
        else {
            throw std::runtime_error("Unable to open the input image file.");
        }
    }

    void write(const char* fname) {
        std::ofstream of{ fname, std::ios_base::binary };
        if (of) {
            if (bmp_info_header.bit_count == 32) {
                write_headers_and_data(of);
            }
            else if (bmp_info_header.bit_count == 24) {
                if (bmp_info_header.width % 4 == 0) {
                    write_headers_and_data(of);
                }
                else {
                    uint32_t row_stride = bmp_info_header.bit_count / 8 * bmp_info_header.width;
                    uint32_t remainder = row_stride % 4;
                    uint32_t new_stride = row_stride + 4 - remainder;
                    std::vector<uint8_t> padding_row(new_stride - row_stride);
                    write_headers(of);
                    for (int y = 0; y < bmp_info_header.height; ++y) {
                        of.write((const char*)(data.data() + row_stride * y), row_stride);
                        of.write((const char*)padding_row.data(), padding_row.size());
                    }
                }
            }
            else {
                throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
            }
        }
        else {
            throw std::runtime_error("Unable to open the output image file.");
        }
    }
    BMP copyB() {
        BMP newImage(this->file_name);
        for (int i = 0; i != newImage.data.size(); i++) {
            if (i % 3 != 0)  newImage.data[i] = 0;
        }
       // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP copyG() {
        BMP newImage(this->file_name);
        for (int i = 0; i != newImage.data.size(); i++) {
            if (i % 3 != 1)  newImage.data[i] = 0;
        }
       // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP copyR() {
        BMP newImage(this->file_name);
        for (int i = 0; i != newImage.data.size(); i++) {
            if (i % 3 != 2)  newImage.data[i] = 0;
        }
      //  newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    double sigma(std::vector<double> mas, double M) {

        double sigma = 0;

        for (int i = 0; i != mas.size(); i++) {
            mas[i] = pow(mas[i] -= M, 2);
        }
        for (int i = 0; i != mas.size(); i++) {
           sigma += mas[i];
        }
        sigma = sigma / (mas.size() - 1);

        sigma = sqrt( sigma / ((mas.size()) - 1));
    }
    double M(char A, std::vector<uint8_t> mas) {

        int A_div;
        double M = 0;
        if (A == 'R') A_div = 2;
        else if (A == 'G') A_div = 1;
        else A_div = 0;

        for (int i = 0; i != this->data.size(); i++) {
            if (i % 3 == A_div) M += data[i];
   
        }
        M = M / (mas.size() / 3);
        return M;
    }
    double correl(char A, char B) {
        int A_div, B_div;
        if (A == 'R') A_div = 2;
        else if (A == 'G') A_div = 1;
        else A_div = 0;

        if (B == 'R') B_div = 2;
        else if (B == 'G') B_div = 1;
        else B_div = 0;

        double M_A = 0, M_B = 0;
        for (int i = 0; i != this->data.size(); i++) {
            if (i % 3 == A_div) M_A += data[i];
            else if (i % 3 == B_div) M_B += data[i];
        }
        M_A = M_A / (data.size() / 3);
        M_B = M_B / (data.size() / 3);
        std::cout << "M(A) and M(B) == " << M_A << " " << M_B << std::endl;
        double A_standartDev = 0, B_standartDev = 0;
        for (int i = 0; i != this->data.size(); i++) {
            if (i % 3 == A_div)  A_standartDev += powf((double)((double)data[i] - M_A), 2);
            else if (i % 3 == B_div) B_standartDev += powf((double)((double)data[i] - M_B), 2);
        }
       // std::cout << "standartDev == " << A_standartDev << " " << B_standartDev << std::endl;

        A_standartDev = sqrt(A_standartDev / ((data.size() / 3) - 1) );
        B_standartDev = sqrt(B_standartDev / ((data.size() / 3) - 1));

        std::cout << "standartDev == " << A_standartDev << " " << B_standartDev << std::endl;

        std::vector<double> mas;
       
        double mas_A = 0, mas_B = 0, count= 0 ;
        for (int i = 0; i != this->data.size(); i++) {
            if (i % 3 == A_div) {
               
                mas_A = data[i] - M_A;
                mas.push_back(mas_A);
                count++;
            }
            
            
        }
        for (int i = 0; i != this->data.size(); i++) {
            if (i % 3 == A_div) {

                mas_B = data[i] - M_B;
                mas.push_back(mas_B);
                count++;
            }


        }
        int M_AB = 0;
        for (int i = 0; i != mas.size(); i++) {
            M_AB += mas[i];
        }
        M_AB = M_AB / mas.size();
       std::cout << "M_AB == " << M_AB << std::endl;

        double corr = M_AB / (A_standartDev * B_standartDev);
        return corr;
    }
    BMP toYCC(const char* fname) {
        BMP newImage(fname);

        float R = 0, G = 0, B = 0;

        for (int i = 0; i != newImage.data.size(); i++) {
            
            if (i % 3 == 0) B = newImage.data[i];
            else if (i % 3 == 1) G = newImage.data[i];
            else if (i % 3 == 2) {
                R = newImage.data[i];
                float Y = 0.299 * R + 0.587 * G + 0.144 * B;
                float Cb = 0.5643 * (B - Y) + 128;
                float Cr = 0.7132 * (R - Y) + 128;

                newImage.data[i - 2] = Cr;
                newImage.data[i - 1] = Y;
                newImage.data[i] = Cb;

            }
        }
        // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP fromYCC() {
        BMP newImage(file_name);

        float Y = 0, Cr = 0, Cb = 0;
        
        for (int i = 0; i != newImage.data.size(); i++) {

            if (i % 3 == 0) Cr = newImage.data[i];
            else if (i % 3 == 1) Y = newImage.data[i];
            else if (i % 3 == 2) {
                Cb = newImage.data[i];
                float R = Y + 1.402 * (Cr -128);
                float G = Y - 0.714 * (Cr - 128) - 0.334 * (Cb -128);
                float B = Y + 1.772 * (Cb - 128);

                newImage.data[i - 2] = B;
                newImage.data[i - 1] = R;
                newImage.data[i] = R;

            }
        }
        // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP toY() {
        BMP newImage(this->file_name);

        float R = 0, G = 0, B = 0;

        for (int i = 0; i != newImage.data.size(); i++) {

            if (i % 3 == 0) B = newImage.data[i];
            else if (i % 3 == 1) G = newImage.data[i];
            else if (i % 3 == 2) {
                R = newImage.data[i];
                float Y = 0.299 * R + 0.587 * G + 0.144 * B;
                float Cb = 0.5643 * (B - Y) + 128;
                float Cr = 0.7132 * (R - Y) + 128;

                newImage.data[i - 2] = Y;
                newImage.data[i - 1] = Y;
                newImage.data[i] = Y;

            }
        }
        // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP toCb() {
        BMP newImage(this->file_name);

        float R = 0, G = 0, B = 0;

        for (int i = 0; i != newImage.data.size(); i++) {

            if (i % 3 == 0) B = newImage.data[i];
            else if (i % 3 == 1) G = newImage.data[i];
            else if (i % 3 == 2) {
                R = newImage.data[i];
                float Y = 0.299 * R + 0.587 * G + 0.144 * B;
                float Cb = 0.5643 * (B - Y) + 128;
                float Cr = 0.7132 * (R - Y) + 128;

                newImage.data[i - 2] = Cb;
                newImage.data[i - 1] = Cb;
                newImage.data[i] = Cb;

            }
        }
        // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
    BMP toCr() {
        BMP newImage(this->file_name);

        float R = 0, G = 0, B = 0;

        for (int i = 0; i != newImage.data.size(); i++) {

            if (i % 3 == 0) B = newImage.data[i];
            else if (i % 3 == 1) G = newImage.data[i];
            else if (i % 3 == 2) {
                R = newImage.data[i];
                float Y = 0.299 * R + 0.587 * G + 0.144 * B;
                float Cb = 0.5643 * (B - Y) + 128;
                float Cr = 0.7132 * (R - Y) + 128;

                newImage.data[i - 2] = Cr;
                newImage.data[i - 1] = Cr;
                newImage.data[i] = Cr;

            }
        }
        // newImage.file_name = "withRcomp.bmp";
        return newImage;
    }
private:
    uint32_t row_stride{ 0 };

    void write_headers(std::ofstream& of) {
        of.write((const char*)&file_header, sizeof(file_header));
        of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
        /*if (bmp_info_header.bit_count == 32) {
            of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
        }*/
    }

    void write_headers_and_data(std::ofstream& of) {
        write_headers(of);
        of.write((const char*)data.data(), data.size());
    }

    uint32_t make_stride_aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }
   

};

int main(int argc, char* argv[]) {
    BMP img1("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim21.bmp");
    BMP img1B;
    img1B = img1.copyB();
    img1B.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22B.bmp");
    BMP img1R;
    img1R = img1.copyR();
    img1R.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22R.bmp");
    BMP img1G;
    img1G = img1.copyG();
    img1G.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22G.bmp");

    printf("Corr R and G %f \n", img1.correl('R', 'G'));
    printf("Corr R and B %f \n", img1.correl('R', 'B'));
    printf("Corr B and G %f \n", img1.correl('B', 'G'));

    std::cout << "offset_data " << img1.file_header.offset_data << std::endl;

    /*BMP img1toYCC;
    img1toYCC = img1.toYCC("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22YCC.bmp");
    img1toYCC.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22YCC.bmp");

    BMP img1toY;
    img1toY = img1.toY();
    img1toY.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22Y.bmp");

    BMP img1toCr;
    img1toCr = img1.toCr();
    img1toCr.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22Cr.bmp");

    BMP img1toCb;
    img1toCb = img1.toCb();
    img1toCb.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22Cb.bmp");

    BMP img1FromYCC;
    img1FromYCC = img1toYCC.fromYCC();
    img1FromYCC.write("C:\\Users\\Катя\\source\\repos\\изображенияЛР1\\изображение\\kodim22fromYCC.bmp");*/
}

//индивидуальное задание: 2А изменить яркость