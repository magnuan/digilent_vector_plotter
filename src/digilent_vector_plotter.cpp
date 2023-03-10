/* An atempt to stream one by one frame, unsyncronized  with the funcCustom mode
*/

#include "sample.h"
#include "cmath.h"
#include <time.h>
#include <math.h>
#include <climits>

static char verbose = 0;

int read_path_from_raster_matrix(char* fname, float data[][2],int max_len){
    return 0;
}

int read_path_from_csv(char* fname, float data[][2], int max_len){
	FILE * fp = fopen(fname,"r");
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	char * c;

	if (fp==NULL){
		fprintf(stderr,"Could not open file %s for reading config\n",fname);
		return -1;
	}
	int cnt = 0;
	while (((read = getline(&line, &len, fp)) != -1) && (cnt<max_len)) {
        float x,y;
        int ii = sscanf(line,"%f,%f",&x, &y);
		if (ii>=2){
            data[cnt][0]=y;
            data[cnt][1]=x;
            cnt++;
        }
    }
    return cnt;
}

int gen_demo_path(double xdata[], double ydata[], int len, int mode ){
    for (int ix=0;ix<len;ix++){
        float phase = (float)ix/float(len)*2*M_PI;
        float x,y;
        switch (mode){
            case 0:
            default:
                x = (0+sinf(phase))/2;
                y = (0+cosf(phase))/2;
                break;
            case 1:
                x = (0+sinf(phase))/2;
                y = (0+sinf(2*phase))/2;
                break;
        }
        ydata[ix]=y;
        xdata[ix]=x;
    }
    return len;
}


void showUsage(char *pgmname)
{
		printf("Usage: %s [OPTIONS] \n", pgmname);
		printf(	"Digilent_vector_plotter.\n"
			"Stream data from file to digilen analog discovery 2 waveform generator\n"
			"Input and outputs can be stdin/stdout, file, TCP or UDP ports\n"
			//"TCP ports are entered: Ex. 192.168.53.44:2210\n"
			//"UDP ports are entered: Ex. 192.168.53.100#5602\n"
			//"File is entered as filename.dump\n"
			//"Std in/out is denoted with dash (-)\n"
			"\t-c source\t Input CSV file\n"  
			"\t-m source\t Input CSV raster matrix file\n"  
			"\t-V \t Verbose mode\n"
			"\t-F Fs \t Output sample rate in Hz\n"
			);
}


int main(int argc,char *argv[]){

    #define MAX_PATH_LENGTH 8*32768
    
	int c;
    float Fs = 1000.;
	


    /**** PARSING COMMAND LINE OPTIONS ****/
    while ((c = getopt (argc, argv, "c:m:F:?hV")) != -1) {
		switch (c) {
			case 'c':
				//path_len = read_path_from_csv(optarg, path_data,MAX_PATH_LENGTH);
                //if (path_len <= 0) return -1;
				break;
			case 'm':
				//path_len = read_path_from_raster_matrix(optarg, path_data,MAX_PATH_LENGTH);
                //if (path_len <= 0) return -1;
				break;
			case 'F':
				Fs = atof(optarg);
                if ((Fs<1 ) || Fs>10e6){
                    fprintf(stderr,"Invalid sample rate"); 
                    return -1;
                }
				break;
			case '?':
			case 'h':
				showUsage(argv[0]);
				return(0);
			case 'V':
				verbose = 1;
				break;
		}
	}

    /*Digilent Analog discovery 2 stuff */
    HDWF hdwf;
    //double rgdSamples[4096];
    char szError[512] = {0};

    
    int nDev;
    //Enumerating all Analog Discovery 2 devices
    //FDwfEnum(3, &nDev); 
    //Enumerating all suported devices
    FDwfEnum(0, &nDev); 
    printf("%d devices found\n", nDev);
    for (int dix = 0; dix<nDev;dix++){
        int used;
        char user_name[32];
        char device_name[32];
        char serial[32];
        
        FDwfEnumDeviceIsOpened(dix, &used);
        FDwfEnumUserName(dix, user_name);
        FDwfEnumDeviceName(dix, device_name);
        FDwfEnumSN(dix, serial);
        printf("Device %d:\t %s \tSN:%s \t%s \t%s\n", dix, device_name, serial, used?"in use":"free", user_name);

    }
    
    int dix = 0;    //Hardcoded to use first available device
    printf("Using device %d\n",dix);
    int config_max = 0;
    int config_index = 0;
    
    int nConf;
    FDwfEnumConfig(dix, &nConf);
    for (int cix=0;cix<nConf;cix++){
        int val;
        /*for (int iix=1;iix<=10;iix++){
            FDwfEnumConfigInfo(cix,iix,&val);
            printf("Dev: %d  config: %d info: %d  val: %d\n",dix,cix,iix,val);
        }*/
    
        // Find configuration with largest Analog Out Buffer Size  (param 8)
        FDwfEnumConfigInfo(cix,8,&val); //Read out DECIAnalogOutBufferSize 
        if (val>config_max){
            config_max = val;
            config_index = cix;
        }
    }
    printf("Dev %d  Config %d has longest DECIAnalogOutBufferSize, %d samples\n",dix, config_index, config_max);

    printf("Open device %d with config %d\n",dix,config_index);
    if(!FDwfDeviceConfigOpen(dix,config_index, &hdwf)) {
        FDwfGetLastErrorMsg(szError);
        printf("Device open failed\n\t%s", szError);
        return 0;
    }

   
    
    int SamplesMin;
    int SamplesMax;
    FDwfAnalogOutNodeDataInfo(hdwf, 0, AnalogOutNodeCarrier, &SamplesMin, &SamplesMax);
    printf("SamplesMin = %d,  SamplesMax = %d\n",SamplesMin,SamplesMax);
    
    int frame_len = SamplesMax;
    



    for (int ch=0; ch <2;ch++){
        // enable first channel
        FDwfAnalogOutNodeEnableSet(hdwf, ch, AnalogOutNodeCarrier, true);
        // set custom waveform samples
        FDwfAnalogOutNodeFrequencySet(hdwf, ch, AnalogOutNodeCarrier, Fs/frame_len);
        // 2V amplitude, 4V pk2pk, for sample value -1 will output -2V, for 1 +2V
        // normalized to ?1 values
        FDwfAnalogOutNodeAmplitudeSet(hdwf, ch, AnalogOutNodeCarrier, 1);
        FDwfAnalogOutNodeFunctionSet(hdwf, ch, AnalogOutNodeCarrier, funcCustom);
    }
    //Set channel 1 to follow channel 0
    FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcNone); 
    FDwfAnalogOutMasterSet(hdwf, 1, 0);  
        
    double *xSamples = (double*) malloc(frame_len*sizeof(double));
    double *ySamples = (double*) malloc(frame_len*sizeof(double));
    
    frame_len = gen_demo_path(xSamples,ySamples,frame_len,1);
    //Send first (and perhaps only) data set to HW
    FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, xSamples, frame_len);
    FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, ySamples, frame_len);
    // start signal generation. Only start channel 0 (master), channel 1 (slave) is set to follow
    FDwfAnalogOutConfigure(hdwf, 0, true);
    
    // it will run until stopped or device closed

    time_t t0 = time(NULL);
    time_t t1 = time(NULL);

    clock_t begin,end;
    double time_spent = 0;

    int count=0;
    while ((t1-t0)<10){
        t1 = time(NULL);
        
        frame_len = gen_demo_path(xSamples,ySamples,frame_len,count%2);
        //Send first (and perhaps only) data set to HW
        begin = clock();

        //-- This part takes about 13ms with an Analog Discovery2 via USB, during which the output is stuck at its state
        FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, xSamples, frame_len);
        FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, ySamples, frame_len);
        FDwfAnalogOutConfigure(hdwf, 0, true);
        //-- This part end

        end = clock();
        
        time_spent += (double)(end - begin);

        Wait(0.02);
        count++;
        //usleep(100e3);
    }
    time_spent /= (CLOCKS_PER_SEC * count);
    printf("Average programming time = %fus\n",time_spent*1e6);

    // on close device is stopped and configuration lost
    FDwfDeviceCloseAll();


    free(xSamples);
    free(ySamples);
}
