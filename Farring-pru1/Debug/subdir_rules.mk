################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	"C:/ti/ccsv6/eclipse/downloads/bin/clpru" -v3 --include_path="C:/ti/ccsv6/eclipse/downloads/include" --include_path="C:/ti/ccsv6/ccs_base/pru/include" --include_path="F:/6 am335x/6 - demo/2 - pruss/pru-icss-5.0.1/include" --include_path="F:/6 am335x/6 - demo/2 - pruss/pru-icss-5.0.1/include/am335x" -g --define=pru0 --define=am3359 --display_error_number --diag_warning=225 --diag_wrap=off --hardware_mac=on --endian=little --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


