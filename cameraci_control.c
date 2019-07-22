#include "cameraci.h"
#include <sysexits.h>
#include <omp.h>
#include <stdio.h>
#include <limits.h>
#include "bmp.h"

//setup array for bmp output
RGBTRIPLE** pixels;
 
int main(int argc, const char **argv){

	//setup file to write histogram to
	FILE *fp = fopen("Picture_Histograms/hist.txt", "a");			
	if(fp == NULL){
		printf("Error!");
		exit(1);
	}
		

    RASPISTILLYUV_STATE *state=raspistill_create_status();
    MMAL_STATUS_T status = MMAL_SUCCESS;
    int exit_code = EX_OK;


	int k = 0;
	int l = 0;
	int kval = 0;
    int W=3296;
    int H=2448;
    
    state->timeout = 1000000;
    state->width = W;
    state->height = H;
    state->useRGB = 1;
    state->onlyLuma=0; 
    state->verbose = 0;
    state->fullResPreview = 1;


    exit_code = raspistill_init(state);
    if(exit_code==0){

        int _w=0;
        int _h=0;
        raspistill_get_actual_capture_size(state, &_w, &_h);
        printf( "camera_still_port dimensions as reported from camera port: %dx%d\n", _w, _h);
        W=_w;
        H=_h;
		
		//setup buffer to hold stream of rgb values
        unsigned char *buffer=(unsigned char*)malloc(W*H*3);

        //take a picture using library function
        int status=raspistill_capture(state, buffer);
		
		/*Saving RGB as BMP file and 
		 * also copying to an array to generate histogram as follows
		 * 
		 * */ 
		char *outputFile = argv[1];
		FILE *outptr = fopen(outputFile, "w");
		if(outptr == NULL){
			fprintf(stderr, "Could not create file testbm\n");
			exit(1);
		}
		
		int padding = (4-(W * sizeof(RGBTRIPLE)) % 4) %4;
		
		
		int filesize = (H * (W + padding))*3 + 54;

		BITMAPFILEHEADER bf;
		bf.bfType = 0x4d42;
		bf.bfSize = filesize;
		bf.bfReserved1 = 0;
		bf.bfReserved2 = 0;
		bf.bfOffBits = 54;
		
		BITMAPINFOHEADER bi;
		bi.biSize = 40;
		bi.biWidth = W;
		bi.biHeight = H;
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = 0;
		bi.biSizeImage = W*H*3;
		bi.biXPelsPerMeter = 3296;
		bi.biYPelsPerMeter = 2448;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;
		

		fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);
		fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

		pixels = malloc(W*sizeof(RGBTRIPLE*));
		for(int i = 0; i < W; i++){
			pixels[i] = malloc(H*sizeof(RGBTRIPLE));
		}
		
		//setup output1 as 2d array of pointers
		int **output1 = (int **)malloc(sizeof(int*) * W);
		for(int i = 0; i< W; i++){
			output1[i] = (int*)malloc(sizeof(int) * H);
		} 
		
		//combine each rgb value into one pixel value also copy to bmp output pixel array
		for(int i = 0; i<W; i++){
			for(int j = 0; j<H; j++){
				kval = (buffer[k] + buffer[k+1] + buffer[k+2]);
				output1[i][j] = kval;
				k+=3;
				
				pixels[i][j].rgbtBlue = kval;
				pixels[i][j].rgbtGreen = kval;
				pixels[i][j].rgbtRed = kval;
				
			 }
		 }
		 
		 //write bmp pixel array to file
		 for(int i = 0; i<W; i++){
			for(int j = 0; j<H; j++){
				fwrite(&(pixels[i][j]), sizeof(RGBTRIPLE), 1, outptr);
			}
		}

		fclose(outptr);
		
		//output3 holds histogram
		int output3[256];
		//set everything to zero in array
		for(int i = 0; i<256; i++){
			output3[i] = 0;
		}

		
		/* LBP as follows: 
		 * compare values to neighbour to get final pixel value for each cell
		 * 
		 * 
		 * */
        int valToAdd = 0;
        
		//#pragma omp parallel for private(i,valToAdd)
         for (int i =1; i< (W-1); i++){
			 for(int j = 1; j< (H-1); j++){
				 if(output1[i][j] < output1[i-1][j-1]){
					valToAdd += 128;
					}
				 if(output1[i][j] < output1[i-1][j]){
				 	valToAdd += 64;
					}
				 if(output1[i][j] < output1[i-1][j+1]){
					valToAdd += 32;
					}
				 if(output1[i][j] < output1[i][j+1]){
					valToAdd += 16;
					}
				 if(output1[i][j] < output1[i+1][j+1]){
					valToAdd += 8;
					}
				 if(output1[i][j] < output1[i+1][j]){
					valToAdd += 4;
					}
				 if(output1[i][j] < output1[i+1][j-1]){
					valToAdd += 2;
					}
				 if(output1[i][j] < output1[i][j-1]){
				  valToAdd += 1;
				}	
				output3[valToAdd] += 1;
				valToAdd = 0;
			}
		}

		/* Uncomment this to store taken pictures histogram in the databse
		 * 
		 * 
		 */
		//for(int i =0; i<256; i++){
			//fprintf(fp,"%d ",output3[i]);
		//}
		//fprintf(fp," %s", argv[1]);
		//fprintf(fp," \n");
		//fclose(fp);



		/*distance formula for comparing two histograms
		 * 
		 * 
		 *
		 */		
		FILE *fp2 = fopen("Picture_Histograms/hist.txt", "r");		
		if(fp2 == NULL){
			printf("Error!");
			exit(1);
		}
		
		/*make array to hold histograms from text files
		 *to compare with recent taken histogram
		 * 
		 */
		int output4[256];
		for(int i = 0; i<256; i++){
			output4[i] = 0;
		}
		
		int finalSoFar = INT_MAX;
		int final = 0.0;
		char finalName[100];
		char buf[100];		
		
		while(fscanf(fp2, "%d", output4) != -1){
			
			/*output4 has histogram values from text file
			 *buf has corresponding name of histogram
			 */
			for(int i =1; i < 256; i++){
				fscanf(fp2, "%d", output4+i);
			}
			fgets(buf,100,fp2);
			
			final = 0;
			for(int i = 0; i< 256; i++){
				
				int z = (output3[i] + output4[i]);
				if( z == 0){
					final += 0;
					continue;
				}
				int x = (output3[i] - output4[i]);
				int y = (x*x);
				final += (0.5*(y/z));
			}
			
			printf("%d \n", final);
			printf("%s \n", buf);
			
			//uncomment this to print only the closest match
			//if(final < finalSoFar){
				//finalSoFar = final;
				//for(int i = 0; i < 100; i++){
					//finalName[i] = buf[i];
				//}
			//}
			
		}
		
		//printf("%d \n", finalSoFar);
		//printf("%s \n", finalName);
		
		fclose(fp2);
		
		
		//free 2 mallocs
		for(int i = 0; i<W; i++){
			free(output1[i]);
		} 
		free(output1);

        // tear down the camera
        printf( "camera teardown\n");
        raspistill_tear_down(state,status);
        printf( "camera teardown done. Exiting\n");
    }

}
