#include <uhd.h>

#include "getopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


int main(int argc, char* argv[])
{
    int option           = 0;
    double freq          = 30e6;
    double rate          = 1e6;
    double gain          = 5.0;
    char* device_args    = "addr=192.168.10.2";
    size_t channel       = 0;
    char* filename       = "out.dat";
    char* filepath           = "/home/cnc-nqco/Documents/UHD_Test/Recv_file_fr_USRP";
    char filename_full[512];
    size_t n_samples     = 5000;
    bool verbose         = false;
    int return_code      = EXIT_SUCCESS;
    bool custom_filename = false;
    char error_string[512];

    sprintf(filename_full, "%s/%s", filepath, filename);
  
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
                n_samples = atoll(optarg);
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

    // Create RX streamer
    uhd_rx_streamer_handle rx_streamer;
    EXECUTE_OR_GOTO(free_usrp, uhd_rx_streamer_make(&rx_streamer))

    // Create RX metadata
    uhd_rx_metadata_handle md;
    EXECUTE_OR_GOTO(free_rx_streamer, uhd_rx_metadata_make(&md))

    // Create other necessary structs
    uhd_tune_request_t tune_request = {
        .target_freq = freq,
        .rf_freq_policy                             = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy                            = UHD_TUNE_REQUEST_POLICY_AUTO};
        
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {.cpu_format = "fc32",
    .otw_format                              = "sc16",
    .args                                    = "",
    .channel_list                            = &channel,
    .n_channels                              = 1};

    uhd_stream_cmd_t stream_cmd = {.stream_mode = UHD_STREAM_MODE_NUM_SAMPS_AND_DONE,
    .num_samps                              = n_samples,
    .stream_now                             = true};

    size_t samps_per_buff = 800;
    float* buffer          = NULL;
    void** buffer_ptr     = NULL;
    FILE* fp             = NULL;
    size_t num_acc_samps = 0;

    // Set sample rate
    fprintf(stderr, "Setting sample TX Rate: %f...\n", rate);
    EXECUTE_OR_GOTO(free_rx_metadata, uhd_usrp_set_rx_rate(usrp, rate, channel))

    // Set gain
    fprintf(stderr, "Setting RX Gain: %f dB...\n", gain);
    EXECUTE_OR_GOTO(free_rx_metadata, uhd_usrp_set_rx_gain(usrp, gain, channel, ""))

    // Set frequency
    fprintf(stderr, "Setting RX frequency: %f MHz...\n", freq/1e6);
    EXECUTE_OR_GOTO(free_rx_metadata, uhd_usrp_set_rx_freq(usrp, &tune_request, channel, &tune_result))

    // Set up streamer
    stream_args.channel_list = &channel;
    EXECUTE_OR_GOTO(free_rx_streamer, uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer))

    fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);

    buffer      = malloc(samps_per_buff * 2 * sizeof(float));
    buffer_ptr = (void**)&buffer;

    // Issue stream command
    fprintf(stderr, "Issuing stream command.\n");
    EXECUTE_OR_GOTO(free_buffer, uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd))

    // Mo file de ghi du lieu vao
    fp = fopen(filename_full, "wb");

    // Actual streaming
    while (num_acc_samps < n_samples) 
    {   
        //So sample moi lan thuc te doc ve
        size_t num_rx_samps = 0;
        EXECUTE_OR_GOTO(close_file,uhd_rx_streamer_recv(rx_streamer, buffer_ptr, samps_per_buff, &md, 3.0, false, &num_rx_samps))        
        
        //Kiem tra xem khi doc ve co loi hay khong
        //Ma loi se gan vao bien error_code
        uhd_rx_metadata_error_code_t error_code;
        EXECUTE_OR_GOTO(close_file, uhd_rx_metadata_error_code(md, &error_code))
        if (error_code != UHD_RX_METADATA_ERROR_CODE_NONE) 
        {
            fprintf(stderr,"Error code 0x%x was returned during streaming. Aborting.\n",error_code);
            goto close_file;
        }

        // Handle data
        fwrite(buffer, sizeof(float) * 2, num_rx_samps, fp);

        if (verbose) 
        {
            int64_t full_secs;
            double frac_secs;
            uhd_rx_metadata_time_spec(md, &full_secs, &frac_secs);
            fprintf(stderr,
                "Received packet: %zu samples, %.f full secs, %f frac secs\n",
                num_rx_samps,
                difftime(full_secs, (int64_t)0),
                frac_secs);
        }      

        num_acc_samps += num_rx_samps;
    }

    printf("Doc ve tong cong duoc %zu mau", num_acc_samps);


// Cleanup
close_file:
    fclose(fp);

free_buffer:
    if (buffer) 
    {
        if (verbose) 
        {
            fprintf(stderr, "Freeing buffer.\n");
        }
        free(buffer);
    }
    buffer      = NULL;
    buffer_ptr  = NULL;

free_rx_streamer:
    if (verbose) 
    {
        fprintf(stderr, "Cleaning up RX streamer.\n");
    }
    uhd_rx_streamer_free(&rx_streamer);

free_rx_metadata:
    if (verbose) 
    {
        fprintf(stderr, "Cleaning up RX metadata.\n");
    }
    uhd_rx_metadata_free(&md);

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
    {
        free(device_args);
    }
    if (custom_filename) 
    {
        free(filename);
    }

    fprintf(stderr, (return_code ? "Failure\n" : "Success\n"));
    return return_code;
}
