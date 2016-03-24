#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <pthread.h>

#include <candev/node.h>
#include <candev/kozak.h>

int done = 0;

void sig_handler(int signo)
{
	done = 1;
}

void printADCVoltage(void *cookie, const CEAC124_ADCReadResult *result)
{
	printf("measure_result = {\n");
	printf(" channel_number = %d,\n", (int) result->channel_number);
	printf(" gain_code = %d,\n", (int) result->gain_code);
	printf(" voltage_code = 0x%x,\n", result->voltage_code);
	printf(" voltage = %lf,\n", result->voltage);
	printf("}\n");
	fflush(stdout);
}

void printDACVoltage(void *cookie, const CEAC124_DACReadResult *result)
{
	printf("measure_result = {\n");
	printf(" channel_number = %d,\n", (int) result->channel_number);
	printf(" voltage_code = 0x%x,\n", result->voltage_code);
	printf(" voltage = %lf,\n", result->voltage);
	printf("}\n");
	fflush(stdout);
}

void printDevAttrib(void *cookie, const CEAC124_DevAttrib *attr)
{
	printf("device_attrib = {\n");
	printf(" device_code = %d,\n", (int) attr->device_code);
	printf(" hw_version  = %d,\n", (int) attr->hw_version);
	printf(" sw_version = %d,\n", (int) attr->sw_version);
	printf(" reason = %d,\n", (int) attr->reason);
	printf("}\n");
	fflush(stdout);
}

void printDevStatus(void *cookie, const CEAC124_DevStatus *st)
{
	printf("device_status = {\n");
	printf(" dev_mode = 0x%x,\n", (int) st->dev_mode);
	printf(" label  = %d,\n", (int) st->label);
	printf(" pADC = %d,\n", (int) st->padc);
	printf(" file_ident = %d,\n", (int) st->file_ident);
	printf(" pDAC = %d,\n", (int) st->pdac);
	printf("}\n");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int status;
	
	CAN_Node node;
	status = CAN_createNode(&node, "can0");
	if(status != 0)
		return 1;

	printf("Node created\n");
	
	CEAC124 dev;
	CEAC124_setup(&dev, 0x1F, &node);
	dev.cb_adc_read_s = printADCVoltage;
	dev.cb_dac_read   = printDACVoltage;
	dev.cb_dev_attrib = printDevAttrib;
	dev.cb_dev_status = printDevStatus;
	
	CEAC124_getDevAttrib(&dev);
	CEAC124_getDevStatus(&dev);
	
	CEAC124_DACWriteProp wprop;
	wprop.channel_number = 0;
	wprop.use_code = 0;
	wprop.voltage = 2.0;
	CEAC124_dacWrite(&dev, &wprop);
	
	CEAC124_DACReadProp rprop;
	rprop.channel_number = 0;
	status = CEAC124_dacRead(&dev, &rprop);
	
	CEAC124_ADCReadSProp prop;
	prop.channel_number = 4;
	prop.gain_code = CEAC124_ADC_READ_GAIN_1;
	prop.time = CEAC124_ADC_READ_TIME_160MS;
	prop.mode = CEAC124_ADC_READ_MODE_CONTINUOUS | CEAC124_ADC_READ_MODE_SEND;
	CEAC124_adcReadS(&dev, &prop);
	
	fflush(stdout);
	
	if(signal(SIGTERM, sig_handler) == SIG_ERR || signal(SIGINT, sig_handler) == SIG_ERR)
	{
	  fprintf(stderr, "unable to catch some signals\n");
		return 5;
	}
	
	CEAC124_listen(&dev, &done);
	
	CEAC124_adcStop(&dev);
	CAN_destroyNode(&node);
	
	printf("exiting...\n");
	fflush(stdout);
	
	return 0;
}
