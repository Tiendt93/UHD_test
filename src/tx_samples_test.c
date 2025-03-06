#include <uhd.h>

#include "getopt.h"

#ifndef __USE_MISC
#define __USE_MISC
#endif

#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MHz *1000000.0
#define kHz *1000.0


#define EXECUTE_OR_GOTO(label, ...) \
    if (__VA_ARGS__) {              \
        return_code = EXIT_FAILURE; \
        goto label;                 \
    }


void print_help(void)
{
    fprintf(stderr,"Options:\n"
        "    -a (device args)\n"
        "    -f (frequency in Hz)\n"
        "    -r (sample rate in Hz)\n"
        "    -g (gain)\n"
        "    -n (number of samples to transmit)\n"
        "    -v (enable verbose prints)\n"
        "    -h (print this help message)\n");
}

bool stop_signal_called = false;

void sigint_handler(int code)
{
    (void)code;
    stop_signal_called = true;
}


int main(int argc, char* argv[])
{

    int option               = 0;
    double freq              = 30e6;
    double rate              = 1e6;
    double gain              = 8;
    char* device_args        = "addr=192.168.10.2";
    size_t channel           = 0;
    uint64_t total_num_samps = 0;
    bool verbose             = false;
    int return_code          = EXIT_SUCCESS;
    char error_string[512];

    // Process options
    while ((option = getopt(argc, argv, "a:f:r:g:n:vh")) != -1) 
    {
        switch (option) 
        {
            case 'a':
                device_args = strdup(optarg);
                break;

            case 'f':
                freq = atof(optarg);
                break;

            case 'r':
                rate = atof(optarg);
                break;

            case 'g':
                gain = atof(optarg);
                break;

            case 'n':
                total_num_samps = atoll(optarg);
                break;

            case 'v':
                verbose = true;
                break;

            case 'h':
                print_help();
                goto free_option_strings;

            default:
                print_help();
                return_code = EXIT_FAILURE;
                goto free_option_strings;
        }
    }

    //Khoi tao cac tham so cho USRP
    // Create USRP
    uhd_usrp_handle usrp;
    fprintf(stderr, "Creating USRP with Ip address \"%s\"...\n", device_args);
    EXECUTE_OR_GOTO(free_option_strings, uhd_usrp_make(&usrp, device_args))

    // Create TX streamer
    uhd_tx_streamer_handle tx_streamer;
    EXECUTE_OR_GOTO(free_usrp, uhd_tx_streamer_make(&tx_streamer))

    // Create TX metadata
    uhd_tx_metadata_handle md;
    EXECUTE_OR_GOTO(free_tx_streamer, uhd_tx_metadata_make(&md, false, 0, 0.1, true, false))

    // Create other necessary structs
    uhd_tune_request_t tune_request = {.target_freq = freq,
        .rf_freq_policy                             = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy                            = UHD_TUNE_REQUEST_POLICY_AUTO};
        
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {.cpu_format = "fc32",
        .otw_format                              = "sc16",
        .args                                    = "",
        .channel_list                            = &channel,
        .n_channels                              = 1};

    size_t samps_per_buff = 20000;
    float* data            = NULL;
    const void** data_ptr = NULL;


    // Set sample rate
    fprintf(stderr, "Setting sample TX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_set_tx_rate(usrp, rate, channel))

    // Set gain
    fprintf(stderr, "Setting TX Gain: %f db...\n", gain);
    EXECUTE_OR_GOTO(free_tx_metadata, uhd_usrp_set_tx_gain(usrp, gain, 0, ""))

    // Set frequency
    fprintf(stderr, "Setting TX frequency: %f MHz...\n", freq / 1e6);
    EXECUTE_OR_GOTO(free_tx_metadata,uhd_usrp_set_tx_freq(usrp, &tune_request, channel, &tune_result))

    // Set up streamer
    stream_args.channel_list = &channel;
    EXECUTE_OR_GOTO(free_tx_streamer, uhd_usrp_get_tx_stream(usrp, &stream_args, tx_streamer))

    //Khai bao buffer chua data phat di
    data = calloc((samps_per_buff * 2), sizeof(float));
    data_ptr = (const void**)&data;

    size_t i,j  = 0;
    float ampl = 1 ;
    float freq_carr = 1 kHz;
    float temp;
    float phase = 0;
    float data_I, data_Q;
    size_t dem=0;

    for (i = 0; i < samps_per_buff; i ++) 
    {   
        temp = 2*M_PI*freq_carr*i/rate + phase;
        data_I = ampl*cos(temp);
        data_Q = ampl*sin(temp);
    
        data[dem++]   = data_I;
        data[dem++]   = data_Q;
    }

    // Ctrl+C will exit loop
    signal(SIGINT, &sigint_handler);
    fprintf(stderr, "Press Ctrl+C to stop streaming...\n");

    // Actual streaming
    uint64_t num_acc_samps = 0;
    size_t num_samps_sent  = 0;

    while (1) 
    {
        if (stop_signal_called)
            break;
        if (total_num_samps > 0 && num_acc_samps >= total_num_samps)
            break;

        EXECUTE_OR_GOTO(free_data,uhd_tx_streamer_send(tx_streamer, data_ptr, samps_per_buff, &md, 0.1, &num_samps_sent))

        num_acc_samps += num_samps_sent;


        //Cac bien de check
        printf("So mau trong buffer: %zu\n", samps_per_buff);
        printf("Vua chuyen %zu mau\n", num_samps_sent);
        printf("====>Tong so da chuyen %lu mau\n", num_acc_samps);

        if (verbose) 
        {
            fprintf(stderr, "Sent %zu samples\n", num_samps_sent);
        }
    }

free_data:
    free(data);

free_tx_metadata:
    if (verbose) 
    {
        fprintf(stderr, "Cleaning up TX metadata.\n");
    }
    uhd_tx_metadata_free(&md);

free_tx_streamer:
    if (verbose) 
    {
        fprintf(stderr, "Cleaning up TX streamer.\n");
    }
    uhd_tx_streamer_free(&tx_streamer);

free_usrp:
    if (verbose) 
    {
        fprintf(stderr, "Cleaning up USRP.\n");
    }
    if (return_code != EXIT_SUCCESS && usrp != NULL) 
    {
        uhd_usrp_last_error(usrp, error_string, 512);
        fprintf(stderr, "USRP reported the following error: %s\n", error_string);
    }
    uhd_usrp_free(&usrp);

free_option_strings:
    if (device_args)
        free(device_args);

    fprintf(stderr, (return_code ? "Failure\n" : "Success\n"));
    

    return 0;
}