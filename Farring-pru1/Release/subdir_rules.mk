################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: PRU Compiler'
	"C:/ti/ccsv6/eclipse/downloads/bin/clpru" -v3 -O2 --include_path="C:/ti/ccsv6/eclipse/downloads/include" --include_path="F:/Tronlong/AM5728/rootfs/pru-icss-5.0.1/include/am572x_2_0" --include_path="F:/Tronlong/AM5728/rootfs/pru-icss-5.0.1/include" --define=am5728 --define=icss1 --define=pru0 --display_error_number --diag_warning=225 --diag_wrap=off --hardware_mac=on --endian=little --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


