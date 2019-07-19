#include "cameraci.h"
#include <sysexits.h>
#include <omp.h>
#include <stdio.h>
#include <limits.h>


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
    int kval = 0;
    int W=3296;
    int H=2448;

    //then it gets 128 x 128
    state->timeout = 1000000;
    state->width = W;
    state->height = H;
    state->useRGB = 1;
    state->onlyLuma=0;
    state->verbose = 0;
    state->fullResPreview = 1;
    //state->filename= "test1.txt";


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

		    //setup output1 as 2d array of pointers
		    int **output1 = (int **)malloc(sizeof(int*) * W);
		    for(int i = 0; i< W; i++){
          output1[i] = (int*)malloc(sizeof(int) * H);
        }

		    //combine each rgb value into one pixel value
        for(int i = 0; i<W; i++){
          for(int j = 0; j<H; j++){
            kval = (buffer[k] + buffer[k+1] + buffer[k+2]);
            output1[i][j] = kval;
            k+=3;
          }
        }

		    //output3 holds histogram
		    int output3[256];
        //set everything to zero in array
        for(int i = 0; i<256; i++){
          output3[i] = 0;
        }


        //compare values to neighbour to get final pixel value for each cell
        int valToAdd = 0;
        //int i = 0;

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

		////print to txtfile and close file
		//for(int i =0; i<256; i++){
			//fprintf(fp,"%d ",output3[i]);
		//}
		//fprintf(fp," %s", argv[1]);
		//fprintf(fp,"\n");
		//fclose(fp);



		//distance formula for comparing two histograms
		FILE *fp2 = fopen("Picture_Histograms/hist.txt", "r");
		if(fp2 == NULL){
			printf("Error!");
			exit(1);
		}

		//make array for histogram from text files to compare with
		int output4[256];
		for(int i = 0; i<256; i++){
			output4[i] = 0;
		}

		float finalSoFar = INT_MAX;
		int final = 0.0;
		char finalName[100];
		char buf[100];

		while(fscanf(fp2, "%d", output4) != -1){

			//output4 has histogram values from text file
			//buf has corresponding name of histogram
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

			if(final < finalSoFar){
				finalSoFar = final;
				for(int i = 0; i < 100; i++){
					finalName[i] = buf[i];
        }
      }
    }

		printf("%f \n", finalSoFar);
		printf("%s \n", finalName);

    //close file that read histograms
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
