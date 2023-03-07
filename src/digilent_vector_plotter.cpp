#include "sample.h"
#include "cmath.h"
#include <time.h>

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
            data[cnt][0]=x;
            data[cnt][1]=y;
            cnt++;
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
    printf("Path length = %d\n",path_len);

    /*Digilent Analog discovery 2 stuff */
    HDWF hdwf;
    //double rgdSamples[4096];
    char szError[512] = {0};
    //path_len = 4096;

    
    // generate custom samples normalized to +-1
    //for(int i = 0; i < 4096; i++) rgdSamples[i] = 2.0*i/4095-1;
   
    #if 0
    printf("Open automatically the first available device\n");
    if(!FDwfDeviceOpen(-1, &hdwf)) {
        FDwfGetLastErrorMsg(szError);
        printf("Device open failed\n\t%s", szError);
        return 0;
    
    }
    #else
    int nDev;
    //Enumerating all Analog Discovery 2 devices
    FDwfEnum(3, &nDev); 

    int config_max = 0;
    int config_index = 0;
    for (int dix = 0; dix<nDev;dix++){
        int nConf;
        FDwfEnumConfig(dix, &nConf);
        for (int cix=0;cix<nConf;cix++){
            int val;
            /*for (int iix=1;iix<=10;iix++){
                FDwfEnumConfigInfo(cix,iix,&val);
                printf("Dev: %d  config: %d info: %d  val: %d\n",dix,cix,iix,val);
            }*/

            FDwfEnumConfigInfo(cix,8,&val); //Read out DECIAnalogOutBufferSize 
            if (val>config_max){
                config_max = val;
                config_index = cix;
            }
        }
        printf("Dev %d  Config %d has longest DECIAnalogOutBufferSize, %d samples\n",dix, config_index, config_max);
    }
    printf("Open device %d with config %d\n",0,config_index);
    if(!FDwfDeviceConfigOpen(0,config_index, &hdwf)) {
        FDwfGetLastErrorMsg(szError);
        printf("Device open failed\n\t%s", szError);
        return 0;
    
    }
    #endif

    

   
    
    int SamplesMin;
    int SamplesMax;
    FDwfAnalogOutNodeDataInfo(hdwf, 0, AnalogOutNodeCarrier, &SamplesMin, &SamplesMax);
    printf("SamplesMin = %d,  SamplesMax = %d\n",SamplesMin,SamplesMax);
    
    int frame_len = LIMIT(path_len,SamplesMin,SamplesMax);

    double *xSamples = (double*) malloc(frame_len*sizeof(double));
    double *ySamples = (double*) malloc(frame_len*sizeof(double));

    bool single_frame = (frame_len==path_len);

    for (int ch=0; ch <2;ch++){
        // enable first channel
        FDwfAnalogOutNodeEnableSet(hdwf, ch, AnalogOutNodeCarrier, true);
        // set custom waveform samples

        if (single_frame)
            FDwfAnalogOutNodeFrequencySet(hdwf, ch, AnalogOutNodeCarrier, Fs/frame_len);
        else
            FDwfAnalogOutNodeFrequencySet(hdwf, ch, AnalogOutNodeCarrier, Fs);

        // 2V amplitude, 4V pk2pk, for sample value -1 will output -2V, for 1 +2V
        // normalized to ±1 values
        FDwfAnalogOutNodeAmplitudeSet(hdwf, ch, AnalogOutNodeCarrier, 5);
    }
    if (single_frame){
        FDwfAnalogOutNodeFunctionSet(hdwf, 0, AnalogOutNodeCarrier, funcCustom);
        FDwfAnalogOutNodeFunctionSet(hdwf, 1, AnalogOutNodeCarrier, funcCustom);
    }
    else{
        FDwfAnalogOutNodeFunctionSet(hdwf, 0, AnalogOutNodeCarrier, funcPlay);
        FDwfAnalogOutNodeFunctionSet(hdwf, 1, AnalogOutNodeCarrier, funcPlay);
    }
    FDwfAnalogOutTriggerSourceSet(hdwf, 0, trigsrcNone); 
    //Set channel 1 to follow channel 0
    FDwfAnalogOutMasterSet(hdwf, 1, 0);  
        

    //Send first (and perhaps only) data set to HW
    for (int ix=0; ix < frame_len;ix++){
        xSamples[ix] = path_data[ix][0];
        ySamples[ix] = path_data[ix][1];
    }
    FDwfAnalogOutNodeDataSet(hdwf, 0, AnalogOutNodeCarrier, xSamples, frame_len);
    FDwfAnalogOutNodeDataSet(hdwf, 1, AnalogOutNodeCarrier, ySamples, frame_len);
    // start signal generation. Only start channel 0 (master), channel 1 (slave) is set to follow
    FDwfAnalogOutConfigure(hdwf, 0, true);
    
    if (single_frame){
        printf("Generating custom waveform for 5 seconds...\n");    
        // it will run until stopped or device closed
        Wait(5);
        printf("done\n");
    }
    else{
        //First frame_len of data is allready sent 
        int path_offset[2] = {frame_len, frame_len};

        time_t t0 = time(NULL);
        time_t t1 = time(NULL);

        while ((t1-t0)<5){
            
            for (int ch=0;ch<2;ch++){
                //printf("LOOP %d: Samples left = %d\n",loop,samples_left);
                //Wait untill a minimum number of transfres are possible
                int data_free;
                int data_lost;
                int data_corrupted;
                DwfState ch_status;
                
                FDwfAnalogOutStatus(hdwf, ch, &(ch_status)); 
                FDwfAnalogOutNodePlayStatus(hdwf, ch, AnalogOutNodeCarrier,&(data_free), &(data_lost), &(data_corrupted));

                if (data_lost || data_corrupted){
                    printf("CH %d free:%d lost:%d corrupted:%d\n",ch,data_free,data_lost, data_corrupted);
                }
                if (data_free>512){
                    //Calculate number of samples to copy from circular input buffer
                    int block1_len = MIN(path_len - path_offset[ch], data_free);
                    int block2_len = data_free-block1_len;
                    for (int ix=0; ix < block1_len;ix++){
                        xSamples[ix] = path_data[ix+path_offset[ch]][ch];
                    }
                    for (int ix=0; ix < block2_len;ix++){
                        xSamples[block1_len+ix] = path_data[ix][ch];
                    }
                    FDwfAnalogOutNodePlayData(hdwf, ch, AnalogOutNodeCarrier,xSamples,data_free); 
                    path_offset[ch] = (path_offset[ch]+data_free)%path_len;
                }
            }
            t1 = time(NULL);
            usleep(100);
        }
    }

    // on close device is stopped and configuration lost
    FDwfDeviceCloseAll();


    free(xSamples);
    free(ySamples);
}
