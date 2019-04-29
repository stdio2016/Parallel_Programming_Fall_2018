#include <cmath> 
#include <iomanip> 
#include <iostream> 
#include <fstream>
#include <vector>
#define ull unsigned long long int
using namespace std; 
int matrix_size;
const float sigma = 15.0f; 
const ull RATE = 1e7;
void FilterCreation(vector<vector <float> >& GKernel) 
{ 
    float r, s = 2.0 * sigma * sigma; 
    // sum is for normalization 
    float sum = 0.0; 
  
    // generating  kernel 
    for (int x = -matrix_size / 2; x <= matrix_size / 2; x++)
    { 
        for (int y = -matrix_size / 2; y <= matrix_size / 2; y++) 
        { 
            r = sqrt(x * x + y * y); 
            GKernel[x + matrix_size / 2][y + matrix_size / 2] = (exp(-(r * r) / s)) / (M_PI * s); 
            sum += GKernel[x + matrix_size / 2][y + matrix_size / 2];
        } 
    } 
  
    // normalising the Kernel 
    for (int i = 0; i < matrix_size; ++i)
    {
        for (int j = 0; j < matrix_size; ++j)
        {
            GKernel[i][j] /= sum;
        }
    }
        
} 
  
void AxisCreation(vector<float> &GKernel) {
    float r, s = 2.0 * sigma * sigma; 
    // sum is for normalization 
    float sum = 0.0; 
  
    // generating  kernel 
    for (int x = -matrix_size / 2; x <= matrix_size / 2; x++)
    {
        r = sqrt(x * x); 
        GKernel[x + matrix_size / 2] = (exp(-(r * r) / s)) / (M_PI * s); 
        sum += GKernel[x + matrix_size / 2]; 
    } 
  
    // normalising the Kernel 
    for (int i = 0; i < matrix_size; ++i)
    {
        GKernel[i] /= sum;
    }
}

// Driver program to test above function 
int main() 
{ 
    ofstream output_file("mask_Gaussian.txt");
    ofstream output_file2("mask_Gaussian_1D.txt");
    if(!output_file.is_open() || !output_file2.is_open())
    {
        printf("Error reading file");
    }

    printf("Input matrix size: ");
    scanf("%d", &matrix_size);
    if(matrix_size % 2 == 0)
    {
        printf("Matrix size MUST be an odd number ! \n");
        return 1;
    }
    printf("Input stddev: ");
    printf("matrix = %d , stddev = %lf \n", matrix_size, sigma);

    vector<vector<float> > GKernel(matrix_size, vector<float>(matrix_size, 0.0f));
    FilterCreation(GKernel); 
  
    vector<float> GAxis(matrix_size, 0.0f);
    AxisCreation(GAxis);

    output_file << matrix_size * matrix_size << endl;

    ull tmp = 0;
    for (int i = 0; i < matrix_size; ++i) 
    { 
        for (int j = 0; j < matrix_size; ++j)
        {
            tmp = (unsigned long long) ( GKernel[i][j] * RATE );
            output_file << tmp << " " ;
        }
        output_file << endl; 
    }

    output_file2 << matrix_size << endl;

    tmp = 0;
    for (int i = 0; i < matrix_size; ++i)
    {
        tmp = (unsigned long long) ( GAxis[i] * RATE );
        output_file2 << tmp << " " ;
    }
    output_file2 << endl;
    return 0;
} 
