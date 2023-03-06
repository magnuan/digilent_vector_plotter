#include "sample.h"
#include "cmath.h"

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
        int ii = sscanf(line,"%f %f",&x, &y);
		if (ii>=2){
            data[cnt][0]=x;
            data[cnt][1]=y;
        }
    }
    return cnt;
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

    #define MAX_PATH_LENGTH 65536
    
	int c;
    float path_data[MAX_PATH_LENGTH][2];
    int path_len;
    float Fs = 1000.;
	
    /**** PARSING COMMAND LINE OPTIONS ****/
    while ((c = getopt (argc, argv, "c:m:F:?hV")) != -1) {
		switch (c) {
			case 'c':
				path_len = read_path_from_csv(optarg, path_data,MAX_PATH_LENGTH);
                if (path_len <= 0) return -1;
				break;
			case 'm':
				path_len = read_path_from_raster_matrix(optarg, path_data,MAX_PATH_LENGTH);
                if (path_len <= 0) return -1;
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
    double rgdSamples[4096];
    char szError[512] = {0};
    path_len = 4096;

    
    // generate custom samples normalized to +-1
    for(int i = 0; i < 4096; i++) rgdSamples[i] = 2.0*i/4095-1;
    
    printf("Open automatically the first available device\n");
    if(!FDwfDeviceOpen(-1, &hdwf)) {
        FDwfGetLastErrorMsg(szError);
        printf("Device open failed\n\t%s", szError);
        return 0;
    
    }
    
    int SamplesMin;
    int SamplesMax;
    FDwfAnalogOutNodeDataInfo(hdwf, 0, AnalogOutNodeCarrier, &SamplesMin, &SamplesMax);
    printf("SamplesMin = %d,  SamplesMax = %d\n",SamplesMin,SamplesMax);
    int frame_len = LIMIT(path_len,SamplesMin,SamplesMax);
    
    double *xSamples = (double*) malloc(frame_len*sizeof(double));
    double *ySamples = (double*) malloc(frame_len*sizeof(double));


    printf("Generating custom waveform for 5 seconds...");    
    for (int ch=0; ch <2;ch++){
        // enable first channel
        FDwfAnalogOutNodeEnableSet(hdwf, ch, AnalogOutNodeCarrier, true);
        // set custom function
        FDwfAnalogOutNodeFunctionSet(hdwf, ch, AnalogOutNodeCarrier, funcCustom);
        // set custom waveform samples
        FDwfAnalogOutNodeFrequencySet(hdwf, ch, AnalogOutNodeCarrier, Fs/frame_len);
        // 2V amplitude, 4V pk2pk, for sample value -1 will output -2V, for 1 +2V
        // normalized to ±1 values
        FDwfAnalogOutNodeAmplitudeSet(hdwf, ch, AnalogOutNodeCarrier, 2);
    }
    //Set channel 1 to follow channel 0
    FDwfAnalogOutMasterSet(hdwf, 1, 0);  
    
    for (int ix=0; ix < frame_len;ix++){
        xSamples[ix] = rgdSamples[ix];
        ySamples[ix] = rgdSamples[(2*ix)%frame_len];
    }


    FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, xSamples, frame_len);
    FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, ySamples, frame_len);
    // by default the offset is 0V
    // start signal generation. Only start channel 0 (master), channel 1 (slave) is set to follow
    FDwfAnalogOutConfigure(hdwf, 0, true);
    // it will run until stopped or device closed
    Wait(15);
    printf("done\n");
    // on close device is stopped and configuration lost
    FDwfDeviceCloseAll();


    free(xSamples);
    free(ySamples);
}
